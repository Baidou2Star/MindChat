#include "friendlabel.h"
#include "ui_friendlabel.h"
#include <QDebug>

FriendLabel::FriendLabel(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::FriendLabel)
{
    ui->setupUi(this);
    ui->close_lb->SetState("normal","hover","pressed",
                           "selected_normal","selected_hover","selected_pressed");
    connect(ui->close_lb, &ClickedLabel::clicked, this, &FriendLabel::slot_close);
}

FriendLabel::~FriendLabel()
{
    delete ui;
}

void FriendLabel::SetText(QString text)
{
    _text = text;
    ui->tip_lb->setText(_text);
    ui->tip_lb->adjustSize();

    QFontMetrics fontMetrics(ui->tip_lb->font());
    const int textWidth = fontMetrics.horizontalAdvance(ui->tip_lb->text());
    const int textHeight = fontMetrics.height();
    const int chipHeight = qMax(30, textHeight + 12);
    const int chipWidth = textWidth + ui->close_wid->width() + 16;

    ui->tip_lb->setFixedHeight(chipHeight);
    ui->close_wid->setFixedHeight(chipHeight);
    this->setFixedWidth(chipWidth);
    this->setFixedHeight(chipHeight);
    _width = this->width();
    _height = this->height();
}

int FriendLabel::Width()
{
    return _width;
}

int FriendLabel::Height()
{
    return _height;
}

QString FriendLabel::Text()
{
    return _text;
}

void FriendLabel::slot_close()
{
    emit sig_close(_text);
}
