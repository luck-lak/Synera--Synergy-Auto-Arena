#ifndef UNITINFOPANEL_H
#define UNITINFOPANEL_H

#include <QWidget>

class Unit;
class QLabel;
class Game;

class UnitInfoPanel : public QWidget
{
    Q_OBJECT

public:
    explicit UnitInfoPanel(Game* game, QWidget* parent = nullptr);

    void showUnit(Unit* unit);
    void hidePanel();
    Unit* currentUnit() const { return m_unit; }

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    void refresh();

    Game* m_game;
    Unit* m_unit = nullptr;
    QLabel* m_nameLabel;
    QLabel* m_statsLabel;
    QLabel* m_traitLabel;
    QLabel* m_equipLabel1;
    QLabel* m_equipLabel2;
};

#endif // UNITINFOPANEL_H
