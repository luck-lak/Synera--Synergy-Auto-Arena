#ifndef GAMEWINDOW_H
#define GAMEWINDOW_H

#include <QMainWindow>
#include <QLabel>

class Game;
class Unit;
class QGraphicsView;
class QPushButton;
class QHBoxLayout;
class QLabel;
class PlayerInfoPanel;
class ShopPanel;
class UnitInfoPanel;

class GameWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit GameWindow(QWidget* parent = nullptr);
    ~GameWindow();

    void startNewGame();                 /* 新游戏：initialize */
    bool loadGame();                     /* 读档：loadFromFile，失败返回 false */

signals:
    void returnToMenu();

private slots:
    void onResetButtonClicked();
    void onFightButtonClicked();
    void onSaveAndExit();
    void onCombatEnded(bool playerWon, int value);
    void onPhaseChanged();
    void onUnitSelected(Unit* unit);
    void onGameOver();

private:
    void setupUI();

    QWidget* m_centralWidget;
    QHBoxLayout* m_mainLayout;
    QGraphicsView* m_view;
    QPushButton* m_resetButton;
    QPushButton* m_fightButton;
    QPushButton* m_saveExitBtn;
    Game* m_game;

private:
    QLabel* m_hpLabel;
    QLabel* m_goldLabel;
    QLabel* m_popLabel;
    QLabel* m_resultLabel;
    PlayerInfoPanel* m_playerPanel;
    UnitInfoPanel* m_unitInfoPanel;
    ShopPanel* m_shopPanel;

private:
    void refreshPlayerInfo();
private slots:
    void onPlayerStateChanged();
};

#endif // GAMEWINDOW_H
