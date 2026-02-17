#ifndef CHATPAGE_H
#define CHATPAGE_H

#include <QWidget>
#include "userdata.h"
#include <QMap>
#include "ChatItemBase.h"

namespace Ui {
class ChatPage;
}

class ChatPage : public QWidget
{
    Q_OBJECT
public:
    explicit ChatPage(QWidget *parent = nullptr);
    ~ChatPage();
    void SetChatData(std::shared_ptr<ChatThreadData> chat_data);
    void AppendChatMsg(std::shared_ptr<ChatDataBase> msg, bool rsp=true);
    void UpdateChatStatus(std::shared_ptr<ChatDataBase> msg);
    void UpdateImgChatStatus(std::shared_ptr<ImgChatData> img_msg);
    void SetSelfIcon(ChatItemBase* pChatItem, QString icon);
    void UpdateFileProgress(std::shared_ptr<MsgInfo> msg_info);
    void LoadHeadIcon(QString avatarPath, QLabel* icon_label, QString file_name, QString req_type);
    void AppendOtherMsg(std::shared_ptr<ChatDataBase> msg);
    void DownloadFileFinished(std::shared_ptr<MsgInfo> msg_info, QString file_path);
signals:
    void sig_add_todo_from_message(QString text, int thread_id, int message_id);
protected:
    void paintEvent(QPaintEvent *event);

private slots:
    void on_send_btn_clicked();

    //PictureBubbleͣź
    void on_clicked_paused(QString unique_name, TransferType transfer_type);
    //PictureBubbleļź
    void on_clicked_resume(QString unique_name, TransferType transfer_type);

private:
    void clearItems();
    Ui::ChatPage *ui;
    std::shared_ptr<ChatThreadData> _chat_data;
    QMap<QString, QWidget*>  _bubble_map;
    //δظϢ
    QHash<QString, ChatItemBase*> _unrsp_item_map;
    //ѾظϢ
    QHash<qint64, ChatItemBase*> _base_item_map;
};

#endif // CHATPAGE_H
