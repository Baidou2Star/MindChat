/*
 * @Author: Baidou
 * @Date: 2026-01-12 09:54:38
 * @LastEditors: Baidou 1424089348@qq.com
 * @LastEditTime: 2026-01-12 11:41:07
 * @FilePath: \VarifyServer\proto.js
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
const path = require("path");
const grpc = require("@grpc/grpc-js");
const protoLoader = require("@grpc/proto-loader");

const PROTO_PATH = path.join(__dirname, "message.proto");

const packageDefinition = protoLoader.loadSync(PROTO_PATH, {
    keepCase: true,
    longs: String,
    enums: String,
    defaults: true,
    oneofs: true,
});

const protoDescriptor = grpc.loadPackageDefinition(packageDefinition);
const messageProto = protoDescriptor.message;

module.exports = messageProto;