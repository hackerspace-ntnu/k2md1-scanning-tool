#include "scanthething.h"
#include "ui_scanthething.h"

#include <QTime>
#include <QTimer>

ScanTheThing::ScanTheThing(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ScanTheThing)
{
    ui->setupUi(this);
    m_testFire = new QTimer();

    m_testFire->setInterval(500);

    connect(m_testFire,&QTimer::timeout, this, &ScanTheThing::timerFire);

    m_testFire->start();
}

ScanTheThing::~ScanTheThing()
{
    delete ui;
}

void ScanTheThing::timerFire()
{
    QTime derp = QTime::currentTime();

    qDebug("Hello!\n");

    ui->progressBar->setValue(derp.second()*4 % 101);
}

void ScanTheThing::on_progressBar_valueChanged(int value)
{
    if(value == 100)
        ui->progressBar->setHidden(true);
    else
        ui->progressBar->setHidden(false);
}
