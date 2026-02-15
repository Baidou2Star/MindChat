#include "mainwindow.h"
#include <QApplication>
#include <QFile>
#include "global.h"
#include "tcpmgr.h"
#include "filetcpmgr.h"
#include <QScreen>
int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication a(argc, argv);

    QFont app_font("Segoe UI");
    int base_size = 10;
    if (const QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect geometry = screen->geometry();
        const qreal dpi = screen->logicalDotsPerInch();
        if (geometry.width() >= 3200 || dpi >= 130.0) {
            base_size = 12;
        } else if (geometry.width() >= 2560 || dpi >= 110.0) {
            base_size = 11;
        }
    }
    app_font.setPointSize(base_size);
    a.setFont(app_font);

    QFile qss(":/style/stylesheet.qss");

    if( qss.open(QFile::ReadOnly))
    {
        qDebug("open success");
        QString style = QLatin1String(qss.readAll());
        a.setStyleSheet(style);
        qss.close();
    }else{
         qDebug("Open failed");
     }


    // 获取当前应用程序的路径
    QString app_path = QCoreApplication::applicationDirPath();
    // 拼接文件名
    QString fileName = "config.ini";
    QString config_path = QDir::toNativeSeparators(app_path +
                             QDir::separator() + fileName);

    QSettings settings(config_path, QSettings::IniFormat);
    QString gate_host = settings.value("GateServer/host").toString();
    QString gate_port = settings.value("GateServer/port").toString();
    gate_url_prefix = "http://"+gate_host+":"+gate_port;

    //启动tcp线程
    TcpThread tcpthread;
    //启动资源网络线程
    FileTcpThread file_tcp_thread;
    MainWindow w;
    w.show();
    return a.exec();
}
