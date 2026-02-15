#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "resetdialog.h"
#include "tcpmgr.h"
#include <QLayout>
#include <QMessageBox>
#include "filetcpmgr.h"
#include <QScreen>
#include <QGuiApplication>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    _ui_status = LOGIN_UI;
    ui->setupUi(this);
    this->setMinimumSize(480, 360);
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    applyWindowSizeByScreen();
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    _login_dlg->show();
    setCentralWidget(_login_dlg);

    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接创建聊天界面信号
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_swich_chatdlg, this, &MainWindow::SlotSwitchChat);
    //链接服务器踢人消息
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_notify_offline, this, &MainWindow::SlotOffline);
    //连接服务器断开心跳超时或异常连接信息
    connect(TcpMgr::GetInstance().get(),&TcpMgr::sig_connection_closed, this, &MainWindow::SlotExcepConOffline);
    //连接资源服务器断开
    connect(FileTcpMgr::GetInstance().get(), &FileTcpMgr::sig_connection_closed,
            this, &MainWindow::SlotResServerConOffline);
}

void MainWindow::applyWindowSizeByScreen()
{
    if (_ui_status != CHAT_UI) {
        applyAuthWindowSize();
        return;
    }

    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) {
        this->setMinimumSize(720, 480);
        this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        this->resize(960, 540);
        return;
    }

    const QSize screen_size = screen->availableGeometry().size();
    const int width = qMax(720, screen_size.width() / 2);
    const int height = qMax(480, (screen_size.height() * 2) / 3);
    this->setMinimumSize(720, 480);
    this->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    this->resize(width, height);
}

void MainWindow::applyAuthWindowSize()
{
    constexpr int kAuthWidth = 420;
    constexpr int kAuthHeight = 500;
    this->setMinimumSize(kAuthWidth, kAuthHeight);
    this->setMaximumSize(kAuthWidth, kAuthHeight);
    this->resize(kAuthWidth, kAuthHeight);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::SlotSwitchReg()
{
    _reg_dlg = new RegisterDialog(this);
    _reg_dlg->hide();

    _reg_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);

     //连接注册界面返回登录信号
    connect(_reg_dlg, &RegisterDialog::sigSwitchLogin, this, &MainWindow::SlotSwitchLogin);
    setCentralWidget(_reg_dlg);
    _login_dlg->hide();
    _reg_dlg->show();
    _ui_status = REGISTER_UI;
    applyWindowSizeByScreen();
}

//从注册界面返回登录界面
void MainWindow::SlotSwitchLogin()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

   _reg_dlg->hide();
    _login_dlg->show();
    _ui_status = LOGIN_UI;
    applyWindowSizeByScreen();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
}

void MainWindow::SlotSwitchReset()
{
    _ui_status = RESET_UI;
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _reset_dlg = new ResetDialog(this);
    _reset_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_reset_dlg);

   _login_dlg->hide();
    _reset_dlg->show();
    applyWindowSizeByScreen();
    //注册返回登录信号和槽函数
    connect(_reset_dlg, &ResetDialog::switchLogin, this, &MainWindow::SlotSwitchLogin2);
}

//从重置界面返回登录界面
void MainWindow::SlotSwitchLogin2()
{
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

   _reset_dlg->hide();
    _login_dlg->show();
    _ui_status = LOGIN_UI;
    applyWindowSizeByScreen();
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
}

void MainWindow::SlotSwitchChat()
{
    _chat_dlg = new ChatDialog();
    _chat_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_chat_dlg);
    _chat_dlg->show();
    _login_dlg->hide();
    _ui_status = CHAT_UI;
    applyWindowSizeByScreen();
    _chat_dlg->loadChatList();
}

void MainWindow::SlotOffline(){
    // 使用静态方法直接弹出一个信息框
        QMessageBox::information(this, "下线提示", "同账号异地登录，该终端下线！");
        TcpMgr::GetInstance()->CloseConnection();
        offlineLogin();
}

void MainWindow::SlotExcepConOffline()
{
    // 使用静态方法直接弹出一个信息框
        QMessageBox::information(this, "下线提示", "心跳超时或临界异常，该终端下线！");
        TcpMgr::GetInstance()->CloseConnection();
        FileTcpMgr::GetInstance()->CloseConnection();
        offlineLogin();
}


void MainWindow::SlotResServerConOffline(){
    // 使用静态方法直接弹出一个信息框
    QMessageBox::information(this, "下线提示", "与资源服务器断开连接！");
    TcpMgr::GetInstance()->CloseConnection();
    FileTcpMgr::GetInstance()->CloseConnection();
    offlineLogin();
}

void MainWindow::offlineLogin(){
    if(_ui_status == LOGIN_UI){
        return;
    }
    //创建一个CentralWidget, 并将其设置为MainWindow的中心部件
    _login_dlg = new LoginDialog(this);
    _login_dlg->setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);
    setCentralWidget(_login_dlg);

   _chat_dlg->hide();
   _ui_status = LOGIN_UI;
   applyWindowSizeByScreen();
    _login_dlg->show();
    //连接登录界面注册信号
    connect(_login_dlg, &LoginDialog::switchRegister, this, &MainWindow::SlotSwitchReg);
    //连接登录界面忘记密码信号
    connect(_login_dlg, &LoginDialog::switchReset, this, &MainWindow::SlotSwitchReset);
}
