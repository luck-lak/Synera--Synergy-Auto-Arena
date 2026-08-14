#ifndef SHOPPANEL_H
#define SHOPPANEL_H

#include <QWidget>
#include <QList>
#include "shopgoods.h"

class QLabel;
class QPushButton;
class QVBoxLayout;
class Player;
class Game;
class HeroCard;
class QHBoxLayout;

class ShopPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ShopPanel(Player* player, Game* game, QWidget* parent = nullptr);

    void refreshGoods();
    void resetRefreshCount();

private slots:
    void onHeroClicked(const QString& heroName);
    void onDetailClicked(const ShopGoods& goods);
    void onRefreshClicked();

protected:
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    //void mousePressEvent(QMouseEvent* event) override;

private:
    Player* m_player;
    Game* m_game;

    QLabel* m_titleLabel;
    QPushButton* m_refreshButton;
    QPushButton* m_closeButton;
    QWidget* m_card;
    QWidget* m_cardsContainer;
    QList<HeroCard*> m_heroCards;
    QList<ShopGoods> m_goods;
    int m_refreshCount = 0;
};

#endif // SHOPPANEL_H
