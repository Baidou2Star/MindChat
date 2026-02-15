#include "TextBubble.h"

#include <QAbstractTextDocumentLayout>
#include <QDebug>
#include <QEvent>
#include <QFont>
#include <QFontMetricsF>
#include <QTextDocument>
#include <QTextOption>
#include <QTimer>
#include <QtMath>

#include "global.h"

TextBubble::TextBubble(ChatRole role, const QString &text, QWidget *parent)
    : BubbleFrame(role, parent)
{
    m_pTextEdit = new QTextEdit();
    m_pTextEdit->setReadOnly(true);
    m_pTextEdit->setFrameStyle(QFrame::NoFrame);
    m_pTextEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pTextEdit->setContentsMargins(0, 0, 0, 0);
    m_pTextEdit->installEventFilter(this);
    m_pTextEdit->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_pTextEdit->document()->setDocumentMargin(3);

    // QTextEdit 行盒模型会让底部视觉留白略大，做轻量光学校正。
    const QMargins bubble_margins = layout()->contentsMargins();
    layout()->setContentsMargins(
        bubble_margins.left(),
        bubble_margins.top() + 1,
        bubble_margins.right(),
        qMax(0, bubble_margins.bottom() - 1)
    );

    QFont font("Microsoft YaHei UI");
    font.setPointSize(12);
    m_pTextEdit->setFont(font);
    m_pTextEdit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    setPlainText(text);
    setWidget(m_pTextEdit);
    setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    initStyleSheet();
}

bool TextBubble::eventFilter(QObject *o, QEvent *e)
{
    if (m_pTextEdit == o && e->type() == QEvent::Paint) {
        adjustTextHeight();
    }
    return BubbleFrame::eventFilter(o, e);
}

void TextBubble::setPlainText(const QString &text)
{
    m_pTextEdit->setPlainText(text);

    QTextDocument *doc = m_pTextEdit->document();
    doc->setTextWidth(-1);
    doc->adjustSize();

    // 控制单条消息最大宽度，避免气泡被布局拉得过宽。
    const qreal max_text_width = 560.0;
    const qreal natural_width = qCeil(doc->idealWidth());
    const qreal text_width = qMin(max_text_width, natural_width);
    doc->setTextWidth(text_width);
    doc->adjustSize();

    const QSizeF doc_size = doc->documentLayout()->documentSize();
    const int edit_width = qMax(1, static_cast<int>(qCeil(doc_size.width())));
    m_pTextEdit->setFixedWidth(edit_width);

    const QMargins margins = layout()->contentsMargins();
    setFixedWidth(edit_width + margins.left() + margins.right());
    adjustTextHeight();
}

void TextBubble::adjustTextHeight()
{
    QTextDocument *doc = m_pTextEdit->document();
    doc->adjustSize();

    const qreal text_height = doc->documentLayout()->documentSize().height();
    const int edit_height = qMax(1, static_cast<int>(qCeil(text_height)));
    m_pTextEdit->setFixedHeight(edit_height);

    const QMargins margins = layout()->contentsMargins();
    setFixedHeight(edit_height + margins.top() + margins.bottom());
}

void TextBubble::initStyleSheet()
{
    if (role() == ChatRole::Self) {
        m_pTextEdit->setStyleSheet(
            "QTextEdit{"
            "background:transparent;"
            "border:none;"
            "color:#ffffff;"
            "padding:0px;"
            "margin:0px;"
            "}"
        );
    } else {
        m_pTextEdit->setStyleSheet(
            "QTextEdit{"
            "background:transparent;"
            "border:none;"
            "color:#203049;"
            "padding:0px;"
            "margin:0px;"
            "}"
        );
    }
}
