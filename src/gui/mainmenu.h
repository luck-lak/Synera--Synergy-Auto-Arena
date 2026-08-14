#ifndef MAINMENU_H
#define MAINMENU_H

#include <QWidget>

class QPushButton;

class MainMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenu(QWidget* parent = nullptr);

signals:
    void newGame();
    void continueGame();

private:
    void updateContinueButton();

    QPushButton* m_newGameBtn;
    QPushButton* m_continueBtn;
};

#endif // MAINMENU_H
