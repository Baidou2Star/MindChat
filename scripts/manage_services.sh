#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CMD="${1:-}"
if [[ "${CMD}" == "logs" ]]; then
  BUILD_DIR_REL="build"
else
  BUILD_DIR_REL="${2:-build}"
fi
BUILD_DIR="${ROOT_DIR}/${BUILD_DIR_REL}"
PID_DIR="${ROOT_DIR}/.run"
LOG_DIR="${ROOT_DIR}/logs/services"

CPP_SERVICES=(
  "StatusServer"
  "ChatServer"
  "ChatServer2"
  "ResourceServer"
  "GateServerWin"
)

VARIFY_SERVICE="VarifyServer"

trim() {
  local s="$1"
  s="${s#${s%%[![:space:]]*}}"
  s="${s%${s##*[![:space:]]}}"
  printf '%s' "$s"
}

ini_get() {
  local file="$1"
  local section="$2"
  local key="$3"
  awk -F'=' -v sec="$section" -v k="$key" '
    function trim(x){ sub(/^[ \t\r\n]+/, "", x); sub(/[ \t\r\n]+$/, "", x); return x }
    /^[ \t]*\[/ {
      cur=$0
      gsub(/^[ \t]*\[/, "", cur)
      gsub(/\][ \t]*$/, "", cur)
      cur=trim(cur)
      next
    }
    cur==sec {
      lhs=trim($1)
      if (lhs==k) {
        rhs=substr($0, index($0, "=")+1)
        print trim(rhs)
        exit
      }
    }
  ' "$file"
}

usage() {
  cat <<USAGE
Usage:
  $(basename "$0") <start|stop|restart|status> [build_dir]
  $(basename "$0") logs <ServiceName>

Examples:
  $(basename "$0") start
  $(basename "$0") start build-linux
  $(basename "$0") stop
  $(basename "$0") status build
  $(basename "$0") logs VarifyServer

Optional env overrides:
  REDIS_START_CMD="..."
  MYSQL_START_CMD="..."
  VARIFY_START_CMD="..."   (default: npm run serve)
USAGE
}

pid_file() {
  local svc="$1"
  echo "${PID_DIR}/${svc}.pid"
}

log_file() {
  local svc="$1"
  echo "${LOG_DIR}/${svc}.log"
}

svc_bin() {
  local svc="$1"
  echo "${BUILD_DIR}/${svc}/${svc}"
}

is_pid_running() {
  local pid="$1"
  kill -0 "$pid" >/dev/null 2>&1
}

is_running() {
  local svc="$1"
  local pf
  pf="$(pid_file "$svc")"
  if [[ -f "$pf" ]]; then
    local pid
    pid="$(cat "$pf")"
    if [[ -n "$pid" ]] && is_pid_running "$pid"; then
      return 0
    fi
  fi
  return 1
}

ensure_dirs() {
  mkdir -p "$PID_DIR" "$LOG_DIR"
}

check_bins_exist() {
  local missing=0
  for svc in "${CPP_SERVICES[@]}"; do
    local bin
    bin="$(svc_bin "$svc")"
    if [[ ! -x "$bin" ]]; then
      echo "[ERROR] missing binary: $bin"
      missing=1
    fi
  done
  if [[ "$missing" -ne 0 ]]; then
    echo "[HINT] build first: cmake --build ${BUILD_DIR_REL} -j"
    exit 1
  fi
}

sync_runtime_configs() {
  local svc src_cfg dst_cfg
  for svc in "${CPP_SERVICES[@]}"; do
    src_cfg="${ROOT_DIR}/${svc}/config.ini"
    dst_cfg="${BUILD_DIR}/${svc}/config.ini"
    if [[ -f "$src_cfg" ]]; then
      mkdir -p "$(dirname "$dst_cfg")"
      cp -f "$src_cfg" "$dst_cfg"
    fi
  done
}

is_port_open() {
  local host="$1"
  local port="$2"
  timeout 1 bash -c "</dev/tcp/${host}/${port}" >/dev/null 2>&1
}

wait_port() {
  local name="$1"
  local host="$2"
  local port="$3"
  local retries="${4:-20}"
  local i
  for ((i=1; i<=retries; i++)); do
    if is_port_open "$host" "$port"; then
      echo "[OK]   ${name} is up at ${host}:${port}"
      return 0
    fi
    sleep 0.5
  done
  return 1
}

run_start_cmd() {
  local desc="$1"
  local cmd="$2"
  if [[ -z "$cmd" ]]; then
    return 1
  fi
  echo "[INFO] trying to start ${desc}: ${cmd}"
  bash -lc "$cmd" >/dev/null 2>&1 || return 1
  return 0
}

check_varify_node_env() {
  if ! command -v node >/dev/null 2>&1; then
    echo "[ERROR] node not found in WSL environment"
    echo "[HINT] install Linux Node.js first (nvm recommended)"
    return 1
  fi

  if ! command -v npm >/dev/null 2>&1; then
    echo "[ERROR] npm not found in WSL environment"
    return 1
  fi

  local npm_path
  npm_path="$(command -v npm)"
  if [[ "$npm_path" == /mnt/* ]] || [[ "$npm_path" == *.exe ]]; then
    echo "[ERROR] npm points to Windows path: $npm_path"
    echo "[HINT] use Linux npm in WSL (do not use /mnt/... npm)"
    return 1
  fi
}

ensure_redis_mysql() {
  local cfg_file="${ROOT_DIR}/StatusServer/config.ini"
  if [[ ! -f "$cfg_file" ]]; then
    echo "[ERROR] missing config: $cfg_file"
    return 1
  fi

  local redis_host redis_port mysql_host mysql_port
  redis_host="$(ini_get "$cfg_file" "Redis" "Host")"
  redis_port="$(ini_get "$cfg_file" "Redis" "Port")"
  mysql_host="$(ini_get "$cfg_file" "Mysql" "Host")"
  mysql_port="$(ini_get "$cfg_file" "Mysql" "Port")"

  redis_host="$(trim "${redis_host:-127.0.0.1}")"
  redis_port="$(trim "${redis_port:-6379}")"
  mysql_host="$(trim "${mysql_host:-127.0.0.1}")"
  mysql_port="$(trim "${mysql_port:-3306}")"

  if ! is_port_open "$redis_host" "$redis_port"; then
    echo "[WARN] Redis not reachable at ${redis_host}:${redis_port}"

    if [[ -n "${REDIS_START_CMD:-}" ]]; then
      run_start_cmd "Redis" "$REDIS_START_CMD" || true
    else
      run_start_cmd "Redis" "systemctl start redis-server" || true
      run_start_cmd "Redis" "systemctl start redis" || true
      run_start_cmd "Redis" "service redis-server start" || true
      run_start_cmd "Redis" "service redis start" || true
      run_start_cmd "Redis" "redis-server --daemonize yes" || true
    fi

    if ! wait_port "Redis" "$redis_host" "$redis_port" 20; then
      echo "[ERROR] Redis is still unavailable at ${redis_host}:${redis_port}"
      echo "[HINT] set REDIS_START_CMD, e.g. REDIS_START_CMD='service redis-server start' or 'redis-server --daemonize yes'"
      return 1
    fi
  else
    echo "[OK]   Redis already running at ${redis_host}:${redis_port}"
  fi

  if ! is_port_open "$mysql_host" "$mysql_port"; then
    echo "[WARN] MySQL not reachable at ${mysql_host}:${mysql_port}"

    if [[ -n "${MYSQL_START_CMD:-}" ]]; then
      run_start_cmd "MySQL" "$MYSQL_START_CMD" || true
    else
      run_start_cmd "MySQL" "systemctl start mysql" || true
      run_start_cmd "MySQL" "systemctl start mysqld" || true
    fi

    if ! wait_port "MySQL" "$mysql_host" "$mysql_port" 30; then
      echo "[ERROR] MySQL is still unavailable at ${mysql_host}:${mysql_port}"
      echo "[HINT] set MYSQL_START_CMD, e.g. MYSQL_START_CMD='systemctl start mysql'"
      return 1
    fi
  else
    echo "[OK]   MySQL already running at ${mysql_host}:${mysql_port}"
  fi
}

varify_ready() {
  local host port
  read -r host port < <(varify_host_port)
  is_port_open "$host" "$port"
}

varify_host_port() {
  local host="127.0.0.1"
  local port="50051"
  local gate_cfg="${ROOT_DIR}/GateServerWin/config.ini"
  if [[ -f "$gate_cfg" ]]; then
    host="$(trim "$(ini_get "$gate_cfg" "VarifyServer" "Host")")"
    port="$(trim "$(ini_get "$gate_cfg" "VarifyServer" "Port")")"
    host="${host:-127.0.0.1}"
    port="${port:-50051}"
  fi
  echo "$host $port"
}

start_varify() {
  local svc="$VARIFY_SERVICE"
  local pf lf
  pf="$(pid_file "$svc")"
  lf="$(log_file "$svc")"

  if is_running "$svc"; then
    echo "[SKIP] ${svc} already running (pid $(cat "$pf"))"
    return 0
  fi

  # If PID file is missing but gRPC port is already listening,
  # treat VarifyServer as already running to avoid double-start bind errors.
  if varify_ready; then
    echo "[SKIP] ${svc} port is already reachable (likely already running)"
    return 0
  fi

  local app_dir="${ROOT_DIR}/VarifyServer"
  if [[ ! -f "${app_dir}/server.js" ]]; then
    echo "[ERROR] missing VarifyServer/server.js"
    return 1
  fi

  if ! check_varify_node_env; then
    return 1
  fi

  if [[ ! -d "${app_dir}/node_modules" ]]; then
    echo "[WARN] VarifyServer/node_modules not found"
    if [[ "${VARIFY_AUTO_NPM_INSTALL:-0}" == "1" ]]; then
      echo "[INFO] auto install npm deps for VarifyServer"
      (cd "$app_dir" && npm install)
    else
      echo "[HINT] run: (cd VarifyServer && npm install)"
    fi
  fi

  if [[ ! -d "${app_dir}/node_modules/@grpc/grpc-js" ]]; then
    echo "[WARN] VarifyServer dependency '@grpc/grpc-js' is missing"
    if [[ "${VARIFY_AUTO_NPM_INSTALL:-0}" == "1" ]]; then
      echo "[INFO] retry npm install for VarifyServer"
      (cd "$app_dir" && npm install)
    else
      echo "[HINT] run: (cd VarifyServer && npm install)"
      return 1
    fi
  fi

  local start_cmd="${VARIFY_START_CMD:-npm run serve}"

  : > "$lf"
  (
    cd "$app_dir"
    nohup bash -lc "$start_cmd" >> "$lf" 2>&1 &
    echo $! > "$pf"
  )

  sleep 0.5
  if is_running "$svc"; then
    echo "[OK]   started ${svc} (pid $(cat "$pf"))"
  else
    echo "[FAIL] ${svc} exited quickly, check log: $lf"
    return 1
  fi

  if varify_ready; then
    echo "[OK]   VarifyServer gRPC port is reachable"
  else
    echo "[WARN] VarifyServer process is up, but port is not ready yet"
  fi
}

stop_varify() {
  local svc="$VARIFY_SERVICE"
  local pf
  pf="$(pid_file "$svc")"

  if ! is_running "$svc"; then
    [[ -f "$pf" ]] && rm -f "$pf"
    if ! varify_ready; then
      echo "[SKIP] ${svc} not running"
      return 0
    fi

    echo "[INFO] ${svc} has no pid file, trying fallback stop by process/port"
    pkill -f "/VarifyServer/server.js" >/dev/null 2>&1 || true
    pkill -f "node server.js" >/dev/null 2>&1 || true

    local host port
    read -r host port < <(varify_host_port)
    local pids
    pids="$(ss -ltnp 2>/dev/null | awk -v p=":${port}" '$4 ~ p {print $0}' | sed -n 's/.*pid=\\([0-9][0-9]*\\).*/\\1/p' | sort -u)"
    if [[ -n "$pids" ]]; then
      while IFS= read -r pid; do
        [[ -n "$pid" ]] && kill "$pid" >/dev/null 2>&1 || true
      done <<< "$pids"
    fi

    sleep 0.5
    if varify_ready; then
      echo "[WARN] ${svc} port still reachable, may be managed by another user/service"
    else
      echo "[OK]   stopped ${svc} (fallback mode)"
    fi
    return 0
  fi

  local pid
  pid="$(cat "$pf")"
  kill "$pid" >/dev/null 2>&1 || true

  local i
  for ((i=0; i<25; i++)); do
    if ! is_pid_running "$pid"; then
      break
    fi
    sleep 0.2
  done

  if is_pid_running "$pid"; then
    kill -9 "$pid" >/dev/null 2>&1 || true
    echo "[WARN] force killed ${svc} (pid $pid)"
  else
    echo "[OK]   stopped ${svc}"
  fi

  rm -f "$pf"
}

start_one_cpp() {
  local svc="$1"
  local bin lf pf
  bin="$(svc_bin "$svc")"
  lf="$(log_file "$svc")"
  pf="$(pid_file "$svc")"

  if is_running "$svc"; then
    echo "[SKIP] ${svc} already running (pid $(cat "$pf"))"
    return 0
  fi

  : > "$lf"
  (
    cd "$(dirname "$bin")"
    nohup "./${svc}" >> "$lf" 2>&1 &
    echo $! > "$pf"
  )

  sleep 0.4
  if is_running "$svc"; then
    echo "[OK]   started ${svc} (pid $(cat "$pf"))"
  else
    echo "[FAIL] ${svc} exited quickly, check log: $lf"
    return 1
  fi
}

stop_one_cpp() {
  local svc="$1"
  local pf
  pf="$(pid_file "$svc")"

  if ! is_running "$svc"; then
    [[ -f "$pf" ]] && rm -f "$pf"
    echo "[SKIP] ${svc} not running"
    return 0
  fi

  local pid
  pid="$(cat "$pf")"
  kill "$pid" >/dev/null 2>&1 || true

  local i
  for ((i=0; i<25; i++)); do
    if ! is_pid_running "$pid"; then
      break
    fi
    sleep 0.2
  done

  if is_pid_running "$pid"; then
    kill -9 "$pid" >/dev/null 2>&1 || true
    echo "[WARN] force killed ${svc} (pid $pid)"
  else
    echo "[OK]   stopped ${svc}"
  fi

  rm -f "$pf"
}

status_all() {
  local svc pf

  # dependency status
  local cfg_file="${ROOT_DIR}/StatusServer/config.ini"
  local redis_host redis_port mysql_host mysql_port
  redis_host="$(trim "$(ini_get "$cfg_file" "Redis" "Host")")"
  redis_port="$(trim "$(ini_get "$cfg_file" "Redis" "Port")")"
  mysql_host="$(trim "$(ini_get "$cfg_file" "Mysql" "Host")")"
  mysql_port="$(trim "$(ini_get "$cfg_file" "Mysql" "Port")")"
  redis_host="${redis_host:-127.0.0.1}"
  redis_port="${redis_port:-6379}"
  mysql_host="${mysql_host:-127.0.0.1}"
  mysql_port="${mysql_port:-3306}"

  if is_port_open "$redis_host" "$redis_port"; then
    echo "[UP]   Redis (${redis_host}:${redis_port})"
  else
    echo "[DOWN] Redis (${redis_host}:${redis_port})"
  fi

  if is_port_open "$mysql_host" "$mysql_port"; then
    echo "[UP]   MySQL (${mysql_host}:${mysql_port})"
  else
    echo "[DOWN] MySQL (${mysql_host}:${mysql_port})"
  fi

  # varify status
  pf="$(pid_file "$VARIFY_SERVICE")"
  if is_running "$VARIFY_SERVICE"; then
    echo "[UP]   ${VARIFY_SERVICE} (pid $(cat "$pf"))"
  elif varify_ready; then
    echo "[UP]   ${VARIFY_SERVICE} (port reachable, no pid file)"
  else
    echo "[DOWN] ${VARIFY_SERVICE}"
  fi

  # cpp services
  for svc in "${CPP_SERVICES[@]}"; do
    pf="$(pid_file "$svc")"
    if is_running "$svc"; then
      echo "[UP]   ${svc} (pid $(cat "$pf"))"
    else
      echo "[DOWN] ${svc}"
    fi
  done
}

show_logs() {
  local svc="${1:-}"
  if [[ -z "$svc" ]]; then
    echo "Usage: $(basename "$0") logs <ServiceName>"
    echo "Services: ${VARIFY_SERVICE} ${CPP_SERVICES[*]}"
    exit 1
  fi
  local lf
  lf="$(log_file "$svc")"
  if [[ ! -f "$lf" ]]; then
    echo "No log file for $svc: $lf"
    exit 1
  fi
  tail -n 120 -f "$lf"
}

start_all() {
  ensure_dirs
  check_bins_exist
  sync_runtime_configs

  echo "[INFO] build dir: $BUILD_DIR"
  echo "[INFO] ensure dependencies: Redis + MySQL"
  ensure_redis_mysql

  echo "[INFO] start VarifyServer"
  start_varify

  echo "[INFO] start order: ${CPP_SERVICES[*]}"
  local svc
  for svc in "${CPP_SERVICES[@]}"; do
    start_one_cpp "$svc"
  done

  echo "[DONE] all services started"
}

stop_all() {
  ensure_dirs

  local idx
  for ((idx=${#CPP_SERVICES[@]}-1; idx>=0; idx--)); do
    stop_one_cpp "${CPP_SERVICES[idx]}"
  done
  stop_varify

  echo "[DONE] all services stopped"
}

case "$CMD" in
  start)
    start_all
    ;;
  stop)
    stop_all
    ;;
  restart)
    stop_all
    start_all
    ;;
  status)
    status_all
    ;;
  logs)
    show_logs "${2:-}"
    ;;
  *)
    usage
    exit 1
    ;;
esac
