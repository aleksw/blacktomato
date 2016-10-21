#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTime>

namespace Ui {
class MainWindow;
}

class QSystemTrayIcon;
class QTimer;
class Mode;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();


protected:
    void closeEvent(QCloseEvent*);

    void drawIcon(const QString& resource, const QString& text);

private slots:
    void onTimerTimeout();

    void startWork();
    void startRest();
    void startLongRest();

    void start(bool resume = false);
    void stop(bool pause = false);

    void resume();
    void pause();

    void quit();

    void update();
    void updateTrayIcon();
    void updateContextMenu();

private:
    Ui::MainWindow *ui{nullptr};
    QSystemTrayIcon* m_trayIcon{nullptr};
    QMenu* m_trayMenu{nullptr};
    QTimer* m_timer{nullptr};
    Mode* m_mode{nullptr};
    QVector<Mode*> m_cycle;
    int m_cycleIndex{0};
    QString m_trayFontFamily;
    bool m_paused{false};
};

#endif // MAINWINDOW_H
