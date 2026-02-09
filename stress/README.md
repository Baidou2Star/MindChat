# Stress Tests (GateServer)

This folder contains a simple load generator for the GateServer HTTP endpoints using PowerShell only (no extra dependencies).

## Prerequisites
- GateServer running on `http://127.0.0.1:8080`
- StatusServer, ChatServer, Redis, and MySQL running for `/user_login`
- If you want to test other endpoints, update `stress/scenarios.json` accordingly

## Run
```powershell
# From repo root
powershell -ExecutionPolicy Bypass -File stress\run.ps1 -Scenario login_mix
```

## Scenarios
Configured in `stress/scenarios.json`:
- `get_test`: hits `/get_test`
- `test_procedure`: hits `/test_procedure`
- `user_login`: login-only
- `login_mix`: weighted mix of login + ping

## Adjust Load
Edit `stages` in `stress/scenarios.json`:
- `durationSeconds`: time per stage
- `targetConcurrency`: number of concurrent workers

## Notes
- The `user_login` scenario uses sample accounts from the SQL backup. Update to match your DB.
- `/get_varifycode` and `/user_register` are not included because they require valid email verification codes.
