#ifndef TIMERBTN_H
#define TIMERBTN_H
#include <QPushButton>
#include <QTimer>

class TimerBtn : public QPushButton
{
public:
    TimerBtn(QWidget *parent = nullptr);
    ~ TimerBtn();
    void BeginCountDown(int seconds = 10);
    void ResetCountDown();

private:
    QTimer  *_timer;
    int _counter;
};

#endif // TIMERBTN_H
