#!/bin/bash
set -e
PROTOC="$(pwd)/build/vcpkg_installed/x64-linux/tools/protobuf/protoc"
PLUGIN="$(pwd)/build/vcpkg_installed/x64-linux/tools/grpc/grpc_cpp_plugin"
for d in ChatServer ChatServer2 GateServerWin StatusServer ResourceServer; do
  echo "regenerate $d/message.proto"
  "$PROTOC" -I "$d" --cpp_out="$d" --grpc_out="$d" --plugin=protoc-gen-grpc="$PLUGIN" "$d/message.proto"
done