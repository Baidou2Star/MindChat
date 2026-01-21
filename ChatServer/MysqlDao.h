#pragma once
#include "const.h"
#include <mysqlx/xdevapi.h>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "data.h"

//struct UserInfo {
//    std::string name;
//    std::string pwd;
//    int uid;
//    std::string email;
//};

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
    MySqlPool(const std::string& host, int port,
        const std::string& user,
        const std::string& pass,
        const std::string& schema,
        int poolSize);
    ~MySqlPool();

    std::unique_ptr<XSqlConnection> getConnection();
    void returnConnection(std::unique_ptr<XSqlConnection> con);
    void Close();

private:
    void keepAlive();

private:
    std::string host_;
    int port_;
    std::string user_;
    std::string pass_;
    std::string schema_;

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

    int  RegUser(const std::string& name,
        const std::string& email,
        const std::string& pwd);

    int  RegUserTransaction(const std::string& name,
        const std::string& email,
        const std::string& pwd,
        const std::string& icon);

    bool CheckEmail(const std::string& name,
        const std::string& email);

    bool UpdatePwd(const std::string& name,
        const std::string& newpwd);

    bool CheckPwd(const std::string& email,
        const std::string& pwd,
        UserInfo& userInfo);

    bool TestProcedure(const std::string& email,
        int& uid,
        std::string& name);
    bool AddFriendApply(const int& from, const int& to);
    bool AuthFriendApply(const int& from, const int& to);
    bool AddFriend(const int& from, const int& to, std::string back_name);
    bool GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int offset, int limit);
    bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo> >& user_info);

    std::shared_ptr<UserInfo> GetUser(int uid);
    std::shared_ptr<UserInfo> GetUser(std::string name);

private:
    std::unique_ptr<MySqlPool> pool_;
};
