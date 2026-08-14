#ifndef HERODETAILPANEL_H
#define HERODETAILPANEL_H

#include <QWidget>

class QLabel;
class ShopGoods;

class HeroDetailPanel : public QWidget
{
    Q_OBJECT

public:
    explicit HeroDetailPanel(const ShopGoods& goods, QWidget* parent = nullptr);
};

#endif // HERODETAILPANEL_H
