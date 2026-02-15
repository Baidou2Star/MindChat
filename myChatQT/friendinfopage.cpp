#include "friendinfopage.h"
#include "ui_friendinfopage.h"
#include <QDebug>
#include <QRegularExpression>

FriendInfoPage::FriendInfoPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::FriendInfoPage),_user_info(nullptr)
{
    ui->setupUi(this);
    ui->verticalLayout->setContentsMargins(36, 28, 36, 28);
    ui->verticalLayout->setSpacing(18);
    ui->widget->setObjectName("friend_info_card");
    ui->widget_6->setObjectName("friend_action_bar");
    ui->icon_lb->setMinimumSize(96, 96);
    ui->icon_lb->setMaximumSize(96, 96);
    ui->icon_lb->setAlignment(Qt::AlignCenter);
    ui->icon_lb->setStyleSheet("border-radius: 48px; border: 2px solid #d6e2f2; background: #f4f8fe;");
    ui->msg_chat->setCursor(Qt::PointingHandCursor);
    ui->voice_chat->setCursor(Qt::PointingHandCursor);
    ui->video_chat->setCursor(Qt::PointingHandCursor);
    ui->msg_chat->setToolTip("发消息");
    ui->voice_chat->setToolTip("语音通话（预留）");
    ui->video_chat->setToolTip("视频通话（预留）");
    ui->msg_chat->SetState("normal","hover","press");
    ui->video_chat->SetState("normal","hover","press");
    ui->voice_chat->SetState("normal","hover","press");
}

FriendInfoPage::~FriendInfoPage()
{
    delete ui;
}

void FriendInfoPage::SetInfo(std::shared_ptr<UserInfo> user_info)
{
    _user_info = user_info;
    // 加载图片
    QPixmap pixmap;
    QRegularExpression regex("^:/res/head_(\\d+)\\.jpg$");
    QRegularExpressionMatch match = regex.match(user_info->_icon);
    if (match.hasMatch()) {
        pixmap.load(user_info->_icon);
    } else {
        pixmap.load(user_info->_icon);
        if (pixmap.isNull()) {
            pixmap.load(":/res/head_0.jpg");
        }
    }

    // 设置图片自动缩放
    ui->icon_lb->setPixmap(pixmap.scaled(ui->icon_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    ui->icon_lb->setScaledContents(true);

    ui->name_lb->setText(user_info->_name);
    ui->nick_lb->setText(user_info->_nick);
    ui->bak_lb->setText(user_info->_nick);
}

void FriendInfoPage::on_msg_chat_clicked()
{
    qDebug() << "msg chat btn clicked";
    emit sig_jump_chat_item(_user_info);
}
