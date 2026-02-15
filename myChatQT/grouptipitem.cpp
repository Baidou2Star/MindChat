#include "grouptipitem.h"
#include "ui_grouptipitem.h"

GroupTipItem::GroupTipItem(QWidget *parent) :ListItemBase (parent),_tip(""),
    ui(new Ui::GroupTipItem)
{
    ui->setupUi(this);
    setMinimumWidth(0);
    setMaximumWidth(QWIDGETSIZE_MAX);
    SetItemType(ListItemType::GROUP_TIP_ITEM);
}

GroupTipItem::~GroupTipItem()
{
    delete ui;
}


QSize GroupTipItem::sizeHint() const
{
    int width = 1000;
    if (parentWidget()) {
        width = parentWidget()->width() - 10;
        if (width < 250) {
            width = 250;
        }
    }
    return QSize(width, 28);
}

void GroupTipItem::SetGroupTip(QString str)
{
    ui->label->setText(str);
}
