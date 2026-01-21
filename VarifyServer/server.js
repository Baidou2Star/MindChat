/*
 * @Author: Baidou 1424089348@qq.com
 * @Date: 2026-01-12 10:48:36
 * @LastEditors: Baidou 1424089348@qq.com
 * @LastEditTime: 2026-01-15 22:08:48
 * @FilePath: \VarifyServer\server.js
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
const grpc = require("@grpc/grpc-js");
const messageProto = require('./proto');
const const_module = require('./const');
const { v4: uuidv4 } = require('uuid');
const emailModule = require('./email');
const redis_module = require('./redis')

// async function GetVarifyCode(call, callback) {
//     console.log("email is ", call.request.email)
//     try {
//         uniqueId = uuidv4();
//         console.log("uniqueId is ", uniqueId)
//         let text_str = '您的验证码为' + uniqueId + '请三分钟内完成注册'
//         //发送邮件
//         let mailOptions = {
//             from: 'sjj0202222@126.com',
//             to: call.request.email,
//             subject: '验证码',
//             text: text_str,
//         };

//         let send_res = await emailModule.SendMail(mailOptions);
//         console.log("send res is ", send_res)

//         callback(null, {
//             email: call.request.email,
//             error: const_module.Errors.Success
//         });


//     } catch (error) {
//         console.log("catch error is ", error)

//         callback(null, {
//             email: call.request.email,
//             error: const_module.Errors.Exception
//         });
//     }

// }

async function GetVarifyCode(call, callback) {
    console.log("email is ", call.request.email)
    try {
        let query_res = await redis_module.GetRedis(const_module.code_prefix + call.request.email);
        console.log("query_res is ", query_res)

        let uniqueId = query_res;
        let shouldGenerateNewCode = false;

        // 如果Redis中没有验证码，则需要生成新的
        if (query_res == null) {
            shouldGenerateNewCode = true;
        } else {
            // 验证码存在，但我们仍可能需要检查是否需要强制更新
            // 由于当前Redis实现可能不支持TTL查询，我们使用现有的逻辑
            // 如果你想实现时间控制，需要扩展redis_module来支持TTL命令
        }

        if (shouldGenerateNewCode) {
            uniqueId = uuidv4();
            if (uniqueId.length > 4) {
                uniqueId = uniqueId.substring(0, 4);
            }
            let bres = await redis_module.SetRedisExpire(const_module.code_prefix + call.request.email, uniqueId, 600)
            if (!bres) {
                callback(null, {
                    email: call.request.email,
                    error: const_module.Errors.RedisErr
                });
                return;
            }
        }

        console.log("uniqueId is ", uniqueId)
        let text_str = '您的验证码为' + uniqueId + '请三分钟内完成注册'

        let mailOptions = {
            from: 'sjj0202222@126.com',
            to: call.request.email,
            subject: '验证码',
            text: text_str,
        };

        let send_res = await emailModule.SendMail(mailOptions);
        console.log("send res is ", send_res)

        callback(null, {
            email: call.request.email,
            error: const_module.Errors.Success
        });

    } catch (error) {
        console.log("catch error is ", error)
        callback(null, {
            email: call.request.email,
            error: const_module.Errors.Exception
        });
    }
}

function main() {


    var server = new grpc.Server()
    server.addService(messageProto.VarifyService.service, { GetVarifyCode: GetVarifyCode })
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        server.start()
        console.log('grpc server started')
        console.log("添加redis的邮件服务启动")
    })
}

main()