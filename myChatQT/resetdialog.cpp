#include "resetdialog.h"
#include "ui_resetdialog.h"
#include <QDebug>
#include <QRegularExpression>
#include "global.h"
#include "httpmgr.h"
#include <QShortcut>

ResetDialog::ResetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ResetDialog)
{
    ui->setupUi(this);
    setMinimumSize(420, 500);
    setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    setWindowTitle("myChat - 重置密码");

    ui->user_edit->setPlaceholderText("请输入用户名");
    ui->email_edit->setPlaceholderText("请输入注册邮箱");
    ui->varify_edit->setPlaceholderText("请输入验证码");
    ui->pwd_edit->setPlaceholderText("请输入新密码（6-15位）");

    ui->user_edit->setClearButtonEnabled(true);
    ui->email_edit->setClearButtonEnabled(true);
    ui->varify_edit->setClearButtonEnabled(true);
    ui->pwd_edit->setClearButtonEnabled(true);

    ui->user_edit->setMaxLength(20);
    ui->email_edit->setMaxLength(50);
    ui->varify_edit->setMaxLength(6);
    ui->pwd_edit->setMaxLength(15);
    ui->pwd_edit->setEchoMode(QLineEdit::Password);

    ui->varify_btn->setCursor(Qt::PointingHandCursor);
    ui->sure_btn->setCursor(Qt::PointingHandCursor);
    ui->return_btn->setCursor(Qt::PointingHandCursor);

    ui->varify_btn->setMinimumHeight(34);
    ui->sure_btn->setMinimumHeight(34);
    ui->return_btn->setMinimumHeight(34);

    ui->user_edit->setFocus();

    connect(ui->user_edit,&QLineEdit::editingFinished,this,[this](){
        checkUserValid();
    });

    connect(ui->email_edit, &QLineEdit::editingFinished, this, [this](){
        checkEmailValid();
    });

    connect(ui->pwd_edit, &QLineEdit::editingFinished, this, [this](){
        checkPassValid();
    });


    connect(ui->varify_edit, &QLineEdit::editingFinished, this, [this](){
         checkVarifyValid();
    });

    connect(ui->user_edit, &QLineEdit::returnPressed, this, &ResetDialog::on_sure_btn_clicked);
    connect(ui->email_edit, &QLineEdit::returnPressed, this, &ResetDialog::on_sure_btn_clicked);
    connect(ui->varify_edit, &QLineEdit::returnPressed, this, &ResetDialog::on_sure_btn_clicked);
    connect(ui->pwd_edit, &QLineEdit::returnPressed, this, &ResetDialog::on_sure_btn_clicked);

    auto *escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(escShortcut, &QShortcut::activated, this, &ResetDialog::on_return_btn_clicked);

    //连接reset相关信号和注册处理回调
    initHandlers();
    connect(HttpMgr::GetInstance().get(), &HttpMgr::sig_reset_mod_finish, this,
            &ResetDialog::slot_reset_mod_finish);

}


ResetDialog::~ResetDialog()
{
    delete ui;
}

void ResetDialog::on_return_btn_clicked()
{
    qDebug() << "sure btn clicked ";
    emit switchLogin();
}

void ResetDialog::on_varify_btn_clicked()
{
    qDebug()<<"receive varify btn clicked ";
    auto email = ui->email_edit->text();
    auto bcheck = checkEmailValid();
    if(!bcheck){
        return;
    }

    ui->varify_btn->setEnabled(false);
    //发送http请求获取验证码
    QJsonObject json_obj;
    json_obj["email"] = email;
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/get_varifycode"),
                                        json_obj, ReqId::ID_GET_VARIFY_CODE,Modules::RESETMOD);
}

void ResetDialog::slot_reset_mod_finish(ReqId id, QString res, ErrorCodes err)
{
    if(err != ErrorCodes::SUCCESS){
        if (id == ReqId::ID_GET_VARIFY_CODE) {
            ui->varify_btn->setEnabled(true);
        }
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

bool ResetDialog::checkUserValid()
{
    if(ui->user_edit->text() == ""){
        AddTipErr(TipErr::TIP_USER_ERR, tr("用户名不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_USER_ERR);
    return true;
}


bool ResetDialog::checkPassValid()
{
    auto pass = ui->pwd_edit->text();

    if(pass.length() < 6 || pass.length()>15){
        //提示长度不准确
        AddTipErr(TipErr::TIP_PWD_ERR, tr("密码长度应为6~15"));
        return false;
    }

    // 创建一个正则表达式对象，按照上述密码要求
    // 这个正则表达式解释：
    // ^[a-zA-Z0-9!@#$%^&*]{6,15}$ 密码长度至少6，可以是字母、数字和特定的特殊字符
    QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    bool match = regExp.match(pass).hasMatch();
    if(!match){
        //提示字符非法
        AddTipErr(TipErr::TIP_PWD_ERR, tr("不能包含非法字符"));
        return false;;
    }

    DelTipErr(TipErr::TIP_PWD_ERR);

    return true;
}



bool ResetDialog::checkEmailValid()
{
    //验证邮箱的地址正则表达式
    auto email = ui->email_edit->text();
    // 邮箱地址的正则表达式
    QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    bool match = regex.match(email).hasMatch(); // 执行正则表达式匹配
    if(!match){
        //提示邮箱不正确
        AddTipErr(TipErr::TIP_EMAIL_ERR, tr("邮箱地址不正确"));
        return false;
    }

    DelTipErr(TipErr::TIP_EMAIL_ERR);
    return true;
}

bool ResetDialog::checkVarifyValid()
{
    auto pass = ui->varify_edit->text();
    if(pass.isEmpty()){
        AddTipErr(TipErr::TIP_VARIFY_ERR, tr("验证码不能为空"));
        return false;
    }

    DelTipErr(TipErr::TIP_VARIFY_ERR);
    return true;
}

void ResetDialog::AddTipErr(TipErr te, QString tips)
{
    _tip_errs[te] = tips;
    showTip(tips, false);
}

void ResetDialog::DelTipErr(TipErr te)
{
    _tip_errs.remove(te);
    if(_tip_errs.empty()){
      ui->err_tip->clear();
      return;
    }

    showTip(_tip_errs.first(), false);
}

void ResetDialog::initHandlers()
{
    //注册获取验证码回包逻辑
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            ui->varify_btn->ResetCountDown();
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("验证码已发送到邮箱，注意查收"), true);
        ui->varify_btn->BeginCountDown();
        qDebug()<< "email is " << email ;
    });

    //注册注册用户回包逻辑
    _handlers.insert(ReqId::ID_RESET_PWD, [this](QJsonObject jsonObj){
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("重置成功,点击返回登录"), true);
        qDebug()<< "email is " << email ;
        qDebug()<< "user uuid is " <<  jsonObj["uuid"].toString();
    });
}

void ResetDialog::showTip(QString str, bool b_ok)
{
    if(b_ok){
         ui->err_tip->setProperty("state","normal");
    }else{
        ui->err_tip->setProperty("state","err");
    }

    ui->err_tip->setText(str);

    repolish(ui->err_tip);
}

void ResetDialog::on_sure_btn_clicked()
{
    bool valid = checkUserValid();
    if(!valid){
        return;
    }

    valid = checkEmailValid();
    if(!valid){
        return;
    }

    valid = checkPassValid();
    if(!valid){
        return;
    }

    valid = checkVarifyValid();
    if(!valid){
        return;
    }

    //发送http重置用户请求
    QJsonObject json_obj;
    json_obj["user"] = ui->user_edit->text();
    json_obj["email"] = ui->email_edit->text();
    json_obj["passwd"] = xorString(ui->pwd_edit->text());
    json_obj["varifycode"] = ui->varify_edit->text();
    HttpMgr::GetInstance()->PostHttpReq(QUrl(gate_url_prefix+"/reset_pwd"),
                 json_obj, ReqId::ID_RESET_PWD,Modules::RESETMOD);
}
