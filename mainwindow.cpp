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
static constexpr auto MESSAGE_TIMEOUT_HINT = 5000;

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
    m_mode(&Mode::work()),
    m_paused(false)
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

void MainWindow::drawIcon(const QString &resource, const QString &text)
{
    static QString lastResource;
    static QString lastText;

    if(text == lastText && resource == lastResource) { return; }
    lastText = text;
    lastResource = resource;

    if(text.length() > 2)
    {
        qDebug() << "icon text too long or empty";
        return;
    }

    int offset = text.length() == 1 ? 11 : 0;

    if(text.isEmpty())
    {
        m_trayIcon->setIcon(QIcon(resource));
    }
    else
    {
        QPixmap pm = QPixmap::fromImage(QImage(resource));
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

        pp.addText(10 + offset,45,font,text);
        painter.drawPath(pp);
        pen.setColor(QColor(255,255,255));
        painter.setPen(pen);
        painter.drawText(10 + offset,45,text);

        m_trayIcon->setIcon(QIcon(pm));
    }
}

void MainWindow::updateTrayIcon()
{
    QString resource;
    QString text;

    if(m_timer->isActive())
    {

        int remTime = m_mode->remainTime().minute();
        if(remTime == 0)
        {
            remTime = m_mode->remainTime().second();
        }
        resource = m_mode->iconResource;
        text = QString("%1").arg(remTime);
    }
    else
    {
        resource = ":/tomato-grey";
        text = m_paused ? "P": "";
    }

    drawIcon(resource, text);
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
        ui->actionPause->setVisible(true);
        ui->actionResume->setVisible(false);

        QString timeLabel = QString("%1 (%2:%03)").arg(m_mode->title).arg(m_mode->remainTime().minute(),2,10,QChar('0'))\
                                             .arg(m_mode->remainTime().second(),2,10,QChar('0'));
        ui->actionTime->setText(timeLabel);
    }
    else
    {
        if(m_paused)
        {
            QString timeLabel = QString("%1 (paused)").arg(m_mode->title);
            ui->actionTime->setText(timeLabel);
        }

        ui->menuStart->menuAction()->setVisible(!m_paused);
        ui->actionStop->setVisible(m_paused);
        ui->actionTime->setVisible(m_paused);
        ui->actionPause->setVisible(false);
        ui->actionResume->setVisible(m_paused);
    }
}

void MainWindow::onTimerTimeout()
{
    if(m_mode->elapsedSeconds++ >= m_mode->timeoutSeconds)
    {
        if(m_cycleIndex != -1){
            if(++m_cycleIndex >= m_cycle.count()) {
                m_cycleIndex = 0;
            }

            m_mode = m_cycle[m_cycleIndex];
            m_mode->reset();
            m_trayIcon->showMessage("Black Tomato",
                                    m_mode->message,
                                    QSystemTrayIcon::Information,
                                    MESSAGE_TIMEOUT_HINT);
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

void MainWindow::start(bool resume)
{
    if(!resume)
    {
        m_mode->reset();
    }
    m_timer->stop();
    m_timer->start(1000);
    m_paused = !resume;
    update();
}

void MainWindow::stop(bool pause)
{
    m_timer->stop();
    m_paused = pause;
    update();
}

void MainWindow::pause()
{
    stop(true);
}

void MainWindow::resume()
{
    start(true);
}
