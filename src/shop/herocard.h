#ifndef HEROCARD_H
#define HEROCARD_H

#include <QWidget>
#include "shopgoods.h"

class QLabel;
class HeroDetailPanel;

class HeroCard : public QWidget
{
    Q_OBJECT

public:
    explicit HeroCard(const ShopGoods& goods, QWidget* parent = nullptr);

    void setPurchased(bool purchased);
    QString heroName() const;

signals:
    void clicked(const QString& heroName);
    void detailClicked(const ShopGoods& goods);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    ShopGoods m_goods;
    QLabel* m_nameLabel;
    QLabel* m_costLabel;
    QLabel* m_factionLabel;
    QLabel* m_imageLabel;
    bool m_purchased = false;

    void loadSprite();
    QString spritePath() const;
};

#endif // HEROCARD_H
