#include "MysqlDao.h"
#include "ConfigMgr.h"
#include <iostream>

static int64_t now_sec() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

/* ================= MySqlPool ================= */

MySqlPool::MySqlPool(const std::string& host, int port,
    const std::string& user,
    const std::string& pass,
    const std::string& schema,
    int poolSize)
    : host_(host),
    port_(port),
    user_(user),
    pass_(pass),
    schema_(schema),
    stop_(false)
{
    for (int i = 0; i < poolSize; ++i) {
        mysqlx::Session sess(
            mysqlx::SessionOption::HOST, host_,
            mysqlx::SessionOption::PORT, port_,
            mysqlx::SessionOption::USER, user_,
            mysqlx::SessionOption::PWD, pass_,
            mysqlx::SessionOption::DB, schema_
        );
        pool_.push(std::make_unique<XSqlConnection>(std::move(sess), now_sec()));
    }

    check_thread_ = std::thread([this]() {
        while (!stop_) {
            keepAlive();
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
        });
    check_thread_.detach();
}

MySqlPool::~MySqlPool() {
    Close();
}

void MySqlPool::keepAlive() {
    std::unique_ptr<XSqlConnection> con;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (pool_.empty()) return;
        con = std::move(pool_.front());
        pool_.pop();
    }

    try {
        con->session.sql("SELECT 1").execute();
        con->last_oper_time = now_sec();
    }
    catch (...) {
        // 忽略，下一次重新建连接
    }

    returnConnection(std::move(con));
}

std::unique_ptr<XSqlConnection> MySqlPool::getConnection() {
    std::unique_lock<std::mutex> lk(mutex_);
    cond_.wait(lk, [this]() { return stop_ || !pool_.empty(); });
    if (stop_) return nullptr;
    auto con = std::move(pool_.front());
    pool_.pop();
    return con;
}

void MySqlPool::returnConnection(std::unique_ptr<XSqlConnection> con) {
    std::lock_guard<std::mutex> lk(mutex_);
    if (!stop_) {
        pool_.push(std::move(con));
        cond_.notify_one();
    }
}

void MySqlPool::Close() {
    stop_ = true;
    cond_.notify_all();
}

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
    pool_->Close();
}

/* -------- RegUser -------- */

int MysqlDao::RegUser(const std::string& name,
    const std::string& email,
    const std::string& pwd)
{
    auto con = pool_->getConnection();
    if (!con) return -1;

    try {
        con->session.sql("CALL reg_user(?,?,?,@result)")
            .bind(name, email, pwd)
            .execute();

        auto row = con->session.sql("SELECT @result")
            .execute().fetchOne();

        pool_->returnConnection(std::move(con));
        return row ? row[0].get<int>() : -1;
    }
    catch (...) {
        pool_->returnConnection(std::move(con));
        return -1;
    }
}

/* -------- RegUserTransaction -------- */

int MysqlDao::RegUserTransaction(const std::string& name,
    const std::string& email,
    const std::string& pwd,
    const std::string& icon)
{
    auto con = pool_->getConnection();
    if (!con) return -1;

    try {
        con->session.startTransaction();

        if (con->session.sql(
            "SELECT 1 FROM user WHERE email=?")
            .bind(email).execute().fetchOne()) {
            con->session.rollback();
            pool_->returnConnection(std::move(con));
            return 0;
        }

        if (con->session.sql(
            "SELECT 1 FROM user WHERE name=?")
            .bind(name).execute().fetchOne()) {
            con->session.rollback();
            pool_->returnConnection(std::move(con));
            return 0;
        }

        con->session.sql("UPDATE user_id SET id=id+1").execute();

        auto row = con->session.sql("SELECT id FROM user_id")
            .execute().fetchOne();
        if (!row) {
            con->session.rollback();
            pool_->returnConnection(std::move(con));
            return -1;
        }

        int uid = row[0].get<int>();

        con->session.sql(
            "INSERT INTO user(uid,name,email,pwd,nick,icon)"
            " VALUES(?,?,?,?,?,?)")
            .bind(uid, name, email, pwd, name, icon)
            .execute();

        con->session.commit();
        pool_->returnConnection(std::move(con));
        return uid;
    }
    catch (...) {
        con->session.rollback();
        pool_->returnConnection(std::move(con));
        return -1;
    }
}

/* -------- CheckEmail -------- */

bool MysqlDao::CheckEmail(const std::string& name,
    const std::string& email)
{
    auto con = pool_->getConnection();
    if (!con) return false;

    auto row = con->session.sql(
        "SELECT email FROM user WHERE name=?")
        .bind(name).execute().fetchOne();

    pool_->returnConnection(std::move(con));
    return row && row[0].get<std::string>() == email;
}

/* -------- UpdatePwd -------- */

bool MysqlDao::UpdatePwd(const std::string& name,
    const std::string& newpwd)
{
    auto con = pool_->getConnection();
    if (!con) return false;

    con->session.sql(
        "UPDATE user SET pwd=? WHERE name=?")
        .bind(newpwd, name)
        .execute();

    pool_->returnConnection(std::move(con));
    return true;
}

/* -------- CheckPwd -------- */

bool MysqlDao::CheckPwd(const std::string& email,
    const std::string& pwd,
    UserInfo& userInfo)
{
    auto con = pool_->getConnection();
    if (!con) return false;

    auto row = con->session.sql(
        "SELECT uid,name,email,pwd FROM user WHERE email=?")
        .bind(email).execute().fetchOne();

    if (!row || row[3].get<std::string>() != pwd) {
        pool_->returnConnection(std::move(con));
        return false;
    }

    userInfo.uid = row[0].get<int>();
    userInfo.name = row[1].get<std::string>();
    userInfo.email = row[2].get<std::string>();
    userInfo.pwd = pwd;

    pool_->returnConnection(std::move(con));
    return true;
}

/* -------- TestProcedure -------- */

bool MysqlDao::TestProcedure(const std::string& email,
    int& uid,
    std::string& name)
{
    auto con = pool_->getConnection();
    if (!con) return false;

    con->session.sql(
        "CALL test_procedure(?,@uid,@name)")
        .bind(email)
        .execute();

    auto r1 = con->session.sql("SELECT @uid")
        .execute().fetchOne();
    auto r2 = con->session.sql("SELECT @name")
        .execute().fetchOne();

    if (!r1 || !r2) {
        pool_->returnConnection(std::move(con));
        return false;
    }

    uid = r1[0].get<int>();
    name = r2[0].get<std::string>();

    pool_->returnConnection(std::move(con));
    return true;
}
