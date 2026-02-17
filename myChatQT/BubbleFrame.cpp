#include "BubbleFrame.h"

#include <QPainter>
#include <QPainterPath>

namespace {
constexpr qreal kTailWidth = 9.0;
constexpr qreal kBubbleRadius = 11.0;
constexpr qreal kTailTopOffset = 12.0;
}

BubbleFrame::BubbleFrame(ChatRole role, QWidget *parent)
    : QFrame(parent)
    , m_role(role)
    , m_margin(4)
{
    m_pHLayout = new QHBoxLayout();
    if (m_role == ChatRole::Self) {
        m_pHLayout->setContentsMargins(m_margin, m_margin, static_cast<int>(kTailWidth) + m_margin, m_margin);
    } else {
        m_pHLayout->setContentsMargins(static_cast<int>(kTailWidth) + m_margin, m_margin, m_margin, m_margin);
    }

    m_pHLayout->setSpacing(0);
    setLayout(m_pHLayout);
}

void BubbleFrame::setMargin(int margin)
{
    Q_UNUSED(margin);
}

void BubbleFrame::setWidget(QWidget *w)
{
    if (m_pHLayout->count() > 0) {
        return;
    }
    m_pHLayout->addWidget(w);
}

void BubbleFrame::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    QColor fillColor;
    QColor borderColor;
    QRectF bubbleRect;

    if (m_role == ChatRole::Self) {
        fillColor = QColor("#2f7de1");
        borderColor = QColor("#2360b0");
        bubbleRect = QRectF(0.5, 0.5, width() - kTailWidth - 1.0, height() - 1.0);
    } else {
        fillColor = QColor("#ffffff");
        borderColor = QColor("#dbe5f1");
        bubbleRect = QRectF(kTailWidth + 0.5, 0.5, width() - kTailWidth - 1.0, height() - 1.0);
    }

    const qreal halfTail = kTailWidth / 2.0;
    const qreal minTailTop = bubbleRect.top() + 2.0;
    const qreal maxTailTop = bubbleRect.bottom() - kTailWidth - 2.0;
    const qreal desiredTailTop = bubbleRect.top() + kTailTopOffset;
    const qreal tailTop = qMax(minTailTop, qMin(desiredTailTop, maxTailTop));
    const qreal tailMid = tailTop + halfTail;
    const qreal tailBottom = tailTop + kTailWidth;

    QPainterPath mainPath;
    mainPath.addRoundedRect(bubbleRect, kBubbleRadius, kBubbleRadius);

    QPainterPath tailPath;
    if (m_role == ChatRole::Self) {
        const qreal rightX = bubbleRect.right();
        tailPath.moveTo(rightX, tailTop);
        tailPath.lineTo(rightX + kTailWidth, tailMid);
        tailPath.lineTo(rightX, tailBottom);
    } else {
        const qreal leftX = bubbleRect.left();
        tailPath.moveTo(leftX, tailTop);
        tailPath.lineTo(leftX - kTailWidth, tailMid);
        tailPath.lineTo(leftX, tailBottom);
    }
    tailPath.closeSubpath();

    const QPainterPath bubblePath = mainPath.united(tailPath);
    painter.setPen(QPen(borderColor, 1));
    painter.setBrush(fillColor);
    painter.drawPath(bubblePath);
}
