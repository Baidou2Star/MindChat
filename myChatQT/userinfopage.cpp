#include "userinfopage.h"
#include "ui_userinfopage.h"
#include "usermgr.h"
#include <QDebug>
#include <QMessageBox>
#include "imagecropperdialog.h"
#include "imagecropperlabel.h"
#include <QFileDialog>
#include <QStandardPaths>
#include <QDir>
#include "usermgr.h"
#include "tcpmgr.h"
#include "filetcpmgr.h"
#include "global.h"
#include <QRegularExpression>
#include <QSpacerItem>
#include <QSizePolicy>

UserInfoPage::UserInfoPage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::UserInfoPage)
{
    ui->setupUi(this);
    ui->verticalLayout_2->setContentsMargins(24, 20, 24, 20);
    ui->verticalLayout_2->setSpacing(10);
    ui->label->setText(tr("昵称："));
    ui->label_2->setText(tr("用户名："));
    ui->label_3->setText(tr("描述："));
    ui->label->setFixedWidth(72);
    ui->label_2->setFixedWidth(72);
    ui->label_3->setFixedWidth(72);
    ui->label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->label_2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->label_3->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    ui->nick_ed->setMinimumWidth(0);
    ui->name_ed->setMinimumWidth(0);
    ui->desc_ed->setMinimumWidth(0);
    ui->nick_ed->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->name_ed->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->desc_ed->setMaximumWidth(QWIDGETSIZE_MAX);
    ui->nick_ed->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->name_ed->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->desc_ed->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    ui->horizontalLayout_2->setStretch(1, 1);
    ui->horizontalLayout_3->setStretch(1, 1);
    ui->horizontalLayout_4->setStretch(1, 1);
    ui->horizontalSpacer->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    ui->horizontalSpacer_2->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    ui->horizontalSpacer_3->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    ui->head_lb->setFixedSize(160, 160);
    ui->head_lb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ui->horizontalSpacer_6->changeSize(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    ui->horizontalLayout->setStretch(1, 1);
    ui->head_lb->setStyleSheet("border-radius: 12px; border: 2px solid #d8e4f4; background: #f6f9ff;");
    ui->head_lb->setAlignment(Qt::AlignCenter);
    ui->label_4->setObjectName("profile_title");
    ui->up_btn->setCursor(Qt::PointingHandCursor);
    ui->submit_btn->setCursor(Qt::PointingHandCursor);
    ui->up_btn->setMinimumHeight(34);
    ui->submit_btn->setMinimumHeight(34);
    ui->nick_ed->setPlaceholderText("请输入昵称");
    ui->name_ed->setPlaceholderText("请输入用户名");
    ui->desc_ed->setPlaceholderText("请输入个性签名");
    ui->nick_ed->setClearButtonEnabled(true);
    ui->name_ed->setClearButtonEnabled(true);
    ui->desc_ed->setClearButtonEnabled(true);
    ui->nick_ed->setMaxLength(20);
    ui->name_ed->setMaxLength(20);
    ui->desc_ed->setMaxLength(40);
    ui->head_lb->setPixmap(QPixmap(":/res/profile_cover_banner.jpg").scaled(
        ui->head_lb->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    ui->head_lb->setScaledContents(false);

    auto icon = UserMgr::GetInstance()->GetIcon();
    qDebug() << "icon is " << icon ;

    //使用正则表达式检查是否使用默认头像
    QRegularExpression regex("^:/res/head_(\\d+)\\.jpg$");
    QRegularExpressionMatch match = regex.match(icon);
    if (match.hasMatch()) {
        QPixmap pixmap(icon);
        QPixmap scaledPixmap = pixmap.scaled(ui->head_lb->size(), 
            Qt::KeepAspectRatio, Qt::SmoothTransformation); // 将图片缩放到label的大小
        ui->head_lb->setPixmap(scaledPixmap); // 将缩放后的图片设置到QLabel上
        ui->head_lb->setScaledContents(false); // 保持正方形头像，不进行控件拉伸
    }
    else {
        // 如果是用户上传的头像，获取存储目录
        QString storageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        auto uid = UserMgr::GetInstance()->GetUid();
        QDir avatarsDir(storageDir + "/user/" + QString::number(uid) + "/avatars");
        // 确保目录存在
        if (avatarsDir.exists()) {
            auto file_name = QFileInfo(icon).fileName();
            QString avatarPath = avatarsDir.filePath(QFileInfo(icon).fileName()); // 获取上传头像的完整路径
            QPixmap pixmap(avatarPath); // 加载上传的头像图片
            if (!pixmap.isNull()) {

                //判断是否正在下载
                bool is_loading = UserMgr::GetInstance()->IsDownLoading(file_name);
                if (is_loading) {
                    qWarning() << "正在下载: " << file_name;
                    //先加载默认的
                    QPixmap pixmap(":/res/head_0.jpg");
                    QPixmap scaledPixmap = pixmap.scaled(ui->head_lb->size(),
                        Qt::KeepAspectRatio, Qt::SmoothTransformation); // 将图片缩放到label的大小
                    ui->head_lb->setPixmap(scaledPixmap); // 将缩放后的图片设置到QLabel上
                    ui->head_lb->setScaledContents(false); // 保持正方形头像，不进行控件拉伸
                    
                }
                else {
                    QPixmap scaledPixmap = pixmap.scaled(ui->head_lb->size(),
                        Qt::KeepAspectRatio, Qt::SmoothTransformation); // 将图片缩放到label的大小
                    ui->head_lb->setPixmap(scaledPixmap); // 将缩放后的图片设置到QLabel上
                    ui->head_lb->setScaledContents(false); // 保持正方形头像，不进行控件拉伸
                }
            }
            else {
                qWarning() << "无法加载上传的头像：" << avatarPath;
                LoadHeadIcon(avatarPath, ui->head_lb, file_name,"self_icon");
            }
        }
        else {
            qWarning() << "头像存储目录不存在：" << avatarsDir.path();
        }
    }


  
    //获取nick
    auto nick = UserMgr::GetInstance()->GetNick();
    //获取name
    auto name = UserMgr::GetInstance()->GetName();
    //描述
    auto desc = UserMgr::GetInstance()->GetDesc();
    ui->nick_ed->setText(nick);
    ui->name_ed->setText(name);
    ui->desc_ed->setText(desc);
    //连接上
    connect(ui->up_btn, &QPushButton::clicked, this, &UserInfoPage::slot_up_load);
}

UserInfoPage::~UserInfoPage()
{
    delete ui;
}

void UserInfoPage::LoadHeadIcon(QString avatarPath, QLabel* icon_label, QString file_name, QString req_type) {
    UserMgr::GetInstance()->AddLabelToReset(avatarPath, ui->head_lb);
    //先加载默认的
    QPixmap pixmap(":/res/head_0.jpg");
    QPixmap scaledPixmap = pixmap.scaled(ui->head_lb->size(),
        Qt::KeepAspectRatio, Qt::SmoothTransformation); // 将图片缩放到label的大小
    ui->head_lb->setPixmap(scaledPixmap); // 将缩放后的图片设置到QLabel上
    ui->head_lb->setScaledContents(false); // 保持正方形头像，不进行控件拉伸

    //判断是否正在下载
    bool is_loading = UserMgr::GetInstance()->IsDownLoading(file_name);
    if (is_loading) {
        qWarning() << "正在下载: " << file_name;
    }
    else {
        //发送请求获取资源
        auto download_info = std::make_shared<DownloadInfo>();
        download_info->_name = file_name;
        download_info->_current_size = 0;
        download_info->_seq = 1;
        download_info->_total_size = 0;
        download_info->_client_path = avatarPath;
        //添加文件到管理者
        UserMgr::GetInstance()->AddDownloadFile(file_name, download_info);
        //发送消息
        FileTcpMgr::GetInstance()->SendDownloadInfo(download_info, req_type);
    }

}

//上传头像
void UserInfoPage::slot_up_load()
{
    // 1. 让对话框也能选 *.webp
    QString filename = QFileDialog::getOpenFileName(
        this,
        tr("选择图片"),
        QString(),
        tr("图片文件 (*.png *.jpg *.jpeg *.bmp *.webp)")
    );
    if (filename.isEmpty())
        return;

    // 2. 直接用 QPixmap::load() 加载，无需手动区分格式
    QPixmap inputImage;
    if (!inputImage.load(filename)) {
        QMessageBox::critical(
            this,
            tr("错误"),
            tr("加载图片失败！请确认已部署 WebP 插件。"),
            QMessageBox::Ok
        );
        return;
    }

    QPixmap image = ImageCropperDialog::getCroppedImage(filename, 600, 400, CropperShape::CIRCLE);
    if (image.isNull())
        return;

    QPixmap scaledPixmap = image.scaled( ui->head_lb->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation); // 将图片缩放到label的大小
    ui->head_lb->setPixmap(scaledPixmap); // 将缩放后的图片设置到QLabel上
    ui->head_lb->setScaledContents(false); // 保持正方形头像，不进行控件拉伸

    QString storageDir = QStandardPaths::writableLocation(
                             QStandardPaths::AppDataLocation);
    auto uid = UserMgr::GetInstance()->GetUid();
    storageDir = storageDir + "/" + QString::number(uid);
    // 2. 在其下再建一个 avatars 子目录
    QDir dir(storageDir);
    if (!dir.exists("avatars")) {
        if (!dir.mkpath("avatars")) {
            qWarning() << "无法创建 avatars 目录：" << dir.filePath("avatars");
            QMessageBox::warning(
                this,
                tr("错误"),
                tr("无法创建存储目录，请检查权限或磁盘空间。")
            );
            return;
        }
    }
    // 3. 拼接最终的文件名 head.png
    QString file_name = generateUniqueIconName();
    QString filePath = dir.filePath("avatars" +
                          QString(QDir::separator()) + file_name);

    // 4. 保存 scaledPixmap 为 PNG（无损、最高质量）
    if (!scaledPixmap.save(filePath, "PNG")) {
        QMessageBox::warning(
            this,
            tr("保存失败"),
            tr("头像保存失败，请检查权限或磁盘空间。")
        );
    } else {
        qDebug() << "头像已保存到：" << filePath;
        // 以后读取直接用同一路径：storageDir/avatars/head.png
    }

    //实现头像上传
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)){
        qWarning() << "Could not open file:" << file.errorString();
        return;
    }

    //保存当前文件位置指针
    qint64 originalPos = file.pos();

    QCryptographicHash hash(QCryptographicHash::Md5);
    if (!hash.addData(&file)) {
            qWarning() << "Failed to read data from file:" << filePath;
            return ;
    }

    // 5. 转化为16进制字符串
    QString file_md5 = hash.result().toHex(); // 返回十六进制字符串

    //读取文件内容并发送
    QByteArray buffer;
    int seq = 0;

    //创建QFileInfo 对象
    auto fileInfo = std::make_shared<QFileInfo>(filePath);
    //获取文件名
    QString fileName = fileInfo->fileName();
    //文件名
    qDebug() << "文件名是: " << fileName;

    //获取文件大小
    int total_size = fileInfo->size();
    //最后一个发送序列
    int last_seq = 0;
    //获取最后一个发送序列
    if(total_size % MAX_FILE_LEN){
        last_seq = (total_size / MAX_FILE_LEN) +1;
    }else{
        last_seq = total_size / MAX_FILE_LEN;
    }

    // 恢复文件指针到原来的位置
    file.seek(originalPos);

    //每次读取MAX_FILE_LEN字节并发送
    buffer = file.read(MAX_FILE_LEN);

    QJsonObject jsonObj;
    //将文件内容转化为Base64 编码(可选)
    QString base64Data = buffer.toBase64();
    ++seq;
    jsonObj["md5"] = file_md5;
    jsonObj["name"] = file_name;
    jsonObj["seq"] = seq;
    jsonObj["trans_size"] = buffer.size() + (seq - 1) * MAX_FILE_LEN;
    jsonObj["total_size"] = total_size;
    jsonObj["token"] = UserMgr::GetInstance()->GetToken();
    jsonObj["uid"] = UserMgr::GetInstance()->GetUid();

    if (buffer.size() + (seq - 1) * MAX_FILE_LEN == total_size) {
        jsonObj["last"] = 1;
    } else {
        jsonObj["last"] = 0;
    }

    jsonObj["data"] = base64Data;
    jsonObj["last_seq"] = last_seq;
    QJsonDocument doc(jsonObj);
    auto send_data = doc.toJson();
    //设置Icon
    UserMgr::GetInstance()->SetIcon(file_name);
    //将md5信息和文件信息关联存储
    UserMgr::GetInstance()->AddUploadFile(file_name, fileInfo);
    //发送消息
    FileTcpMgr::GetInstance()->SendData(ID_UPLOAD_HEAD_ICON_REQ, send_data);
    file.close();
    emit sig_reset_head();
}


