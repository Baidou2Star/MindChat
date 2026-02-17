#include "logindialog.h"
#include "ui_logindialog.h"
#include <QDebug>
#include "httpmgr.h"
#include "tcpmgr.h"
#include <QRegExp>
#include <QRegularExpression>
#include <QPainter>
#include "filetcpmgr.h"
#include "QPainterPath"
#include <QShortcut>
#include <QLabel>

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginDialog)
{
    ui->setupUi(this);
    setMinimumSize(420, 500);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    setWindowTitle("MindChat - 登录");

    ui->email_edit->setPlaceholderText("请输入邮箱");
    ui->pass_edit->setPlaceholderText("请输入密码");
    ui->email_edit->setClearButtonEnabled(true);
    ui->pass_edit->setClearButtonEnabled(true);
    ui->email_edit->setMaxLength(50);
    ui->pass_edit->setMaxLength(15);
    ui->pass_edit->setEchoMode(QLineEdit::Password);
    ui->email_edit->setFocus();

    ui->login_btn->setCursor(Qt::PointingHandCursor);
    ui->reg_btn->setCursor(Qt::PointingHandCursor);
    ui->login_btn->setMinimumHeight(36);
    ui->reg_btn->setMinimumHeight(36);
    ui->head_widget->setMinimumHeight(180);
    ui->head_label->setFixedSize(220, 140);
    ui->gridLayout->setAlignment(ui->head_label, Qt::AlignCenter);

    auto* titleLabel = new QLabel("MindChat", this);
    titleLabel->setObjectName("login_title");
    titleLabel->setAlignment(Qt::AlignCenter);
    auto* subtitleLabel = new QLabel("电话你先挂，聊天我结尾，有事和我讲", this);
    subtitleLabel->setObjectName("login_subtitle");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    ui->verticalLayout_2->insertWidget(1, titleLabel);
    ui->verticalLayout_2->insertWidget(2, subtitleLabel);

    connect(ui->email_edit, &QLineEdit::returnPressed, this, &LoginDialog::on_login_btn_clicked);
    connect(ui->pass_edit, &QLineEdit::returnPressed, this, &LoginDialog::on_login_btn_clicked);

    auto *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &QDialog::close);

    connect(ui->reg_btn, &QPushButton::clicked, this, &LoginDialog::switchRegister);
    ui->forget_label->SetState("normal","hover","","selected","selected_hover","");
    ui->forget_label->setCursor(Qt::PointingHandCursor);
    connect(ui->forget_label, &ClickedLabel::clicked, this, &LoginDialog::slot_forget_pwd);
    initHttpHandlers();
    //连接登录回包信号
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_login_mod_finish, this,
            &LoginDialog::slot_login_mod_finish);
    //连接tcp连接请求的信号和槽函数
    connect(this, &LoginDialog::sig_connect_tcp, TcpMgr::GetInstance().get(), &TcpMgr::slot_tcp_connect);
    //连接tcp管理者发出的连接成功信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_con_success, this, &LoginDialog::slot_tcp_con_finish);
    //连接tcp管理者发出的登陆失败信号
    connect(TcpMgr::GetInstance().get(), &TcpMgr::sig_login_failed, this, &LoginDialog::slot_login_failed);

    //连接tcp连接资源服务器请求的信号和槽函数
    connect(this, &LoginDialog::sig_connect_res_server,
            FileTcpMgr::GetInstance().get(), &FileTcpMgr::slot_tcp_connect);

    //连接资源管理tcp发出的连接成功信号
    connect(FileTcpMgr::GetInstance().get(), &FileTcpMgr::sig_con_success, this, &LoginDialog::slot_res_con_finish);
    initHead();
}

LoginDialog::~LoginDialog()
{
    qDebug()<<"destruct LoginDlg";
    delete ui;
}

void LoginDialog::initHead()
{
    QPixmap originalPixmap(":/res/login_hero_banner.png");
    if (originalPixmap.isNull()) {
        originalPixmap = QPixmap(":/res/myhead.jpg");
    }
    ui->head_label->setAlignment(Qt::AlignCenter);
    qDebug()<< originalPixmap.size() << ui->head_label->size();
    QPixmap scaledPixmap = originalPixmap.scaled(ui->head_label->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->head_label->setPixmap(scaledPixmap);
}

void LoginDialog::initHttpHandlers()
{
    //注册获取登录回包逻辑
    _handlers.insert(ReqId::ID_LOGIN_USER, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            enableBtn(true);
            return;
        }
        auto email = jsonObj["email"].toString();

        //发送信号通知tcpMgr发送长链接
        _si = std::make_shared<ServerInfo>();

        _si->_uid = jsonObj["uid"].toInt();
        _si->_chat_host = jsonObj["chathost"].toString();
        _si->_chat_port = jsonObj["chatport"].toString();
        _si->_token = jsonObj["token"].toString();

        _si->_res_host = jsonObj["reshost"].toString();
        _si->_res_port = jsonObj["resport"].toString();


        qDebug()<< "email is " << email << " uid is " << _si->_uid <<" chat host is "
                << _si->_chat_host << " chat port is "
                << _si->_chat_port << " token is " << _si->_token
                << " res host is " << _si->_res_host
                << " res port is " << _si->_res_port;
        emit sig_connect_tcp(_si);
       // qDebug() << "send thread is " << QThread::currentThread();
       // emit sig_test();
    });
}

void LoginDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
         ui->err_tip->setProperty("state","normal");
    }else{
        ui->err_tip->setProperty("state","err");
    }

    ui->err_tip->setText(str);

    repolish(ui->err_tip);
}

void LoginDialog::slot_forget_pwd()
{
    qDebug()<<"slot forget pwd";
    emit switchReset();
}

bool LoginDialog::checkUserValid(){

    auto email = ui->email_edit->text();
    if(email.isEmpty()){
        qDebug() << "email empty " ;
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱不能为空"));
        return false;
    }
    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool LoginDialog::checkPwdValid(){
    auto pwd = ui->pass_edit->text();
    if(pwd.length() < 6 || pwd.length() > 15){
        qDebug() << "Pass length invalid";
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    bool match = regExp.match(pwd).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符且长度为(6~15)"));
        return false;;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    return true;
}

bool LoginDialog::enableBtn(bool enabled)
{
    ui->login_btn->setEnabled(enabled);
    ui->reg_btn->setEnabled(enabled);
    return true;
}

void LoginDialog::on_login_btn_clicked()
{
    qDebug()<<"login btn clicked";
    if(checkUserValid() == false){
        return;
    }

    if(checkPwdValid() == false){
        return ;
    }

    enableBtn(false);
    auto email = ui->email_edit->text();
    auto pwd = ui->pass_edit->text();
    //发送http请求登录
    QJsonObject json_obj;
    json_obj["email"] = email;
    json_obj["passwd"] = xorString(pwd);
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_login"),
                                        json_obj, ReqId::ID_LOGIN_USER,Modules::LOGINMOD);
}

void LoginDialog::slot_login_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return;
    }

    // 解析 JSON 字符串,res需转化为QByteArray
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    //json解析错误
    if(jsonDoc.isNull()){
        showTip(tr("json解析错误"),false);
        return;
    }

    //json解析错误
    if(!jsonDoc.isObject()){
        showTip(tr("json解析错误"),false);
        return;
    }


    //调用对应的逻辑,根据id回调。
    _handlers[id](jsonDoc.object());

    return;
}

void LoginDialog::slot_tcp_con_finish(bool bsuccess)
{
    if(bsuccess){
        showTip(tr("聊天服务连接成功，正在连接资源服务器..."),true);
        emit sig_connect_res_server(_si);
    }else{
        showTip(tr("网络异常"), false);
    }
}

void LoginDialog::slot_login_failed(int err)
{
    QString result = QString("登录失败, err is %1")
                             .arg(err);
    showTip(result,false);
    enableBtn(true);
}

void LoginDialog::slot_res_con_finish(bool bsuccess)
{
       if(bsuccess){
          showTip(tr("聊天服务连接成功，正在登录..."),true);
          QJsonObject jsonObj;
          jsonObj["uid"] = _si->_uid;
          jsonObj["token"] = _si->_token;

          QJsonDocument doc(jsonObj);
          QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

          //发送tcp请求给chat server
         emit TcpMgr::GetInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN, jsonData);

       }else{
          showTip(tr("网络异常"),false);
          enableBtn(true);
       }

}

void LoginDialog::AddTipErr(TipErr te,QString tips){
    _tip_errs[te] = tips;
    showTip(tips, false);
}
void LoginDialog::DelTipErr(TipErr te){
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
      ui->err_tip->clear();
      return;
    }

    showTip(_tip_errs.first(), false);
}
