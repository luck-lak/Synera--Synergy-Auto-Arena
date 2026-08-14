#include "mainmenu.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QFile>
#include <QStandardPaths>
#include <QDir>

static QString savePath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Synera";
    QDir().mkpath(dir);
    return dir + "/save.json";
}

MainMenu::MainMenu(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("Synera"));
    setFixedSize(400, 350);

    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(20);

    /* 标题 */
    auto* title = new QLabel(QStringLiteral("Synera"), this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 48px; font-weight: bold; color: #f1c40f;");
    layout->addWidget(title);

    auto* subtitle = new QLabel(QStringLiteral("— 自走棋 —"), this);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("font-size: 14px; color: #888;");
    layout->addWidget(subtitle);

    layout->addSpacing(30);

    /* 新游戏 */
    m_newGameBtn = new QPushButton(QStringLiteral("新游戏"), this);
    m_newGameBtn->setFixedSize(200, 50);
    m_newGameBtn->setStyleSheet(R"(
        QPushButton {
            font-size: 18px; font-weight: bold;
            background-color: #3a5a3a;
            border: 1px solid #5a8a5a;
            border-radius: 6px;
            color: #c0f0c0;
        }
        QPushButton:hover { background-color: #4a7a4a; }
    )");
    connect(m_newGameBtn, &QPushButton::clicked, this, &MainMenu::newGame);
    layout->addWidget(m_newGameBtn, 0, Qt::AlignCenter);

    /* 继续游戏 */
    m_continueBtn = new QPushButton(QStringLiteral("继续游戏"), this);
    m_continueBtn->setFixedSize(200, 50);
    m_continueBtn->setStyleSheet(R"(
        QPushButton {
            font-size: 18px; font-weight: bold;
            background-color: #3a3a5a;
            border: 1px solid #5a5a8a;
            border-radius: 6px;
            color: #c0c0f0;
        }
        QPushButton:hover { background-color: #4a4a7a; }
        QPushButton:disabled {
            background-color: #2a2a2a;
            border: 1px solid #444;
            color: #555;
        }
    )");
    connect(m_continueBtn, &QPushButton::clicked, this, &MainMenu::continueGame);
    layout->addWidget(m_continueBtn, 0, Qt::AlignCenter);

    updateContinueButton();

    setStyleSheet("background-color: #1e1e1e;");
}

void MainMenu::updateContinueButton()
{
    m_continueBtn->setEnabled(QFile::exists(savePath()));
}
