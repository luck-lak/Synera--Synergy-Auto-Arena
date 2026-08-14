#include "gamewindow.h"
#include "core/game.h"
#include "playerinfopanel.h"
#include "unitinfopanel.h"
#include "shop/shoppanel.h"
#include "audio/soundmanager.h"
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>

GameWindow::GameWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_centralWidget(new QWidget(this))
    , m_mainLayout(new QHBoxLayout())
    , m_view(new QGraphicsView(this))
    , m_resetButton(new QPushButton("Reset", this))
    , m_fightButton(new QPushButton("Fight", this))
    , m_saveExitBtn(new QPushButton("保存并退出", this))
    , m_game(new Game(this))
{
    setupUI();
    // 不再自动 initialize，由调用方决定新游戏还是读档
}

GameWindow::~GameWindow() = default;

void GameWindow::startNewGame()
{
    m_game->initialize();
    SoundManager::instance()->startBgm();
}

bool GameWindow::loadGame()
{
    bool ok = m_game->loadFromFile();
    if (ok) {
        SoundManager::instance()->startBgm();
        refreshPlayerInfo();   // HP/金币/POP 标签 + 玩家面板刷新
        onPhaseChanged();      // Fight 按钮状态
        if (m_game->player()->isDead())
            onGameOver();      // 读档时血量≤0，恢复结束画面
    }
    return ok;
}

void GameWindow::onSaveAndExit()
{
    SoundManager::instance()->stopBgm();
    m_game->saveToFile();

    // 展示"已保存"1.5 秒后回到菜单
    m_resultLabel->setText(QStringLiteral("已保存"));
    m_resultLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: #4fc3f7;");
    QTimer::singleShot(1500, this, [this]() {
        m_resultLabel->setText("");
        hide();
        emit returnToMenu();
    });
}

void GameWindow::onResetButtonClicked()
{
    if (m_game) m_game->reset();
}

void GameWindow::onFightButtonClicked()
{
    if (!m_game) return;

    if (m_game->currentPhase() == Game::Phase::Resolve) {
        m_game->nextRound();
    } else if (m_game->currentPhase() == Game::Phase::Prep) {
        m_game->startCombat();
    }
}

void GameWindow::setupUI()
{
    connect(m_game, &Game::playerInfoChanged,
            this, &GameWindow::onPlayerStateChanged);
    connect(m_game, &Game::combatEnded,
            this, &GameWindow::onCombatEnded);
    connect(m_game, &Game::phaseChanged,
            this, &GameWindow::onPhaseChanged);
    connect(m_game, &Game::gameOver,
            this, &GameWindow::onGameOver);

    setCentralWidget(m_centralWidget);
    m_centralWidget->setLayout(m_mainLayout);

    setStyleSheet(R"(
        QMainWindow {
            background-color: #2b2b2b;
        }
        QWidget {
            background-color: #2b2b2b;
            color: #f2f2f2;
        }
        QPushButton {
            background-color: #2f2f2f;
            color: #f2f2f2;
            border: 1px solid #565656;
            border-radius: 4px;
            padding: 6px 14px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #3a3a3a;
        }
        QPushButton:pressed {
            background-color: #242424;
        }
    )");

    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setDragMode(QGraphicsView::NoDrag);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    m_view->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    m_view->setMouseTracking(true);
    m_view->viewport()->setMouseTracking(true);

    QWidget* leftContainer = new QWidget(m_centralWidget);
    QVBoxLayout* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    leftLayout->addWidget(m_view, 1);

    /* Fight 按钮 */
    QWidget* fightBar = new QWidget(this);
    QHBoxLayout* fightLayout = new QHBoxLayout(fightBar);
    fightLayout->setContentsMargins(0, 0, 0, 0);
    fightLayout->addWidget(m_fightButton);
    fightLayout->addStretch();
    leftLayout->addWidget(fightBar);
    connect(m_fightButton, &QPushButton::clicked,
            this, &GameWindow::onFightButtonClicked);

    /* Reset + 保存退出 */
    QWidget* controlBar = new QWidget(this);
    QHBoxLayout* controlLayout = new QHBoxLayout(controlBar);
    controlLayout->setContentsMargins(0, 0, 0, 0);
    controlLayout->addWidget(m_resetButton);
    controlLayout->addWidget(m_saveExitBtn);
    controlLayout->addStretch();
    leftLayout->addWidget(controlBar);

    m_saveExitBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #4a3030;
            color: #f0c0c0;
            border: 1px solid #7a5050;
        }
        QPushButton:hover { background-color: #5a4040; }
    )");
    connect(m_resetButton, &QPushButton::clicked, this, &GameWindow::onResetButtonClicked);
    connect(m_saveExitBtn, &QPushButton::clicked, this, &GameWindow::onSaveAndExit);

    /* 底部信息条 */
    QWidget* infoBar = new QWidget(this);
    QHBoxLayout* infoLayout = new QHBoxLayout(infoBar);
    infoLayout->setContentsMargins(0, 0, 0, 0);

    m_hpLabel = new QLabel(QString("HP:%1").arg(m_game->player()->Hp()), this);
    m_goldLabel = new QLabel(QString("GOLD:%1").arg(m_game->player()->Gold()), this);
    m_popLabel = new QLabel(
        QString("POP: %1/%2").arg(m_game->player()->PopulationUsed())
                             .arg(m_game->player()->PopulationCap()),
        this);

    m_resultLabel = new QLabel("", this);
    m_resultLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #ffcc00;");

    infoLayout->addWidget(m_hpLabel);
    infoLayout->addWidget(m_goldLabel);
    infoLayout->addWidget(m_popLabel);
    infoLayout->addWidget(m_resultLabel);
    infoLayout->addStretch();
    leftLayout->addWidget(infoBar);

    m_view->setScene(m_game->scene());

    /* 左侧单位详情面板 */
    m_unitInfoPanel = new UnitInfoPanel(m_game, m_centralWidget);
    m_mainLayout->addWidget(m_unitInfoPanel);
    m_mainLayout->addWidget(leftContainer);

    /* 右侧玩家面板 */
    m_playerPanel = new PlayerInfoPanel(m_game->player(), m_game, m_centralWidget);
    m_mainLayout->addWidget(m_playerPanel);

    connect(m_game, &Game::unitSelected, this, &GameWindow::onUnitSelected);

    /* 商店覆盖层 */
    m_shopPanel = new ShopPanel(m_game->player(), m_game, this);
    m_shopPanel->refreshGoods();
    connect(m_game, &Game::roundChanged, this, [this]() {
        m_shopPanel->resetRefreshCount();
        m_shopPanel->refreshGoods();
    });
    connect(m_playerPanel, &PlayerInfoPanel::shopRequested, this, [this]() {
        m_shopPanel->show();
        m_shopPanel->raise();
    });
}

void GameWindow::onPlayerStateChanged() { refreshPlayerInfo(); }

void GameWindow::refreshPlayerInfo()
{
    Player* p = m_game->player();
    m_hpLabel->setText(QString("HP:%1").arg(p->Hp()));
    m_goldLabel->setText(QString("Gold:%1").arg(p->Gold()));
    m_popLabel->setText(QString("POP:%1/%2").arg(p->PopulationUsed()).arg(p->PopulationCap()));
    m_playerPanel->refresh();
}

void GameWindow::onCombatEnded(bool playerWon, int value)
{
    if (playerWon) {
        m_resultLabel->setText(QString("Victory! +%1 Gold").arg(value));
        m_resultLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #4fc3f7;");
    } else {
        m_resultLabel->setText(QString("Defeat! -%1 HP").arg(value));
        m_resultLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #ef5350;");
    }
}

void GameWindow::onPhaseChanged()
{
    Game::Phase phase = m_game->currentPhase();
    if (phase == Game::Phase::Prep) {
        m_fightButton->setText("Fight");
        m_resultLabel->setText("");
    } else if (phase == Game::Phase::Combat) {
        m_fightButton->setText("Fighting...");
        m_resultLabel->setText("");
    } else if (phase == Game::Phase::Resolve) {
        m_fightButton->setText(QString("Next Round (%1)").arg(m_game->round() + 1));
    }
}

void GameWindow::onUnitSelected(Unit* unit)
{
    if (!m_unitInfoPanel) return;
    if (m_unitInfoPanel->currentUnit() == unit && m_unitInfoPanel->isVisible()) {
        m_unitInfoPanel->hidePanel();
        return;
    }
    m_unitInfoPanel->showUnit(unit);
}

void GameWindow::onGameOver()
{
    m_resultLabel->setText(QStringLiteral("游戏结束"));
    m_resultLabel->setStyleSheet("font-weight: bold; font-size: 20px; color: #ef5350;");
    m_fightButton->setEnabled(false);
    m_resetButton->setEnabled(false);
}
