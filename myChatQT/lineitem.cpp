#include "lineitem.h"
#include "ui_lineitem.h"

LineItem::LineItem(QWidget *parent) :
    ListItemBase(parent),
    ui(new Ui::LineItem)
{
    ui->setupUi(this);
    SetItemType(ListItemType::LINE_ITEM);
}

LineItem::~LineItem()
{
    delete ui;
}

QSize LineItem::sizeHint() const
{
   int width = 1000;
   if (parentWidget()) {
       width = parentWidget()->width() - 10;
       if (width < 250) {
           width = 250;
       }
   }
   return QSize(width, 8);
}
