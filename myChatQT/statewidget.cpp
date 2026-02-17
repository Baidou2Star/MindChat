#include "statewidget.h"
#include <QPaintEvent>
#include <QStyleOption>
#include <QPainter>
#include <QLabel>
#include <QVBoxLayout>
#include <QImage>

namespace {
QRect AlphaBoundingRect(const QImage& image)
{
    int left = image.width();
    int right = -1;
    int top = image.height();
    int bottom = -1;

    for (int y = 0; y < image.height(); ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(line[x]) == 0) {
                continue;
            }
            left = qMin(left, x);
            right = qMax(right, x);
            top = qMin(top, y);
            bottom = qMax(bottom, y);
        }
    }

    if (right < left || bottom < top) {
        return QRect(0, 0, image.width(), image.height());
    }
    return QRect(QPoint(left, top), QPoint(right, bottom));
}

QString ResolveSideIconPath(const QString& object_name, bool selected)
{
    if (object_name == "side_chat_lb") {
        return selected ? QStringLiteral(":/res/sidebar_chat_active_enterprise.png")
                        : QStringLiteral(":/res/sidebar_chat_outline_enterprise.png");
    }
    if (object_name == "side_contact_lb") {
        return selected ? QStringLiteral(":/res/sidebar_contact_active_enterprise.png")
                        : QStringLiteral(":/res/sidebar_contact_outline_enterprise.png");
    }
    if (object_name == "side_schedule_lb") {
        return selected ? QStringLiteral(":/res/sidebar_schedule_active_enterprise.png")
                        : QStringLiteral(":/res/sidebar_schedule_outline_enterprise.png");
    }
    if (object_name == "side_settings_lb") {
        return selected ? QStringLiteral(":/res/sidebar_settings_active_enterprise.png")
                        : QStringLiteral(":/res/sidebar_settings_outline_enterprise.png");
    }
    return QString();
}

QPixmap DarkenPixmap(const QPixmap& src, int alpha)
{
    if (src.isNull()) {
        return src;
    }

    QImage image = src.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&image);
    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
    painter.fillRect(image.rect(), QColor(0, 0, 0, alpha));
    painter.end();
    return QPixmap::fromImage(image);
}
}

StateWidget::StateWidget(QWidget *parent) : QWidget(parent),_curstate(ClickLbState::Normal)
{
    setCursor(Qt::PointingHandCursor);
    //添加红点
    AddRedPoint();
}

void StateWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

    const QString state_name = property("state").toString();
    const bool selected = state_name.startsWith("selected") || (_curstate == ClickLbState::Selected);
    const QString icon_path = ResolveSideIconPath(objectName(), selected);
    if (icon_path.isEmpty()) {
        return;
    }

    if (selected) {
        QRectF indicator_rect(2.0, (height() - 22.0) / 2.0, 4.0, 22.0);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#1890FF"));
        p.drawRoundedRect(indicator_rect, 2.0, 2.0);
    }

    QPixmap source(icon_path);
    if (source.isNull()) {
        return;
    }
    const QImage image = source.toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    const QRect crop = AlphaBoundingRect(image);
    const QPixmap trimmed = source.copy(crop);

    const QSize target_size(20, 20);
    QRect target_rect(QPoint(0, 0), target_size);
    target_rect.moveCenter(rect().center());
    QPixmap scaled = trimmed.scaled(target_size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    if (selected) {
        scaled = DarkenPixmap(scaled, 36);
    }
    const QRect draw_rect(target_rect.x() + (target_rect.width() - scaled.width()) / 2,
                          target_rect.y() + (target_rect.height() - scaled.height()) / 2,
                          scaled.width(), scaled.height());
    p.drawPixmap(draw_rect, scaled);

}

// 处理鼠标点击事件
void StateWidget::mousePressEvent(QMouseEvent* event)  {
    if (event->button() == Qt::LeftButton) {
        if(_curstate == ClickLbState::Selected){
            qDebug()<<"PressEvent , already to selected press: "<< _selected_press;
            //emit clicked();
            // 调用基类的mousePressEvent以保证正常的事件处理
            QWidget::mousePressEvent(event);
            return;
        }

        if(_curstate == ClickLbState::Normal){
            qDebug()<<"PressEvent , change to selected press: "<< _selected_press;
            _curstate = ClickLbState::Selected;
            setProperty("state",_selected_press);
            repolish(this);
            update();
        }

        return;
    }
    // 调用基类的mousePressEvent以保证正常的事件处理
    QWidget::mousePressEvent(event);
}

void StateWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        if(_curstate == ClickLbState::Normal){
            //qDebug()<<"ReleaseEvent , change to normal hover: "<< _normal_hover;
            setProperty("state",_normal_hover);
            repolish(this);
            update();

        }else{
            //qDebug()<<"ReleaseEvent , change to select hover: "<< _selected_hover;
            setProperty("state",_selected_hover);
            repolish(this);
            update();
        }
        emit clicked();
        return;
    }
    // 调用基类的mousePressEvent以保证正常的事件处理
    QWidget::mousePressEvent(event);
}

// 处理鼠标悬停进入事件
void StateWidget::enterEvent(QEvent* event) {
    // 在这里处理鼠标悬停进入的逻辑
    if(_curstate == ClickLbState::Normal){
         //qDebug()<<"enter , change to normal hover: "<< _normal_hover;
        setProperty("state",_normal_hover);
        repolish(this);
        update();

    }else{
         //qDebug()<<"enter , change to selected hover: "<< _selected_hover;
        setProperty("state",_selected_hover);
        repolish(this);
        update();
    }

    QWidget::enterEvent(event);
}

// 处理鼠标悬停离开事件
void StateWidget::leaveEvent(QEvent* event){
    // 在这里处理鼠标悬停离开的逻辑
    if(_curstate == ClickLbState::Normal){
        // qDebug()<<"leave , change to normal : "<< _normal;
        setProperty("state",_normal);
        repolish(this);
        update();

    }else{
        // qDebug()<<"leave , change to select normal : "<< _selected;
        setProperty("state",_selected);
        repolish(this);
        update();
    }
    QWidget::leaveEvent(event);
}

void StateWidget::SetState(QString normal, QString hover, QString press,
                            QString select, QString select_hover, QString select_press)
{
    _normal = normal;
    _normal_hover = hover;
    _normal_press = press;

    _selected = select;
    _selected_hover = select_hover;
    _selected_press = select_press;

    setProperty("state",normal);
    repolish(this);
}

ClickLbState StateWidget::GetCurState(){
    return _curstate;
}

void StateWidget::ClearState()
{
    _curstate = ClickLbState::Normal;
    setProperty("state",_normal);
    repolish(this);
    update();
}

void StateWidget::SetSelected(bool bselected)
{
    if(bselected){
        _curstate = ClickLbState::Selected;
        setProperty("state",_selected);
        repolish(this);
        update();
        return;
    }

    _curstate = ClickLbState::Normal;
    setProperty("state",_normal);
    repolish(this);
    update();
    return;

}

void StateWidget::AddRedPoint()
{
    //添加红点示意图
    _red_point = new QLabel();
    _red_point->setObjectName("red_point");
    QVBoxLayout* layout2 = new QVBoxLayout;
    _red_point->setAlignment(Qt::AlignCenter);
    layout2->addWidget(_red_point);
    layout2->setMargin(0);
    this->setLayout(layout2);
    _red_point->setVisible(false);
}

void StateWidget::ShowRedPoint(bool show)
{
    _red_point->setVisible(show);
}





