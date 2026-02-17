#pragma once

#include <functional>

enum ErrorCodes {
    Success = 0,
    Error_Json = 1001,
    RPCFailed = 1002,
    VarifyExpired = 1003,
    VarifyCodeErr = 1004,
    UserExist = 1005,
    PasswdErr = 1006,
    EmailNotMatch = 1007,
    PasswdUpFailed = 1008,
    PasswdInvalid = 1009,
    TokenInvalid = 1010,
    UidInvalid = 1011,
    CREATE_CHAT_FAILED = 1012,
    LOAD_CHAT_FAILED = 1013,
    TODO_PARSE_FAILED = 1014,
    TODO_SAVE_FAILED = 1015,
    TODO_LOAD_FAILED = 1016,
};

class Defer {
public:
    explicit Defer(std::function<void()> func) : func_(std::move(func)) {}
    ~Defer() { func_(); }

private:
    std::function<void()> func_;
};

#define MAX_LENGTH  1024*2
#define HEAD_TOTAL_LEN 4
#define HEAD_ID_LEN 2
#define HEAD_DATA_LEN 2
#define MAX_RECVQUE  10000
#define MAX_SENDQUE 1000

enum MSG_IDS {
    MSG_CHAT_LOGIN = 1005,
    MSG_CHAT_LOGIN_RSP = 1006,
    ID_SEARCH_USER_REQ = 1007,
    ID_SEARCH_USER_RSP = 1008,
    ID_ADD_FRIEND_REQ = 1009,
    ID_ADD_FRIEND_RSP = 1010,
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,
    ID_AUTH_FRIEND_REQ = 1013,
    ID_AUTH_FRIEND_RSP = 1014,
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015,
    ID_TEXT_CHAT_MSG_REQ = 1017,
    ID_TEXT_CHAT_MSG_RSP = 1018,
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019,
    ID_NOTIFY_OFF_LINE_REQ = 1021,
    ID_HEART_BEAT_REQ = 1023,
    ID_HEARTBEAT_RSP = 1024,
    ID_LOAD_CHAT_THREAD_REQ = 1025,
    ID_LOAD_CHAT_THREAD_RSP = 1026,
    ID_CREATE_PRIVATE_CHAT_REQ = 1027,
    ID_CREATE_PRIVATE_CHAT_RSP = 1028,
    ID_LOAD_CHAT_MSG_REQ = 1029,
    ID_LOAD_CHAT_MSG_RSP = 1030,
    ID_IMG_CHAT_MSG_REQ = 1035,
    ID_IMG_CHAT_MSG_RSP = 1036,
    ID_NOTIFY_IMG_CHAT_MSG_REQ = 1039,
    ID_FILE_INFO_SYNC_REQ = 1041,
    ID_FILE_INFO_SYNC_RSP = 1042,
    ID_PARSE_TODO_REQ = 1049,
    ID_PARSE_TODO_RSP = 1050,
    ID_CREATE_TODO_REQ = 1051,
    ID_CREATE_TODO_RSP = 1052,
    ID_LIST_TODO_REQ = 1053,
    ID_LIST_TODO_RSP = 1054,
    ID_UPDATE_TODO_REQ = 1055,
    ID_UPDATE_TODO_RSP = 1056,
    ID_DELETE_TODO_REQ = 1057,
    ID_DELETE_TODO_RSP = 1058,
    ID_SET_TODO_STATUS_REQ = 1059,
    ID_SET_TODO_STATUS_RSP = 1060,
};

#define USERIPPREFIX  "uip_"
#define USERTOKENPREFIX  "utoken_"
#define IPCOUNTPREFIX  "ipcount_"
#define USER_BASE_INFO "ubaseinfo_"
#define LOGIN_COUNT  "logincount"
#define NAME_INFO  "nameinfo_"
#define LOCK_PREFIX "lock_"
#define USER_SESSION_PREFIX "usession_"
#define LOCK_COUNT "lockcount"

#define LOCK_TIME_OUT 10
#define ACQUIRE_TIME_OUT 5

enum MsgStatus {
    UN_READ = 0,
    SEND_FAILED = 1,
    READED = 2,
    UN_UPLOAD = 3
};

constexpr int LLM_BOT_UID = 900000001;
