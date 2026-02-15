#include "timerbtn.h"

TimerBtn::TimerBtn(QWidget *parent):QPushButton(parent),_counter(10)
{
    _timer = new QTimer(this);

    connect(_timer, &QTimer::timeout, [this](){
        _counter--;
        if(_counter <= 0){
            _timer->stop();
            _counter = 10;
            this->setText("获取");
            this->setEnabled(true);
            return;
        }
        this->setText(QString::number(_counter));
    });
}

TimerBtn::~TimerBtn()
{
    _timer->stop();
}

void TimerBtn::BeginCountDown(int seconds)
{
    if (_timer->isActive()) {
        return;
    }

    if (seconds <= 0) {
        seconds = 10;
    }

    _counter = seconds;
    this->setEnabled(false);
    this->setText(QString::number(_counter));
    _timer->start(1000);
}

void TimerBtn::ResetCountDown()
{
    _timer->stop();
    _counter = 10;
    this->setText("获取");
    this->setEnabled(true);
}


