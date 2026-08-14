#include <QApplication>
#include "gui/mainmenu.h"
#include "gui/gamewindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainMenu menu;
    GameWindow* window = nullptr;

    /* "新游戏" → 每次重建窗口，避免旧数据残留 */
    QObject::connect(&menu, &MainMenu::newGame, [&]() {
        delete window;                                   /* 清除旧窗口 */
        window = new GameWindow();
        QObject::connect(window, &GameWindow::returnToMenu, [&]() {
            menu.show();
        });
        window->startNewGame();
        window->show();
        menu.hide();
    });

    /* "继续游戏" → 同样重建，从文件读取 */
    QObject::connect(&menu, &MainMenu::continueGame, [&]() {
        delete window;
        window = new GameWindow();
        QObject::connect(window, &GameWindow::returnToMenu, [&]() {
            menu.show();
        });
        if (!window->loadGame()) {
            delete window;                              /* 读档失败，清理 */
            window = nullptr;
            return;
        }
        window->show();
        menu.hide();
    });

    menu.show();
    return app.exec();
}
