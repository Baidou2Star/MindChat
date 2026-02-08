#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

CONFIG_FILES=(
  "GateServerWin/config.ini"
  "ChatServer/config.ini"
  "ChatServer2/config.ini"
  "StatusServer/config.ini"
  "ResourceServer/config.ini"
)

usage() {
  cat <<'EOF'
用法:
  scripts/switch_db.sh <db_name> [--restart] [build_dir]
  scripts/switch_db.sh                # 交互输入 db_name

示例:
  scripts/switch_db.sh stress_mysql
  scripts/switch_db.sh myChat --restart build
EOF
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

DB_NAME="${1:-}"
RESTART="false"
BUILD_DIR="build"

if [[ -z "${DB_NAME}" ]]; then
  read -r -p "请输入数据库名 (Schema): " DB_NAME
fi

if [[ -z "${DB_NAME}" ]]; then
  echo "[ERROR] 数据库名不能为空"
  exit 1
fi

shift_count=0
if [[ "${1:-}" != "" ]]; then
  shift_count=1
fi
if [[ "${shift_count}" -gt 0 ]]; then
  shift "${shift_count}"
fi

if [[ "${1:-}" == "--restart" ]]; then
  RESTART="true"
  shift
fi

if [[ -n "${1:-}" ]]; then
  BUILD_DIR="$1"
fi

echo "[INFO] 切换 Schema -> ${DB_NAME}"
changed=0

for rel in "${CONFIG_FILES[@]}"; do
  file="${ROOT_DIR}/${rel}"
  if [[ ! -f "${file}" ]]; then
    echo "[WARN] 跳过不存在文件: ${rel}"
    continue
  fi

  old_schema="$(awk -F'=' '/^[[:space:]]*Schema[[:space:]]*=/{gsub(/^[ \t]+|[ \t]+$/, "", $2); print $2; exit}' "${file}")"
  sed -i -E "s|^[[:space:]]*Schema[[:space:]]*=.*$|Schema = ${DB_NAME}|" "${file}"
  new_schema="$(awk -F'=' '/^[[:space:]]*Schema[[:space:]]*=/{gsub(/^[ \t]+|[ \t]+$/, "", $2); print $2; exit}' "${file}")"

  if [[ "${new_schema}" == "${DB_NAME}" ]]; then
    echo "[OK] ${rel}: ${old_schema:-<none>} -> ${new_schema}"
    changed=$((changed + 1))
  else
    echo "[ERROR] 修改失败: ${rel}"
    exit 1
  fi
done

echo "[INFO] 已完成，成功更新 ${changed} 个配置文件"

if [[ "${RESTART}" == "true" ]]; then
  echo "[INFO] 重启服务: build_dir=${BUILD_DIR}"
  bash "${ROOT_DIR}/scripts/manage_services.sh" restart "${BUILD_DIR}"
fi

