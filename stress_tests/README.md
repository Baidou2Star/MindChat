# myChat 压力测试脚本说明

脚本入口：`stress_tests/run_stress.py`

日志目录：默认写入 `logs/stress/<时间戳_模式>/`

- `events.log`：逐步事件日志（JSON 行）
- `summary.json`：该次压测的结构化结果

## 压测方案优化点（已实现）


1. **分阶段递增**：连接上限与 pingpong 上限都采用阶梯增压，避免一次性打爆后拿不到稳定点。
2. **判停条件明确**：按连接成功率、心跳成功率、P95 延迟和掉线率自动停测，并输出失败原因 Top 列表。
3. **账号预热自动化**：`prepare` 子命令可切库并调用 `stress_bulk_seed_users`（自动识别函数/过程参数个数后组 SQL）。
4. **WSL 场景提示**：启动时检查 `ulimit -n` 建议值，防止本机 fd 上限先卡住压测。
5. **轻量协议复现**：真实走 `GateServer /user_login` + `ChatServer` 二进制包登录 + 心跳回包，压测更接近实际链路。

## 使用步骤

> 先确保服务已启动（`scripts/manage_services.sh start`）

### 1) 预处理：切换压测库 + 造号

```bash
python3 stress_tests/run_stress.py \
  --schema stress_mysql \
  --user-prefix stress_u_ \
  --user-start 300000 \
  --password abc123 \
  prepare \
  --seed-users 20000
```

如果你想手工指定存储过程调用：

```bash
python3 stress_tests/run_stress.py \
  prepare \
  --seed-call "CALL stress_bulk_seed_users(300000,20000,'stress_u_','stress.local','abc123');"
```

### 2) 测试服务器稳定连接上限（分阶段）

```bash
python3 stress_tests/run_stress.py \
  --connect-batch-size 100 \
  --connect-batch-interval-ms 120 \
  --gate-login-retries 3 \
  --chat-connect-retries 3 \
  --retry-backoff-ms 200 \
  conn-limit \
  --start-connections 1000 \
  --step-connections 1000 \
  --max-connections 30000 \
  --latency-threshold-ms 50
```

### 3) 1W 连接稳定性（收发/心跳）

```bash
python3 stress_tests/run_stress.py stability \
  --connections 10000 \
  --duration-sec 600 \
  --latency-threshold-ms 10
```

### 4) 10ms pingpong 条件下连接上限

```bash
python3 stress_tests/run_stress.py pingpong-limit \
  --start-connections 2000 \
  --step-connections 1000 \
  --max-connections 30000 \
  --ping-interval-ms 10 \
  --latency-threshold-ms 10
```

## 常用参数

- `--gate-host/--gate-port`：Gate HTTP 地址。
- `--connect-batch-size`：建连并发；机器扛得住可适当调大。
- `--connect-batch-interval-ms`：建连批次间隔，压不住时建议设置 `80~300`。
- `--heartbeat-batch-size`：单轮心跳并发批次。
- `--gate-login-retries`：Gate 返回 `1002` 时重试次数。
- `--chat-connect-retries`：`chat_connect_oserror_99/timeout/reset` 时重试次数。
- `--retry-backoff-ms`：建连重试退避时间。
- `--extra-attempt-ratio`：补偿失败登录的额外账号比例。
- `--min-connect-ratio` / `--min-heartbeat-ratio`：判稳阈值。

> 注意：全局参数（如 `--connect-batch-size`、`--request-timeout`）要写在子命令（如 `conn-limit`）前面。

## WSL 建议

压测前建议提高 fd 上限（根据目标连接数调整）：

```bash
ulimit -n 65535
```

如果 MySQL/Redis/服务端与脚本都在同一台 WSL，优先使用 `127.0.0.1`，减少跨栈转发抖动。
