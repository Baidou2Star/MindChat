#include "ChatItemBase.h"
#include <QFont>
#include <QVBoxLayout>
#include "BubbleFrame.h"
ChatItemBase::ChatItemBase(ChatRole role, QWidget *parent)
    : QWidget(parent)
    , m_role(role)
{
    m_pNameLabel    = new QLabel();
    m_pNameLabel->setObjectName("chat_user_name");
    QFont font("Microsoft YaHei UI");
    font.setPointSize(12);
    m_pNameLabel->setFont(font);
    m_pNameLabel->setFixedHeight(24);
    m_pIconLabel    = new QLabel();
    m_pIconLabel->setScaledContents(true);
    m_pIconLabel->setFixedSize(40, 40);
    m_pBubble       = new QWidget();
    QGridLayout *pGLayout = new QGridLayout();
    pGLayout->setVerticalSpacing(4);
    pGLayout->setHorizontalSpacing(6);
    pGLayout->setContentsMargins(8, 4, 8, 4);
    QSpacerItem*pSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    //添加状态图标控件
    m_pStatusLabel = new QLabel();
    m_pStatusLabel->setFixedSize(15,15);
    m_pStatusLabel->setScaledContents(true);

    if(m_role == ChatRole::Self)
    {
        m_pNameLabel->setContentsMargins(0,0,8,0);
        m_pNameLabel->setAlignment(Qt::AlignRight);
        pGLayout->addWidget(m_pNameLabel, 0,2, 1,1);
        pGLayout->addWidget(m_pIconLabel, 0, 3, 2,1, Qt::AlignTop);
        pGLayout->addItem(pSpacer, 1, 0, 1, 1);
        pGLayout->addWidget(m_pStatusLabel,1,1,1,1,Qt::AlignCenter);
        pGLayout->addWidget(m_pBubble, 1,2, 1,1, Qt::AlignRight | Qt::AlignTop);
        pGLayout->setColumnStretch(0, 1);
        pGLayout->setColumnStretch(1, 0); // status 图标(固定大小)
        pGLayout->setColumnStretch(2, 0); // 名字 + 气泡按内容宽度
        pGLayout->setColumnStretch(3, 0);
    }else{
        m_pNameLabel->setContentsMargins(8,0,0,0);
        m_pNameLabel->setAlignment(Qt::AlignLeft);
        pGLayout->addWidget(m_pIconLabel, 0, 0, 2,1, Qt::AlignTop);
        pGLayout->addWidget(m_pNameLabel, 0,1, 1,1);
        pGLayout->addWidget(m_pBubble, 1,1, 1,1, Qt::AlignLeft | Qt::AlignTop);
        pGLayout->addItem(pSpacer, 1, 2, 1, 1);
        pGLayout->setColumnStretch(0, 0);
        pGLayout->setColumnStretch(1, 0); // 名字 + 气泡按内容宽度
        pGLayout->setColumnStretch(2, 1);
    }
    this->setLayout(pGLayout);
}

void ChatItemBase::setUserName(const QString &name)
{
    m_pNameLabel->setText(name);
}

void ChatItemBase::setUserIcon(const QPixmap &icon)
{
    m_pIconLabel->setPixmap(icon);
}

void ChatItemBase::setWidget(QWidget *w)
{
   QGridLayout *pGLayout = (qobject_cast<QGridLayout *>)(this->layout());
   pGLayout->replaceWidget(m_pBubble, w);
   if (m_role == ChatRole::Self) {
       pGLayout->setAlignment(w, Qt::AlignRight | Qt::AlignTop);
   } else {
       pGLayout->setAlignment(w, Qt::AlignLeft | Qt::AlignTop);
   }
   w->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
   delete m_pBubble;
   m_pBubble = w;
}

void ChatItemBase::setStatus(int status)
{
    if(status == MsgStatus::UN_READ){
        m_pStatusLabel->setPixmap(QPixmap(":/res/unread.png"));
        return ;
    }

    if(status == MsgStatus::SEND_FAILED){
        m_pStatusLabel->setPixmap(QPixmap(":/res/send_fail.png"));
        return ;
    }

    if(status == MsgStatus::READED){
        m_pStatusLabel->setPixmap(QPixmap(":/res/readed.png"));
        return ;
    }
}


QLabel* ChatItemBase::getIconLabel() {
    return m_pIconLabel;
}

QWidget* ChatItemBase::getBubble() {
    return m_pBubble;
}
