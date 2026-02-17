#include "schedulepage.h"
#include "ui_schedulepage.h"

#include "tcpmgr.h"
#include "usermgr.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QListWidgetItem>
#include <algorithm>

namespace {

QDateTime ParseTodoDateTime(const QString& value) {
    return QDateTime::fromString(value.trimmed(), "yyyy-MM-dd HH:mm:ss");
}

bool TodoLessByStartTime(const QJsonObject& lhs, const QJsonObject& rhs) {
    const QDateTime lhs_time = ParseTodoDateTime(lhs["start_time"].toString());
    const QDateTime rhs_time = ParseTodoDateTime(rhs["start_time"].toString());

    const bool lhs_valid = lhs_time.isValid();
    const bool rhs_valid = rhs_time.isValid();
    if (lhs_valid != rhs_valid) {
        return lhs_valid;
    }
    if (lhs_valid && rhs_valid && lhs_time != rhs_time) {
        return lhs_time < rhs_time;
    }

    const QDateTime lhs_created = ParseTodoDateTime(lhs["created_at"].toString());
    const QDateTime rhs_created = ParseTodoDateTime(rhs["created_at"].toString());
    if (lhs_created.isValid() && rhs_created.isValid() && lhs_created != rhs_created) {
        return lhs_created < rhs_created;
    }

    return lhs["todo_id"].toInt() < rhs["todo_id"].toInt();
}

QString BuildTodoSummary(const QJsonObject& todo) {
    QString time_text = todo["start_time"].toString().trimmed();
    if (time_text.isEmpty()) {
        time_text = todo["time_text"].toString().trimmed();
    }
    if (time_text.isEmpty()) {
        time_text = QStringLiteral("时间待定");
    }

    QString location = todo["location"].toString().trimmed();
    if (location.isEmpty()) {
        location = QStringLiteral("地点待定");
    }

    return QStringLiteral("%1 | %2").arg(time_text, location);
}

QJsonObject ParseTodoObject(const QListWidgetItem* item) {
    if (item == nullptr) {
        return {};
    }

    const QByteArray payload = item->data(Qt::UserRole).toByteArray();
    const QJsonDocument doc = QJsonDocument::fromJson(payload);
    if (!doc.isObject()) {
        return {};
    }
    return doc.object();
}

}  // namespace

SchedulePage::SchedulePage(QWidget* parent)
    : QWidget(parent),
      ui(new Ui::SchedulePage),
      _source_thread_id(0),
      _source_message_id(0),
      _current_todo_id(0),
      _current_todo_status(0),
      _busy(false) {
    ui->setupUi(this);
    ui->status_lb->setText(QStringLiteral("就绪"));
    BindTodoSignals();

    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_parse_todo_rsp,
            this, &SchedulePage::slot_parse_todo_rsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_create_todo_rsp,
            this, &SchedulePage::slot_create_todo_rsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_update_todo_rsp,
            this, &SchedulePage::slot_update_todo_rsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_delete_todo_rsp,
            this, &SchedulePage::slot_delete_todo_rsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_set_todo_status_rsp,
            this, &SchedulePage::slot_set_todo_status_rsp);
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_list_todo_rsp,
            this, &SchedulePage::slot_list_todo_rsp);

    RequestList();
}

SchedulePage::~SchedulePage() {
    delete ui;
}

void SchedulePage::LoadFromText(const QString& text, int source_thread_id, int source_message_id, bool auto_parse) {
    _source_text = text;
    _source_thread_id = source_thread_id;
    _source_message_id = source_message_id;
    _current_todo_id = 0;
    _current_todo_status = 0;

    ui->pending_todo_list->clearSelection();
    ui->done_todo_list->clearSelection();
    ui->raw_input_edit->setPlainText(text);
    ui->toggle_status_btn->setText(QStringLiteral("标记已完成"));
    ui->status_lb->setText(QStringLiteral("已接收消息文本，点击智能解析"));

    if (auto_parse) {
        RequestParse();
    }
}

void SchedulePage::RefreshTodoList() {
    RequestList();
}

void SchedulePage::on_parse_btn_clicked() {
    RequestParse();
}

void SchedulePage::on_save_btn_clicked() {
    if (_busy) {
        return;
    }
    if (_current_todo_id > 0) {
        ui->status_lb->setText(QStringLiteral("已选中历史待办，请点击“保存修改”或先点“新建待办”"));
        return;
    }

    QJsonObject req = BuildTodoPayload();
    req["status"] = 0;
    if (req["event"].toString().trimmed().isEmpty()) {
        ui->status_lb->setText(QStringLiteral("请先输入待办文本或事件内容"));
        return;
    }

    QJsonDocument doc(req);
    SetBusy(true);
    emit TcpMgr::GetInstance()->sig_send_data(ID_CREATE_TODO_REQ, doc.toJson(QJsonDocument::Compact));
    ui->status_lb->setText(QStringLiteral("正在保存待办..."));
}

void SchedulePage::on_update_btn_clicked() {
    if (_busy) {
        return;
    }
    if (_current_todo_id <= 0) {
        ui->status_lb->setText(QStringLiteral("请先从左侧选择一个待办后再修改"));
        return;
    }

    QJsonObject req = BuildTodoPayload();
    req["todo_id"] = _current_todo_id;
    req["status"] = _current_todo_status;
    if (req["event"].toString().trimmed().isEmpty()) {
        ui->status_lb->setText(QStringLiteral("事件内容不能为空"));
        return;
    }

    QJsonDocument doc(req);
    SetBusy(true);
    emit TcpMgr::GetInstance()->sig_send_data(ID_UPDATE_TODO_REQ, doc.toJson(QJsonDocument::Compact));
    ui->status_lb->setText(QStringLiteral("正在保存修改..."));
}

void SchedulePage::on_delete_btn_clicked() {
    if (_busy) {
        return;
    }
    if (_current_todo_id <= 0) {
        ui->status_lb->setText(QStringLiteral("请先选择要删除的待办"));
        return;
    }

    QJsonObject req;
    req["uid"] = UserMgr::GetInstance()->GetUid();
    req["todo_id"] = _current_todo_id;
    QJsonDocument doc(req);
    SetBusy(true);
    emit TcpMgr::GetInstance()->sig_send_data(ID_DELETE_TODO_REQ, doc.toJson(QJsonDocument::Compact));
    ui->status_lb->setText(QStringLiteral("正在删除待办..."));
}

void SchedulePage::on_toggle_status_btn_clicked() {
    if (_busy) {
        return;
    }
    if (_current_todo_id <= 0) {
        ui->status_lb->setText(QStringLiteral("请先选择一个待办"));
        return;
    }

    const int next_status = (_current_todo_status == 1) ? 0 : 1;
    QJsonObject req;
    req["uid"] = UserMgr::GetInstance()->GetUid();
    req["todo_id"] = _current_todo_id;
    req["status"] = next_status;

    QJsonDocument doc(req);
    SetBusy(true);
    emit TcpMgr::GetInstance()->sig_send_data(ID_SET_TODO_STATUS_REQ, doc.toJson(QJsonDocument::Compact));
    ui->status_lb->setText(next_status == 1 ? QStringLiteral("正在标记为已完成...") : QStringLiteral("正在恢复为未完成..."));
}

void SchedulePage::on_new_btn_clicked() {
    if (_busy) {
        return;
    }
    _source_thread_id = 0;
    _source_message_id = 0;
    _source_text.clear();
    ui->raw_input_edit->clear();
    ui->pending_todo_list->clearSelection();
    ui->done_todo_list->clearSelection();
    ClearEditorForNew();
    ui->status_lb->setText(QStringLiteral("已切换到新建模式"));
}

void SchedulePage::on_refresh_btn_clicked() {
    RequestList();
}

void SchedulePage::slot_parse_todo_rsp(QJsonObject rsp) {
    const int error = rsp["error"].toInt(1);
    if (error != 0) {
        ui->status_lb->setText(QStringLiteral("解析失败，可手动编辑后保存"));
    } else {
        ui->status_lb->setText(QStringLiteral("解析成功，可编辑确认后保存"));
    }

    _source_text = rsp["source_text"].toString(_source_text);
    _source_thread_id = rsp["source_thread_id"].toInt(_source_thread_id);
    _source_message_id = rsp["source_message_id"].toInt(_source_message_id);

    ui->title_edit->setText(rsp["title"].toString());
    ui->event_edit->setPlainText(rsp["event"].toString());
    ui->location_edit->setText(rsp["location"].toString());
    ui->time_text_edit->setText(rsp["time_text"].toString());
    ui->start_time_edit->setText(rsp["start_time"].toString());
    ui->end_time_edit->setText(rsp["end_time"].toString());
}

void SchedulePage::slot_create_todo_rsp(QJsonObject rsp) {
    SetBusy(false);
    const int error = rsp["error"].toInt(1);
    if (error != 0) {
        ui->status_lb->setText(QStringLiteral("保存失败，请检查字段后重试"));
        return;
    }

    _source_thread_id = 0;
    _source_message_id = 0;
    _source_text.clear();
    ui->raw_input_edit->clear();
    ClearEditorForNew();
    ui->status_lb->setText(QStringLiteral("保存成功，已添加到待办"));
    RequestList();
}

void SchedulePage::slot_update_todo_rsp(QJsonObject rsp) {
    SetBusy(false);
    if (rsp["error"].toInt(1) != 0) {
        ui->status_lb->setText(QStringLiteral("修改失败，请稍后重试"));
        return;
    }

    ui->status_lb->setText(QStringLiteral("修改成功"));
    RequestList();
}

void SchedulePage::slot_delete_todo_rsp(QJsonObject rsp) {
    SetBusy(false);
    if (rsp["error"].toInt(1) != 0) {
        ui->status_lb->setText(QStringLiteral("删除失败，请稍后重试"));
        return;
    }

    ClearEditorForNew();
    ui->status_lb->setText(QStringLiteral("删除成功"));
    RequestList();
}

void SchedulePage::slot_set_todo_status_rsp(QJsonObject rsp) {
    SetBusy(false);
    if (rsp["error"].toInt(1) != 0) {
        ui->status_lb->setText(QStringLiteral("状态更新失败，请稍后重试"));
        return;
    }

    const int status = rsp["status"].toInt(_current_todo_status);
    _current_todo_status = status;
    ui->toggle_status_btn->setText(status == 1 ? QStringLiteral("标记未完成") : QStringLiteral("标记已完成"));
    ui->status_lb->setText(status == 1 ? QStringLiteral("已标记为完成") : QStringLiteral("已恢复为未完成"));
    RequestList();
}

void SchedulePage::slot_list_todo_rsp(QJsonObject rsp) {
    ui->pending_todo_list->clear();
    ui->done_todo_list->clear();
    if (rsp["error"].toInt(1) != 0) {
        ui->status_lb->setText(QStringLiteral("加载待办失败"));
        return;
    }

    const QJsonArray rows = rsp["todos"].toArray();
    QList<QJsonObject> pending_todos;
    QList<QJsonObject> done_todos;
    pending_todos.reserve(rows.size());
    done_todos.reserve(rows.size());

    for (const QJsonValue& value : rows) {
        const QJsonObject obj = value.toObject();
        if (obj["status"].toInt() == 1) {
            done_todos.push_back(obj);
        } else {
            pending_todos.push_back(obj);
        }
    }

    std::sort(pending_todos.begin(), pending_todos.end(), TodoLessByStartTime);
    std::sort(done_todos.begin(), done_todos.end(), TodoLessByStartTime);
    FillTodoList(ui->pending_todo_list, pending_todos);
    FillTodoList(ui->done_todo_list, done_todos);

    ui->pending_group_lb->setText(QStringLiteral("未完成（%1）").arg(pending_todos.size()));
    ui->done_group_lb->setText(QStringLiteral("已完成（%1）").arg(done_todos.size()));
    ui->status_lb->setText(QStringLiteral("待办列表已刷新：%1 条").arg(rows.size()));
}

void SchedulePage::RequestParse() {
    const QString raw_text = ui->raw_input_edit->toPlainText().trimmed();
    if (raw_text.isEmpty()) {
        ui->status_lb->setText(QStringLiteral("请输入待解析文本"));
        return;
    }

    _source_text = raw_text;

    QJsonObject req;
    req["uid"] = UserMgr::GetInstance()->GetUid();
    req["source_text"] = raw_text;
    req["source_thread_id"] = _source_thread_id;
    req["source_message_id"] = _source_message_id;

    QJsonDocument doc(req);
    emit TcpMgr::GetInstance()->sig_send_data(ID_PARSE_TODO_REQ, doc.toJson(QJsonDocument::Compact));
    ui->status_lb->setText(QStringLiteral("正在解析..."));
}

void SchedulePage::RequestList() {
    QJsonObject req;
    req["uid"] = UserMgr::GetInstance()->GetUid();
    req["limit"] = 100;
    QJsonDocument doc(req);
    emit TcpMgr::GetInstance()->sig_send_data(ID_LIST_TODO_REQ, doc.toJson(QJsonDocument::Compact));
}

void SchedulePage::BindTodoSignals() {
    connect(ui->pending_todo_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        SelectTodoFromItem(ui->pending_todo_list, item);
    });
    connect(ui->done_todo_list, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        SelectTodoFromItem(ui->done_todo_list, item);
    });
}

void SchedulePage::LoadTodoToEditor(const QJsonObject& todo) {
    _current_todo_id = todo["todo_id"].toInt();
    _current_todo_status = todo["status"].toInt();
    _source_thread_id = todo["source_thread_id"].toInt();
    _source_message_id = todo["source_message_id"].toInt();
    _source_text = todo["source_text"].toString();

    ui->raw_input_edit->setPlainText(_source_text);
    ui->title_edit->setText(todo["title"].toString());
    ui->event_edit->setPlainText(todo["event"].toString());
    ui->location_edit->setText(todo["location"].toString());
    ui->time_text_edit->setText(todo["time_text"].toString());
    ui->start_time_edit->setText(todo["start_time"].toString());
    ui->end_time_edit->setText(todo["end_time"].toString());
    ui->toggle_status_btn->setText(_current_todo_status == 1 ? QStringLiteral("标记未完成")
                                                              : QStringLiteral("标记已完成"));
}

void SchedulePage::ClearEditorForNew() {
    _current_todo_id = 0;
    _current_todo_status = 0;
    ui->title_edit->clear();
    ui->event_edit->clear();
    ui->location_edit->clear();
    ui->time_text_edit->clear();
    ui->start_time_edit->clear();
    ui->end_time_edit->clear();
    ui->toggle_status_btn->setText(QStringLiteral("标记已完成"));
}

QJsonObject SchedulePage::BuildTodoPayload() const {
    QString source_text = _source_text.trimmed();
    if (source_text.isEmpty()) {
        source_text = ui->raw_input_edit->toPlainText().trimmed();
    }

    QString event_text = ui->event_edit->toPlainText().trimmed();
    if (event_text.isEmpty()) {
        event_text = source_text;
    }

    QString title_text = ui->title_edit->text().trimmed();
    if (title_text.isEmpty()) {
        title_text = source_text.isEmpty() ? QStringLiteral("未命名待办") : source_text.left(20);
    }

    QJsonObject req;
    req["uid"] = UserMgr::GetInstance()->GetUid();
    req["source_thread_id"] = _source_thread_id;
    req["source_message_id"] = _source_message_id;
    req["source_text"] = source_text;
    req["title"] = title_text;
    req["event"] = event_text;
    req["location"] = ui->location_edit->text().trimmed();
    req["time_text"] = ui->time_text_edit->text().trimmed();
    req["start_time"] = ui->start_time_edit->text().trimmed();
    req["end_time"] = ui->end_time_edit->text().trimmed();
    req["status"] = _current_todo_status;
    return req;
}

void SchedulePage::FillTodoList(QListWidget* list, const QList<QJsonObject>& todos) {
    list->clear();
    for (const QJsonObject& obj : todos) {
        QString title = obj["title"].toString().trimmed();
        if (title.isEmpty()) {
            title = QStringLiteral("未命名待办");
        }
        const QString summary = BuildTodoSummary(obj);
        const QString event_text = obj["event"].toString().trimmed();

        QString text = title + "\n" + summary;
        if (!event_text.isEmpty()) {
            text += "\n" + event_text;
        }

        auto* item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, QJsonDocument(obj).toJson(QJsonDocument::Compact));
        item->setToolTip(event_text);
        item->setSizeHint(QSize(item->sizeHint().width(), 72));
        list->addItem(item);
    }
}

void SchedulePage::SelectTodoFromItem(QListWidget* active_list, QListWidgetItem* item) {
    if (active_list == ui->pending_todo_list) {
        ui->done_todo_list->clearSelection();
    } else {
        ui->pending_todo_list->clearSelection();
    }

    const QJsonObject obj = ParseTodoObject(item);
    if (obj.isEmpty()) {
        return;
    }

    LoadTodoToEditor(obj);
    ui->status_lb->setText(QStringLiteral("已载入待办：%1").arg(obj["title"].toString()));
}

void SchedulePage::SetBusy(bool busy) {
    _busy = busy;
    ui->parse_btn->setEnabled(!busy);
    ui->save_btn->setEnabled(!busy);
    ui->update_btn->setEnabled(!busy);
    ui->delete_btn->setEnabled(!busy);
    ui->toggle_status_btn->setEnabled(!busy);
    ui->new_btn->setEnabled(!busy);
    ui->refresh_btn->setEnabled(!busy);
}
