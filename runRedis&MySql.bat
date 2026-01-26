@echo off

REM 切换到脚本所在目录（myChat）
cd /d %~dp0

echo === Starting Redis ===
cd redis\Redis-x64-5.0.14.1
start redis-server.exe redis.windows.conf

echo === Starting MySQL in WSL ===
start wsl -e bash -c "echo 020222 | sudo -S service mysql start; exec bash"

echo === Starting VarifyServer (npm run serve) ===
cd %~dp0VarifyServer
start cmd /k npm run serve

echo === All services started ===
