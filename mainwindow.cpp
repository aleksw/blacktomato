#include <QSystemTrayIcon>
#include <QTimer>
#include <QCloseEvent>
#include <QDebug>
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QFontDatabase>
#include "mainwindow.h"
#include "ui_mainwindow.h"

static constexpr auto WORK_MINS = 25;
static constexpr auto REST_MINS = 5;
static constexpr auto LONG_REST_MINS = 30;

class Mode
{
public:
    int timeoutSeconds{0};
    QString title;
    QString message;
    QString iconResource;
    int elapsedSeconds{0};

    Mode(QString title, QString message, int timeoutMins, QString iconResource)
        :
          timeoutSeconds(timeoutMins * 60),
          title(title),
          message(message),
          iconResource(iconResource)
    {
    }

    static Mode& work()
    {
        static Mode w("Work", "Time to resume working", WORK_MINS, ":/tomato-red");
        return w;
    }

    static Mode& rest()
    {
        static Mode r("Rest", "Please take a short break", REST_MINS, ":/tomato-green");
        return r;
    }

    static Mode& longRest()
    {
        static Mode lr("Long Rest", "Please enjoy a longer break", LONG_REST_MINS, ":/tomato-blue");
        return lr;
    }

    QTime remainTime()
    {
        unsigned secondsLeft = timeoutSeconds - elapsedSeconds;
        unsigned minRemain = secondsLeft / 60;
        unsigned secRemain = (secondsLeft % 60);
        return QTime(0,minRemain,secRemain);
    }

    void reset()
    {
        elapsedSeconds = 0;
    }

};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_trayIcon(new QSystemTrayIcon(this)),
    m_timer(new QTimer(this)),
    m_mode(&Mode::work())
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/tomato-grey"));
    setWindowTitle("Black Tomato");

    m_timer->setTimerType(Qt::TimerType::CoarseTimer);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(onTimerTimeout()));

    m_trayIcon->setContextMenu(ui->menu);

    update();

    m_trayIcon->show();

    m_cycle << &Mode::work() << &Mode::rest()
            << &Mode::work() << &Mode::rest()
            << &Mode::work() << &Mode::rest()
            << &Mode::work() << &Mode::longRest();
    m_cycleIndex = 0;

    int id = QFontDatabase::addApplicationFont(":/dejavu-font");
    m_trayFontFamily = QFontDatabase::applicationFontFamilies(id).at(0);
}

MainWindow::~MainWindow()
{
    delete ui; ui = nullptr;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    hide();
    event->ignore();
}

void MainWindow::updateTrayIcon()
{
    static QString lastResource;
    static int lastMin;

    if(m_timer->isActive() > 0)
    {
        int min = m_mode->remainTime().minute();
        if(min == 0)
        {
            min = 1;
            min = m_mode->remainTime().second();
        }
        int offset = (min <= 9) ? 11 : 0;

        if(min == lastMin && m_mode->iconResource == lastResource) { return; }

        lastResource = m_mode->iconResource;
        lastMin = min;

        QPixmap pm = QPixmap::fromImage(QImage(m_mode->iconResource));
        QPainter painter(&pm);

        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(38);
        font.setFixedPitch(true);
        font.setFamily(m_trayFontFamily);

        QPen pen(QColor(70,70,70));
        pen.setWidth(6);
        painter.setFont(font);
        painter.setPen(pen);
        QPainterPath pp;

        QString minString = QString("%1").arg(min);
        pp.addText(10 + offset,45,font,minString);
        painter.drawPath(pp);
        pen.setColor(QColor(255,255,255));
        painter.setPen(pen);
        painter.drawText(10 + offset,45,minString);

        m_trayIcon->setIcon(QIcon(pm));
    }
    else
    {
        m_trayIcon->setIcon(QIcon(":/tomato-grey"));
    }
}

void MainWindow::quit()
{
    QApplication::quit();
}

void MainWindow::update()
{
    updateTrayIcon();
    updateContextMenu();
}

void MainWindow::updateContextMenu()
{
    if(m_timer->isActive())
    {
        ui->menuStart->menuAction()->setVisible(false);
        ui->actionStop->setVisible(true);
        ui->actionTime->setVisible(true);
        ui->actionTime->setEnabled(false);

        QString timeLabel = QString("%1 (%2:%03)").arg(m_mode->title).arg(m_mode->remainTime().minute(),2,10,QChar('0'))\
                                             .arg(m_mode->remainTime().second(),2,10,QChar('0'));
        ui->actionTime->setText(timeLabel);
    }
    else
    {
        ui->menuStart->menuAction()->setVisible(true);
        ui->actionStop->setVisible(false);
        ui->actionTime->setVisible(false);
    }
}

void MainWindow::onTimerTimeout()
{
    m_mode->elapsedSeconds++;
    if(m_mode->elapsedSeconds >= m_mode->timeoutSeconds)
    {
        if(m_cycleIndex != -1){
            if(++m_cycleIndex >= m_cycle.count()) {
                m_cycleIndex = 0;
            }

            m_mode = m_cycle[m_cycleIndex];
            m_mode->reset();
            m_trayIcon->showMessage("Black Tomato", m_mode->message, QSystemTrayIcon::Information, 2000);
        }
        else
        {
            stop();
        }
    }

    update();

}

void MainWindow::startWork()
{
    m_cycleIndex = 0;
    m_mode = m_cycle.at(m_cycleIndex);
    start();
}

void MainWindow::startRest()
{
    m_cycleIndex = 1;
    m_mode = m_cycle.at(m_cycleIndex);
    start();
}

void MainWindow::startLongRest()
{
    m_cycleIndex = m_cycle.count()-1;
    m_mode = m_cycle.at(m_cycleIndex);
    start();
}

void MainWindow::start()
{
    m_mode->reset();
    m_timer->stop();
    m_timer->start(1000);
    update();
}

void MainWindow::stop()
{
    m_timer->stop();
    update();
}
