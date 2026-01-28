#include "MysqlDao.h"
#include "ConfigMgr.h"

#include <iostream>
#include <chrono>
#include <algorithm>

static int64_t GetNowTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

/* ================= MySqlPool ================= */

MySqlPool::MySqlPool(const std::string& host, int port, const std::string& user,
    const std::string& pass, const std::string& schema, int poolSize)
    : host_(host), port_(port), user_(user), pass_(pass), schema_(schema), stop_(false) {

    for (int i = 0; i < poolSize; ++i) {
        try {
            mysqlx::Session sess(
                mysqlx::SessionOption::HOST, host_,
                mysqlx::SessionOption::PORT, port_,
                mysqlx::SessionOption::USER, user_,
                mysqlx::SessionOption::PWD, pass_,
                mysqlx::SessionOption::DB, schema_
            );
            pool_.push(std::make_unique<XSqlConnection>(std::move(sess), GetNowTimestamp()));
        }
        catch (const mysqlx::Error& e) {
            std::cerr << "MySQL Pool Init Failed: " << e.what() << std::endl;
        }
    }

    // 后台保活线程：不 detach，析构时 join，避免 UAF
    check_thread_ = std::thread([this]() {
        while (!stop_.load(std::memory_order_acquire)) {
            for (int i = 0; i < 60; ++i) {
                if (stop_.load(std::memory_order_acquire)) return;
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            keepAlive();
        }
        });
}

MySqlPool::~MySqlPool() {
    Close();
    if (check_thread_.joinable()) {
        check_thread_.join();
    }
}

void MySqlPool::keepAlive() {
    // 非阻塞尝试拿一个连接，避免池空时卡死
    std::unique_ptr<XSqlConnection> con;
    {
        std::unique_lock<std::mutex> lk(mutex_, std::try_to_lock);
        if (!lk || stop_.load(std::memory_order_acquire) || pool_.empty()) return;

        con = std::move(pool_.front());
        pool_.pop();
    }

    try {
        con->session.sql("SELECT 1").execute();
        con->last_oper_time = GetNowTimestamp();
    }
    catch (...) {
        // 保活失败可按需做重连策略；这里保持“失败吞掉、归还连接”的简单策略
    }

    returnConnection(std::move(con));
}

std::unique_ptr<XSqlConnection> MySqlPool::getConnection() {
    std::unique_lock<std::mutex> lk(mutex_);
    cond_.wait(lk, [this]() {
        return stop_.load(std::memory_order_acquire) || !pool_.empty();
        });

    if (stop_.load(std::memory_order_acquire)) return nullptr;

    auto con = std::move(pool_.front());
    pool_.pop();
    return con;
}

void MySqlPool::returnConnection(std::unique_ptr<XSqlConnection> con) {
    if (!con) return;

    std::lock_guard<std::mutex> lk(mutex_);
    if (stop_.load(std::memory_order_acquire)) return;

    pool_.push(std::move(con));
    cond_.notify_one();
}

void MySqlPool::Close() {
    bool expected = false;
    if (!stop_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        // already closed
        return;
    }

    cond_.notify_all();

    // 清空池（释放 session）
    std::lock_guard<std::mutex> lk(mutex_);
    while (!pool_.empty()) pool_.pop();
}

/* ================= RAII Guards（自动归还连接 + 事务异常安全） ================= */

namespace {

    struct ConnGuard {
        MySqlPool* pool{ nullptr };
        std::unique_ptr<XSqlConnection> con;

        ConnGuard(MySqlPool* p, std::unique_ptr<XSqlConnection>&& c)
            : pool(p), con(std::move(c)) {
        }

        ConnGuard(const ConnGuard&) = delete;
        ConnGuard& operator=(const ConnGuard&) = delete;

        ~ConnGuard() {
            if (pool && con) pool->returnConnection(std::move(con));
        }

        XSqlConnection* operator->() { return con.get(); }
        explicit operator bool() const { return (bool)con; }
    };

    struct TxGuard {
        mysqlx::Session* sess{ nullptr };
        bool committed{ false };

        explicit TxGuard(mysqlx::Session& s) : sess(&s) {
            sess->startTransaction();
        }

        TxGuard(const TxGuard&) = delete;
        TxGuard& operator=(const TxGuard&) = delete;

        void commit() {
            sess->commit();
            committed = true;
        }

        ~TxGuard() {
            if (!sess) return;
            if (!committed) {
                try { sess->rollback(); }
                catch (...) {}
            }
        }
    };

} // namespace

/* ================= MysqlDao ================= */

MysqlDao::MysqlDao() {
    auto& cfg = ConfigMgr::Inst();
    pool_.reset(new MySqlPool(
        cfg["Mysql"]["Host"],
        std::stoi(cfg["Mysql"]["Port"]),
        cfg["Mysql"]["User"],
        cfg["Mysql"]["Passwd"],
        cfg["Mysql"]["Schema"],
        5
    ));
}

MysqlDao::~MysqlDao() {
    if (pool_) pool_->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return -1;

    try {
        cg->session.sql("CALL reg_user(?,?,?,@result)")
            .bind(name, email, pwd).execute();

        auto row = cg->session.sql("SELECT @result").execute().fetchOne();
        return row ? row[0].get<int>() : -1;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "[RegUser] Error: " << e.what() << std::endl;
        return -1;
    }
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        auto row = cg->session.sql("SELECT email FROM user WHERE name = ?")
            .bind(name).execute().fetchOne();
        return (row && row[0].get<std::string>() == email);
    }
    catch (...) {
        return false;
    }
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        cg->session.sql("UPDATE user SET pwd = ? WHERE name = ?")
            .bind(newpwd, name).execute();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MysqlDao::CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        auto row = cg->session.sql("SELECT uid, name, email, pwd FROM user WHERE name = ?")
            .bind(name).execute().fetchOne();

        if (!row || row[3].get<std::string>() != pwd) return false;

        userInfo.uid = row[0].get<int>();
        userInfo.name = row[1].get<std::string>();
        userInfo.email = row[2].get<std::string>();
        userInfo.pwd = row[3].get<std::string>();
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MysqlDao::AddFriendApply(const int& from, const int& to, const std::string& desc, const std::string& back_name) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        cg->session.sql(
            "INSERT INTO friend_apply (from_uid, to_uid, descs, back_name) VALUES (?,?,?,?) "
            "ON DUPLICATE KEY UPDATE descs = ?, back_name = ?"
        ).bind(from, to, desc, back_name, desc, back_name).execute();
        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "[AddFriendApply] Error: " << e.what() << std::endl;
        return false;
    }
}

bool MysqlDao::AuthFriendApply(const int& from, const int& to) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        // 修复：你原代码这里 bind(to, from) 是错的
        auto res = cg->session.sql("UPDATE friend_apply SET status = 1 WHERE from_uid = ? AND to_uid = ?")
            .bind(from, to).execute();
        return res.getAffectedItemsCount() > 0;
    }
    catch (...) {
        return false;
    }
}

bool MysqlDao::AddFriend(const int& from, const int& to, std::string back_name,
    std::vector<std::shared_ptr<AddFriendMsg>>& chat_datas) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        TxGuard tx(cg->session);

        // 1) 读取申请信息（锁行）
        auto row_apply = cg->session.sql(
            "SELECT back_name, descs FROM friend_apply WHERE from_uid = ? AND to_uid = ? FOR UPDATE"
        ).bind(to, from).execute().fetchOne();

        if (!row_apply) return false;

        std::string reverse_back = row_apply[0].get<std::string>();
        std::string apply_desc = row_apply[1].get<std::string>();

        // 2) 更新状态
        cg->session.sql("UPDATE friend_apply SET status = 1 WHERE from_uid = ? AND to_uid = ?")
            .bind(to, from).execute();

        // 3) 建立好友关系
        cg->session.sql("INSERT IGNORE INTO friend(self_id, friend_id, back) VALUES (?, ?, ?)")
            .bind(from, to, back_name).execute();
        cg->session.sql("INSERT IGNORE INTO friend(self_id, friend_id, back) VALUES (?, ?, ?)")
            .bind(to, from, reverse_back).execute();

        // 4) 创建会话
        cg->session.sql("INSERT INTO chat_thread (type, created_at) VALUES ('private', NOW())").execute();
        auto res_id = cg->session.sql("SELECT LAST_INSERT_ID()").execute().fetchOne();
        uint64_t threadId = res_id[0].get<uint64_t>();

        cg->session.sql("INSERT INTO private_chat(thread_id, user1_id, user2_id) VALUES (?, ?, ?)")
            .bind(threadId, std::min(from, to), std::max(from, to)).execute();

        // 5) 插入消息
        auto insert_msg = [&](int sender, int recv, const std::string& content) {
            cg->session.sql(
                "INSERT INTO chat_message(thread_id, sender_id, recv_id, content, created_at, status) "
                "VALUES (?, ?, ?, ?, NOW(), 2)"
            ).bind(threadId, sender, recv, content).execute();

            auto mid_row = cg->session.sql("SELECT LAST_INSERT_ID()").execute().fetchOne();

            auto msg = std::make_shared<AddFriendMsg>();
            msg->set_sender_id(sender);
            msg->set_msg_id(mid_row[0].get<uint64_t>());
            msg->set_msgcontent(content);
            msg->set_thread_id(threadId);
            msg->set_status(2);
            chat_datas.push_back(msg);
            };

        if (!apply_desc.empty()) insert_msg(to, from, apply_desc);
        insert_msg(from, to, "We are friends now!");

        tx.commit();
        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "[AddFriend] Error: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        return false;
    }
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(int uid) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return nullptr;

    try {
        auto row = cg->session.sql(
            "SELECT uid, name, email, pwd, nick, `desc`, sex, icon FROM user WHERE uid = ?"
        ).bind(uid).execute().fetchOne();

        if (!row) return nullptr;

        auto user = std::make_shared<UserInfo>();
        user->uid = row[0].get<int>();
        user->name = row[1].get<std::string>();
        user->email = row[2].get<std::string>();
        user->pwd = row[3].get<std::string>();
        user->nick = row[4].get<std::string>();
        user->desc = row[5].get<std::string>();
        user->sex = row[6].get<int>();
        user->icon = row[7].get<std::string>();
        return user;
    }
    catch (...) {
        return nullptr;
    }
}

std::shared_ptr<UserInfo> MysqlDao::GetUser(std::string name) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return nullptr;

    try {
        auto row = cg->session.sql(
            "SELECT uid, name, email, pwd, nick, `desc`, sex, icon FROM user WHERE name = ?"
        ).bind(name).execute().fetchOne();

        if (!row) return nullptr;

        auto user = std::make_shared<UserInfo>();
        user->uid = row[0].get<int>();
        user->name = row[1].get<std::string>();
        user->email = row[2].get<std::string>();
        user->pwd = row[3].get<std::string>();
        user->nick = row[4].get<std::string>();
        user->desc = row[5].get<std::string>();
        user->sex = row[6].get<int>();
        user->icon = row[7].get<std::string>();
        return user;
    }
    catch (...) {
        return nullptr;
    }
}

bool MysqlDao::GetApplyList(int touid, std::vector<std::shared_ptr<ApplyInfo>>& applyList, int begin, int limit) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        auto res = cg->session.sql(
            "SELECT a.from_uid, a.status, u.name, u.nick, u.sex "
            "FROM friend_apply a JOIN user u ON a.from_uid = u.uid "
            "WHERE a.to_uid = ? AND a.id > ? ORDER BY a.id ASC LIMIT ?"
        ).bind(touid, begin, limit).execute();

        std::vector<mysqlx::Row> rows = res.fetchAll();
        for (auto& row : rows) {
            applyList.push_back(std::make_shared<ApplyInfo>(
                row[0].get<int>(),                 // from_uid
                row[2].get<std::string>(),         // name
                "", "",                             // 你原结构里的占位字段
                row[3].get<std::string>(),         // nick
                row[4].get<int>(),                 // sex
                row[1].get<int>()                  // status
            ));
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MysqlDao::GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_info_list) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        // 优化：一次 JOIN 拉全，避免 N+1
        auto res = cg->session.sql(
            "SELECT u.uid, u.name, u.email, u.pwd, u.nick, u.`desc`, u.sex, u.icon, f.back "
            "FROM friend f JOIN user u ON f.friend_id = u.uid "
            "WHERE f.self_id = ?"
        ).bind(self_id).execute();

        std::vector<mysqlx::Row> rows = res.fetchAll();
        for (auto& row : rows) {
            auto user = std::make_shared<UserInfo>();
            user->uid = row[0].get<int>();
            user->name = row[1].get<std::string>();
            user->email = row[2].get<std::string>();
            user->pwd = row[3].get<std::string>();
            user->nick = row[4].get<std::string>();
            user->desc = row[5].get<std::string>();
            user->sex = row[6].get<int>();
            user->icon = row[7].get<std::string>();

            std::string back = row[8].get<std::string>();
            user->back = back.empty() ? user->name : back;

            user_info_list.push_back(user);
        }
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MysqlDao::GetUserThreads(int64_t userId, int64_t lastId, int pageSize,
    std::vector<std::shared_ptr<ChatThreadInfo>>& threads, bool& loadMore, int& nextLastId) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        std::string sql =
            "WITH all_threads AS ( "
            "  SELECT thread_id, 'private' AS type, user1_id, user2_id FROM private_chat "
            "  WHERE (user1_id = ? OR user2_id = ?) AND thread_id > ? "
            "  UNION ALL "
            "  SELECT thread_id, 'group' AS type, 0, 0 FROM group_chat_member "
            "  WHERE user_id = ? AND thread_id > ? "
            ") SELECT * FROM all_threads ORDER BY thread_id LIMIT ?";

        auto res = cg->session.sql(sql)
            .bind(userId, userId, lastId, userId, lastId, pageSize + 1)
            .execute();

        std::vector<mysqlx::Row> rows = res.fetchAll();

        loadMore = (rows.size() > (size_t)pageSize);
        if (loadMore) rows.pop_back();

        for (auto& row : rows) {
            auto info = std::make_shared<ChatThreadInfo>();
            info->_thread_id = row[0].get<int64_t>();
            info->_type = row[1].get<std::string>();
            info->_user1_id = row[2].get<int64_t>();
            info->_user2_id = row[3].get<int64_t>();
            threads.push_back(info);
        }

        if (!threads.empty()) nextLastId = (int)threads.back()->_thread_id;
        return true;
    }
    catch (...) {
        return false;
    }
}

bool MysqlDao::CreatePrivateChat(int user1_id, int user2_id, int& thread_id) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    int u1 = std::min(user1_id, user2_id);
    int u2 = std::max(user1_id, user2_id);

    try {
        auto row = cg->session.sql("SELECT thread_id FROM private_chat WHERE user1_id = ? AND user2_id = ?")
            .bind(u1, u2).execute().fetchOne();

        if (row) {
            thread_id = row[0].get<int>();
            return true;
        }

        TxGuard tx(cg->session);

        cg->session.sql("INSERT INTO chat_thread (type, created_at) VALUES ('private', NOW())").execute();
        auto res_id = cg->session.sql("SELECT LAST_INSERT_ID()").execute().fetchOne();
        thread_id = res_id[0].get<int>();

        cg->session.sql("INSERT INTO private_chat (thread_id, user1_id, user2_id, created_at) VALUES (?, ?, ?, NOW())")
            .bind(thread_id, u1, u2).execute();

        tx.commit();
        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "[CreatePrivateChat] Error: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        return false;
    }
}

std::shared_ptr<PageResult> MysqlDao::LoadChatMsg(int thread_id, int last_message_id, int page_size) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return nullptr;

    try {
        auto page_res = std::make_shared<PageResult>();

        auto res = cg->session.sql(
            "SELECT message_id, thread_id, sender_id, recv_id, content, created_at, status "
            "FROM chat_message WHERE thread_id = ? AND message_id > ? "
            "ORDER BY message_id ASC LIMIT ?"
        ).bind(thread_id, last_message_id, page_size + 1).execute();

        std::vector<mysqlx::Row> rows = res.fetchAll();
        page_res->load_more = (rows.size() > (size_t)page_size);
        if (page_res->load_more) rows.pop_back();

        for (auto& row : rows) {
            ChatMessage msg;
            msg.message_id = row[0].get<uint64_t>();
            msg.thread_id = row[1].get<uint64_t>();
            msg.sender_id = row[2].get<uint64_t>();
            msg.recv_id = row[3].get<uint64_t>();
            msg.content = row[4].get<std::string>();
            msg.chat_time = row[5].get<std::string>();
            msg.status = row[6].get<int>();
            page_res->messages.push_back(std::move(msg));
        }

        if (!page_res->messages.empty()) {
            page_res->next_cursor = (int)page_res->messages.back().message_id;
        }
        return page_res;
    }
    catch (...) {
        return nullptr;
    }
}

bool MysqlDao::AddChatMsg(std::vector<std::shared_ptr<ChatMessage>>& chat_datas) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        TxGuard tx(cg->session);

        for (auto& msg : chat_datas) {
            cg->session.sql(
                "INSERT INTO chat_message (thread_id, sender_id, recv_id, content, created_at, status) "
                "VALUES (?, ?, ?, ?, ?, ?)"
            ).bind(msg->thread_id, msg->sender_id, msg->recv_id, msg->content, msg->chat_time, msg->status)
                .execute();

            auto mid = cg->session.sql("SELECT LAST_INSERT_ID()").execute().fetchOne();
            msg->message_id = mid[0].get<uint64_t>();
        }

        tx.commit();
        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "[AddChatMsg] Error: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        return false;
    }
}

bool MysqlDao::AddChatMsg(std::shared_ptr<ChatMessage> chat_data) {
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        cg->session.sql(
            "INSERT INTO chat_message (thread_id, sender_id, recv_id, content, created_at, status) "
            "VALUES (?, ?, ?, ?, ?, ?)"
        ).bind(chat_data->thread_id, chat_data->sender_id, chat_data->recv_id,
            chat_data->content, chat_data->chat_time, chat_data->status)
            .execute();

        auto mid = cg->session.sql("SELECT LAST_INSERT_ID()").execute().fetchOne();
        chat_data->message_id = mid[0].get<uint64_t>();
        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "[AddChatMsg Single] Error: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        return false;
    }
}

bool MysqlDao::UpdateHeadInfo(int uid, const std::string& icon)
{
    ConnGuard cg(pool_.get(), pool_->getConnection());
    if (!cg) return false;

    try {
        auto res = cg->session.sql(
            "UPDATE user SET icon = ? WHERE uid = ?"
        ).bind(icon, uid).execute();

        // 检查是否更新成功（是否存在该 uid）
        if (res.getAffectedItemsCount() == 0) {
            std::cerr << "No user found with uid: " << uid << std::endl;
            return false;
        }

        return true;
    }
    catch (const mysqlx::Error& e) {
        std::cerr << "MySQL Error in UpdateHeadInfo: " << e.what() << std::endl;
        return false;
    }
    catch (...) {
        return false;
    }
}
