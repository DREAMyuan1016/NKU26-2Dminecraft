#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QKeyEvent;
class QStackedWidget;
class QWidget;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class GameWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    void startGame();
    void showHelp();
    void showStartScreen();

    GameWidget *m_gameWidget;
    QStackedWidget *m_stack;
    QWidget *m_startScreen;
    QWidget *m_helpScreen;
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
