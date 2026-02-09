#pragma once
#include "const.h"
#include <mysqlx/xdevapi.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include "data.h"
#include "message.pb.h"
#include "FileInfo.h"
using message::AddFriendMsg;
using message::TextChatData;

// 包装 Session 和最后操作时间
class XSqlConnection {
public:
    XSqlConnection(mysqlx::Session&& s, int64_t t)
        : session(std::move(s)), last_oper_time(t) {
    }
    mysqlx::Session session;
    int64_t last_oper_time;
};

class MySqlPool {
public:
    MySqlPool(const std::string& host, int port, const std::string& user,
        const std::string& pass, const std::string& schema, int poolSize);
    ~MySqlPool();

    std::unique_ptr<XSqlConnection> getConnection();
    void returnConnection(std::unique_ptr<XSqlConnection> con);
    void Close();

private:
    void keepAlive();

    std::string host_, user_, pass_, schema_;
    int port_;
    std::queue<std::unique_ptr<XSqlConnection>> pool_;
    std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> stop_;
    std::thread check_thread_;
};

class MysqlDao {
public:
    MysqlDao();
    ~MysqlDao();

    int RegUser(const std::string& name, const std::string& email, const std::string& pwd);
    bool CheckEmail(const std::string& name, const std::string& email);
    bool UpdatePwd(const std::string& name, const std::string& newpwd);
    bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);

    bool AddFriendApply(const int& from, const int& to, const std::string& desc, const std::string& back_name);
    bool AuthFriendApply(const int& from, const int& to);
    bool AddFriend(const int& from, const int& to, std::string back_name, std::vector<std::shared_ptr<AddFriendMsg>>& chat_datas);

    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);
    bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit);
    bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info);

    bool GetUserThreads(int64_t userId, int64_t lastId, int pageSize,
        std::vector<std::shared_ptr<ChatThreadInfo>>& threads, bool& loadMore, int& nextLastId);
    bool CreatePrivateChat(int user1_id, int user2_id, int& thread_id);
    std::shared_ptr<PageResult> LoadChatMsg(int thread_id, int last_message_id, int page_size);
    bool AddChatMsg(std::vector<std::shared_ptr<ChatMessage>>& chat_datas);
    bool AddChatMsg(std::shared_ptr<ChatMessage> chat_data);
    bool UpdateHeadInfo(int uid, const std::string& icon);

    bool UpdateUploadStatus(int chat_message_id);
    std::shared_ptr<ChatImgInfo> GetImgInfoByMsgId(int message_id);
    std::shared_ptr<ChatMessage> GetChatMsgById(int message_id);

private:
    std::unique_ptr<MySqlPool> pool_;
};