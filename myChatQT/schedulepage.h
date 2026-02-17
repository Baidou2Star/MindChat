#ifndef SCHEDULEPAGE_H
#define SCHEDULEPAGE_H

#include <QJsonObject>
#include <QListWidget>
#include <QWidget>

namespace Ui {
class SchedulePage;
}

class SchedulePage : public QWidget {
    Q_OBJECT

public:
    explicit SchedulePage(QWidget* parent = nullptr);
    ~SchedulePage();

    void LoadFromText(const QString& text, int source_thread_id, int source_message_id, bool auto_parse = true);
    void RefreshTodoList();

private slots:
    void on_parse_btn_clicked();
    void on_save_btn_clicked();
    void on_update_btn_clicked();
    void on_delete_btn_clicked();
    void on_toggle_status_btn_clicked();
    void on_new_btn_clicked();
    void on_refresh_btn_clicked();

    void slot_parse_todo_rsp(QJsonObject rsp);
    void slot_create_todo_rsp(QJsonObject rsp);
    void slot_update_todo_rsp(QJsonObject rsp);
    void slot_delete_todo_rsp(QJsonObject rsp);
    void slot_set_todo_status_rsp(QJsonObject rsp);
    void slot_list_todo_rsp(QJsonObject rsp);

private:
    void BindTodoSignals();
    void LoadTodoToEditor(const QJsonObject& todo);
    void ClearEditorForNew();
    QJsonObject BuildTodoPayload() const;
    void FillTodoList(QListWidget* list, const QList<QJsonObject>& todos);
    void SelectTodoFromItem(QListWidget* active_list, QListWidgetItem* item);
    void SetBusy(bool busy);
    void RequestParse();
    void RequestList();

private:
    Ui::SchedulePage* ui;
    int _source_thread_id;
    int _source_message_id;
    QString _source_text;
    int _current_todo_id;
    int _current_todo_status;
    bool _busy;
};

#endif // SCHEDULEPAGE_H
