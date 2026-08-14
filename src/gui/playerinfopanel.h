#ifndef PLAYERINFOPANEL_H
#define PLAYERINFOPANEL_H

#include <QWidget>

class QLabel;
class QPushButton;
class QProgressBar;
class Player;
class Game;

class PlayerInfoPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PlayerInfoPanel(Player* player, Game* game, QWidget* parent = nullptr);

    void refresh();

signals:
    void shopRequested();

private:
    Player* m_player;
    Game* m_game;

    QLabel* m_titleLabel;
    QLabel* m_hpLabel;
    QProgressBar* m_hpBar;
    QLabel* m_goldLabel;
    QLabel* m_levelLabel;
    QLabel* m_roundLabel;
    QLabel* m_traitLabel1;
    QLabel* m_traitLabel2;
    QLabel* m_traitLabel3;
    QLabel* m_traitLabel4;
    QPushButton* m_levelUpButton;
    QPushButton* m_shopButton;
};

#endif // PLAYERINFOPANEL_H
