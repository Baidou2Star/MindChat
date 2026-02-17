#pragma once

#include <string>
#include <utility>
#include <vector>

struct UserInfo {
    UserInfo() : name(""), pwd(""), uid(0), email(""), nick(""), desc(""), sex(0), icon(""), back("") {}
    std::string name;
    std::string pwd;
    int uid;
    std::string email;
    std::string nick;
    std::string desc;
    int sex;
    std::string icon;
    std::string back;
};

struct ApplyInfo {
    ApplyInfo(int uid, std::string name, std::string desc,
              std::string icon, std::string nick, int sex, int status)
        : _uid(uid), _name(std::move(name)), _desc(std::move(desc)),
          _icon(std::move(icon)), _nick(std::move(nick)), _sex(sex), _status(status) {}

    int _uid;
    std::string _name;
    std::string _desc;
    std::string _icon;
    std::string _nick;
    int _sex;
    int _status;
};

struct ChatThreadInfo {
    int _thread_id = 0;
    std::string _type;
    int _user1_id = 0;
    int _user2_id = 0;
};

struct ChatMessage {
    int message_id = 0;
    int thread_id = 0;
    int sender_id = 0;
    int recv_id = 0;
    std::string unique_id;
    std::string content;
    std::string chat_time;
    int status = 0;
    int msg_type = 0;
};

struct PageResult {
    std::vector<ChatMessage> messages;
    bool load_more = false;
    int next_cursor = 0;
};

struct TodoItem {
    int todo_id = 0;
    int uid = 0;
    int source_thread_id = 0;
    int source_message_id = 0;
    std::string source_text;
    std::string title;
    std::string event_text;
    std::string location;
    std::string time_text;
    std::string start_time;
    std::string end_time;
    int status = 0;
    std::string created_at;
};

enum class ChatMsgType {
    TEXT = 0,
    PIC = 1,
    VIDEO = 2,
    FILE = 3
};
