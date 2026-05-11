#include "mainwindow.h"
#include "gamewidget.h"
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWidget>


#ifndef TDIMENSIONMC_ASSET_ROOT
#define TDIMENSIONMC_ASSET_ROOT "D:/develop/Qtproject/TdimensionMC/assets"
#endif

namespace {

 //获取指定资源文件的绝对路径, 优先在应用程序所在目录下的 assets 文件夹查找，
 //若不存在则回退到编译时指定的默认资源根目录。
QString assetPath(const char *relativePath)
{
    const QString relative = QString::fromLatin1(relativePath);
    const QString appAssetPath = QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/assets/") + relative);
    if (QFileInfo::exists(appAssetPath)) {
        return appAssetPath;
    }
    return QString::fromUtf8(TDIMENSIONMC_ASSET_ROOT) + QLatin1Char('/') + relative;
}


QString utf8Hex(const char *hex)
{
    QByteArray bytes;
    for (int i = 0; hex[i] != '\0' && hex[i + 1] != '\0'; i += 2) {
        bool ok = false;
        const char pair[] = { hex[i], hex[i + 1], '\0' };
        const char value = static_cast<char>(QByteArray(pair).toUInt(&ok, 16));
        if (ok) {
            bytes.append(value);
        }
    }
    return QString::fromUtf8(bytes);
}


//创建像素风格的自定义字体,字体像素大小
//使用 "Minecraft AE" 字体族，设置像素大小、加粗属性，整体呈现像素化 UI 风格。
QFont pixelFont(int pixelSize, bool bold = false)
{
    QFont font(QStringLiteral("Minecraft AE"));
    font.setStyleHint(QFont::Monospace);
    font.setPixelSize(pixelSize);
    font.setBold(bold);
    font.setWeight(bold ? QFont::Black : QFont::DemiBold);
    return font;
}


//生成像素边框风格的样式表
// borderWidth是边框宽度,padding是内边距
//生成浅色文字、深黑背景、青色边框的样式，用于标签等控件。
QString pixelLabelStyle(int borderWidth = 4, int padding = 14)
{
    return QStringLiteral("color: #f5f5f5; background: #050505; border: %1px solid #54fff0; padding: %2px;")
    .arg(borderWidth)
        .arg(padding);
}


class MenuBackground : public QWidget
{
public:
    explicit MenuBackground(QWidget *parent = nullptr)
        : QWidget(parent)
        , m_background(assetPath("blocks/start.png"))
    {
        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        if (!m_background.isNull()) {
            painter.drawPixmap(rect(), m_background);
        } else {
            // 若图片加载失败则填充纯色作为备用
            painter.fillRect(rect(), QColor(42, 32, 24));
        }
        // 覆盖半透明黑色层，使背景略微压暗
        painter.fillRect(rect(), QColor(0, 0, 0, 92));
    }

private:
    QPixmap m_background;
};
class PixelTitle : public QWidget
{
public:
    explicit PixelTitle(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setMinimumHeight(260);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);
        const QString title = QStringLiteral("DIMENSIONAL");
        const QString subtitle = QStringLiteral("MYSTERY");
        const int titleSize = qMax(64, width() / 12);
        const int subtitleSize = qMax(42, width() / 20);
        const QRect titleRect(0, 18, width(), 130);
        const QRect subtitleRect(0, 125, width(), 75);

        // 标题：多层偏移绘制产生立体阴影
        painter.setFont(pixelFont(titleSize, true));
        painter.setPen(QPen(QColor(0, 0, 0), 8));
        painter.drawText(titleRect.translated(8, 10), Qt::AlignCenter, title);
        painter.setPen(QColor(126, 126, 126));
        painter.drawText(titleRect.translated(-4, -6), Qt::AlignCenter, title);
        painter.setPen(QColor(68, 68, 68));
        painter.drawText(titleRect.translated(0, 16), Qt::AlignCenter, title);
        painter.setPen(QColor(226, 218, 212));
        painter.drawText(titleRect, Qt::AlignCenter, title);

        // 副标题
        painter.setFont(pixelFont(subtitleSize, true));
        painter.setPen(QPen(QColor(0, 0, 0), 6));
        painter.drawText(subtitleRect.translated(5, 7), Qt::AlignCenter, subtitle);
        painter.setPen(QColor(120, 120, 120));
        painter.drawText(subtitleRect.translated(-3, -4), Qt::AlignCenter, subtitle);
        painter.setPen(QColor(222, 216, 212));
        painter.drawText(subtitleRect, Qt::AlignCenter, subtitle);
    }
};


 // @brief 辅助函数：创建像素风格的按钮
 //text 按钮显示文本 parent是父控件，minWidth是最小宽度
 //设置按钮的固定样式表，包括普通态、悬停态、按下态，字体使用 Microsoft YaHei，光标变为手型。
QPushButton *createPixelButton(const QString &text, QWidget *parent, int minWidth = 360)
{
    auto *button = new QPushButton(text, parent);
    button->setMinimumSize(minWidth, 62);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    button->setStyleSheet(QStringLiteral(
        "QPushButton {"
        "  color: #f5f5f5;"
        "  background: #050505;"
        "  border: 4px solid #54fff0;"
        "  font: 900 30px 'Microsoft YaHei';"
        "  padding: 8px 22px;"
        "  letter-spacing: 0px;"
        "}"
        "QPushButton:hover {"
        "  color: #ffffa0;"
        "  background: #101010;"
        "  border-color: #8effff;"
        "}"
        "QPushButton:pressed {"
        "  color: #c8ffff;"
        "  background: #000000;"
        "  border-color: #35cfc4;"
        "}"));
    return button;
}

QLabel *createHelpLabel(const QString &text, QWidget *parent, int size = 25)
{
    auto *label = new QLabel(text, parent);
    label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    label->setWordWrap(false);
    label->setFont(pixelFont(size, true));
    label->setStyleSheet(pixelLabelStyle(4, 16));
    label->setMinimumHeight(74);
    return label;
}
} // 匿名命名空间结束


//构造主窗口，搭建三个主要界面：开始菜单、帮助界面、游戏界面
//开始菜单包含像素绘制标题、启动/帮助/选项/退出按钮，帮助界面展示工具介绍并有一个返回按钮。
 //利用信号与槽负责切换界面。
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_gameWidget(nullptr)
    , m_stack(new QStackedWidget(this))
    , m_startScreen(new MenuBackground(this))
    , m_helpScreen(new MenuBackground(this))
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 开始菜单
    auto *startLayout = new QVBoxLayout(m_startScreen);
    startLayout->setContentsMargins(90, 44, 90, 44);
    startLayout->addWidget(new PixelTitle(m_startScreen));
    startLayout->addSpacing(18);

    // 按钮列居中摆放
    auto *buttonColumn = new QWidget(m_startScreen);
    auto *buttonLayout = new QVBoxLayout(buttonColumn);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(26);
    buttonLayout->setAlignment(Qt::AlignCenter);

    // 创建两个主按钮（文本为十六进制中文编码）
    auto *startButton = createPixelButton(utf8Hex("e5bc80e5a78be6b8b8e6888f"), buttonColumn, 720);
    auto *helpButton = createPixelButton(utf8Hex("e5b8aee58aa9"), buttonColumn, 720);
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(helpButton);

    startLayout->addWidget(buttonColumn, 0, Qt::AlignHCenter);
    startLayout->addStretch(1);

    // 底部按钮栏：选项和退出
    auto *bottomLayout = new QHBoxLayout;
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    auto *optionsButton = createPixelButton(utf8Hex("e98089e9a1b92e2e2e"), m_startScreen, 360);
    auto *exitButton = createPixelButton(utf8Hex("e98080e587bae6b8b8e6888f"), m_startScreen, 360);
    bottomLayout->addStretch(1);
    bottomLayout->addWidget(optionsButton);
    bottomLayout->addSpacing(32);
    bottomLayout->addWidget(exitButton);
    bottomLayout->addStretch(1);
    startLayout->addLayout(bottomLayout);

    // ----- 帮助界面布局 -----
    auto *helpLayout = new QVBoxLayout(m_helpScreen);
    helpLayout->setContentsMargins(72, 36, 72, 36);
    auto *helpTitle = new QLabel(utf8Hex("e5b7a5e585b7e6a08fe789a9e59381e4bb8be7bb8d"), m_helpScreen);
    helpTitle->setAlignment(Qt::AlignCenter);
    helpTitle->setFont(pixelFont(46, true));
    helpTitle->setStyleSheet(QStringLiteral("color: #f5f5f5;"));
    helpLayout->addWidget(helpTitle);
    helpLayout->addSpacing(28);

    // 工具介绍标签列表
    auto *itemsColumn = new QVBoxLayout;
    itemsColumn->setContentsMargins(0, 0, 0, 0);
    itemsColumn->setSpacing(22);
    itemsColumn->addWidget(createHelpLabel(utf8Hex("e696b9e59d97efbc9ae58fb3e994aee694bee7bdaee6b3a5e59c9fe696b9e59d97efbc8ce794a8e69da5e690ade8b7afe38081e5b081e8b7afe68896e4b8b4e697b6e59eabe8849a"), m_helpScreen, 24));
    itemsColumn->addWidget(createHelpLabel(utf8Hex("e5bc93efbc9ae58fb3e994aee59091e9bca0e6a087e696b9e59091e5b084e7aeadefbc8ce98082e59088e8bf9ce8b79de7a6bbe694bbe587bb"), m_helpScreen, 24));
    itemsColumn->addWidget(createHelpLabel(utf8Hex("e58991efbc9ae58fb3e994aee68ca5e7a08defbc8ce8bf91e68898e694bbe587bbe5b9b6e58fafe68993e6b688e6958ce696b9e9a39ee8a18ce789a9"), m_helpScreen, 24));
    itemsColumn->addWidget(createHelpLabel(utf8Hex("e99590efbc9ae98089e68ba9e5908ee68c89e4bd8fe5b7a6e994aee68c96e68e98e79fb3e5a4b4e38081e79fbfe79fb3e5928ce69ca8e5a4b4"), m_helpScreen, 24));
    itemsColumn->addWidget(createHelpLabel(utf8Hex("e98791e88bb9e69e9cefbc9ae58fb3e994aee9a39fe794a8efbc8ce681a2e5a48de7949fe591bde5b9b6e88eb7e5be97e4b8b4e697b6e98791e889b2e68aa4e79bbe"), m_helpScreen, 24));
    helpLayout->addLayout(itemsColumn);
    helpLayout->addStretch(1);

    auto *doneButton = createPixelButton(utf8Hex("e5ae8ce68890"), m_helpScreen, 720);
    helpLayout->addWidget(doneButton, 0, Qt::AlignHCenter);

    // 将两个界面加入到 StackedWidget
    m_stack->addWidget(m_startScreen);   // 索引 0
    m_stack->addWidget(m_helpScreen);    // 索引 1
    setCentralWidget(m_stack);
    setMinimumSize(1280, 860);
    resize(1440, 900);
    setWindowTitle(QStringLiteral("Dimensional Mystery"));

    // 信号连接
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startGame);
    connect(optionsButton, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(doneButton, &QPushButton::clicked, this, &MainWindow::showStartScreen);
    connect(exitButton, &QPushButton::clicked, qApp, &QApplication::quit);

    // 初始焦点置于开始界面，以便接收键盘事件
    m_startScreen->setFocus();
}

MainWindow::~MainWindow()
{
    delete ui;
}


 //切换到游戏界面。首次调用时创建 GameWidget 对象并添加到 StackedWidget。
void MainWindow::startGame()
{
    if (!m_gameWidget) {
        m_gameWidget = new GameWidget(this);
        m_stack->addWidget(m_gameWidget);   // 索引 2，游戏界面
    }
    m_stack->setCurrentWidget(m_gameWidget);
    m_gameWidget->setFocus();
}

 //切换到帮助界面
void MainWindow::showHelp()
{
    m_stack->setCurrentWidget(m_helpScreen);
    m_helpScreen->setFocus();
}


//切换回开始菜单
void MainWindow::showStartScreen()
{
    m_stack->setCurrentWidget(m_startScreen);
    m_startScreen->setFocus();
}

//键盘事件处理，提供快捷操作
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (m_stack->currentWidget() == m_startScreen && event->key() == Qt::Key_Space) {
        startGame();
        event->accept();
        return;
    }

    if (m_stack->currentWidget() == m_helpScreen && (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Space)) {
        showStartScreen();
        event->accept();
        return;
    }

    QMainWindow::keyPressEvent(event);
}
