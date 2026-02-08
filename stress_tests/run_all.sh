#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SCRIPT="${ROOT_DIR}/stress_tests/run_stress.py"

# 可通过环境变量覆盖
SCHEMA="${SCHEMA:-stress_mysql}"
SEED_USERS="${SEED_USERS:-20000}"
USER_PREFIX="${USER_PREFIX:-stress_u_}"
USER_START="${USER_START:-300000}"
PASSWORD="${PASSWORD:-abc123}"

echo "[INFO] Step1: prepare"
python3 "$SCRIPT" \
  --schema "$SCHEMA" \
  --user-prefix "$USER_PREFIX" \
  --user-start "$USER_START" \
  --password "$PASSWORD" \
  prepare \
  --seed-users "$SEED_USERS"

echo "[INFO] Step2: conn-limit"
python3 "$SCRIPT" conn-limit \
  --start-connections 1000 \
  --step-connections 1000 \
  --max-connections 30000 \
  --latency-threshold-ms 50

echo "[INFO] Step3: stability@1W"
python3 "$SCRIPT" stability \
  --connections 10000 \
  --duration-sec 600 \
  --latency-threshold-ms 10

echo "[INFO] Step4: pingpong-limit@10ms"
python3 "$SCRIPT" pingpong-limit \
  --start-connections 2000 \
  --step-connections 1000 \
  --max-connections 30000 \
  --ping-interval-ms 10 \
  --latency-threshold-ms 10

echo "[DONE] all stress stages complete"
