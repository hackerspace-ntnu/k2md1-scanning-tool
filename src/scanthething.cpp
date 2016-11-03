#include "scanthething.h"
#include "ui_scanthething.h"

#include <QMessageBox>
#include <QDir>
#include <QWindow>

#include "depthscanner.h"
#include "depthorient.h"
#include "depthreconstruct.h"

#include <unistd.h>

ScanTheThing::ScanTheThing(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ScanTheThing)
{
    ui->setupUi(this);

    resetEverything();
}

ScanTheThing::~ScanTheThing()
{
    delete ui;
}

void ScanTheThing::on_progressBar_valueChanged(int value)
{
//    if(value == std::max(ui->progressBar->maximum(), 1))
//        ui->progressBar->setHidden(true);
//    else
//        ui->progressBar->setVisible(false);
}

void ScanTheThing::displayErrorMessage(const QString &error)
{
    if(!m_errorBox)
        m_errorBox = new QMessageBox(this);

    m_errorBox->setText(error);
    m_errorBox->show();
}

void ScanTheThing::activateScanning()
{
    setProgress(false, 100, 100, "Finished calibration");
    ui->scanButton->setEnabled(true);
}

void ScanTheThing::activateReconstruction()
{
    setProgress(false, 100, 100, "Finished scanning");
    ui->scanButton->setEnabled(false);
    ui->reconButton->setEnabled(true);
}

void ScanTheThing::setProgress(bool continuous, int progress, int progressMax, const QString &text)
{
    ui->progressBar->setVisible(false);
    if(continuous)
    {
        ui->progressBar->setMaximum(0);
        ui->progressBar->setValue(0);
    }else{
        ui->progressBar->setMaximum(progressMax);
        ui->progressBar->setValue(progress);
    }
    ui->progressBar->setFormat(text);
    ui->progressBar->setVisible(true);
}

void ScanTheThing::setProgress(int value, int max)
{
    setProgress(false, value, max, QString("Processed %v/%m"));
}

void ScanTheThing::updateImageCount(int imgs)
{
    ui->scanButton->setText("Stop scan");
    setProgress(true, 0, 0, "");
    ui->progressBar->setFormat(QString("Have %1 images").arg(imgs));
}

void ScanTheThing::insertXWindow(QWindow *)
{
//    window->setFlags(Qt::FramelessWindowHint);
//    ui->splitter->addWidget(m_currentWindowContainer = QWidget::createWindowContainer(window, this));
    //    m_currentWindow = window;
}

void ScanTheThing::finishedProcess()
{

}

void ScanTheThing::resetEverything()
{
    setProgress(0, 100);
    ui->progressBar->setVisible(false);

    ui->calibrateButton->setText("Calibrate");
    ui->scanButton->setText("Scan");

    ui->calibrateButton->setEnabled(true);
    ui->scanButton->setEnabled(false);
    ui->reconButton->setEnabled(false);

    if(m_scanner)
    {
        m_threadPool.cancel(m_scanner);
        m_scanner = nullptr;
    }
    if(m_reconstruct)
    {
        m_threadPool.cancel(m_reconstruct);
        m_reconstruct = nullptr;
    }
}

void ScanTheThing::on_calibrateButton_clicked()
{
    if(!m_scanner)
    {
        resetEverything();

        m_scanner = new DepthScanner(this);

        setProgress(true, 0, 0, "Calibrating");

        m_threadPool.start(m_scanner);

        ui->calibrateButton->setText("Stop calibration");
        m_scanner->setState(DepthScanner::Calibrating);
    } else if(m_scanner->state() == DepthScanner::Calibrating){
        ui->calibrateButton->setEnabled(false);
        m_scanner->setState(DepthScanner::Calibrated);
        /* The above state change is propagated through m_scanner to acitvateScanning() */
    }
}

void ScanTheThing::on_scanButton_clicked()
{
    if(m_scanner->state() == DepthScanner::Calibrated)
    {
        setProgress(true, 0, 0, "Scanning");
        m_scanner->setState(DepthScanner::Scanning);
    } else if(m_scanner->state() == DepthScanner::Scanning)
    {
        ui->scanButton->setEnabled(false);
        m_scanner->setState(DepthScanner::Scanned);
        /* The above state change is propagated through m_scanner to activateReconstruction() */
    }
}

void ScanTheThing::on_reconButton_clicked()
{
    if(numImages() == 75)
    {
        displayErrorMessage("Too few images!");
        return;
    }
    if(!m_reconstruct)
    {
        m_reconstruct = new DepthReconstruct(this);

        m_reconstruct->setNumImages(numImages());
        m_reconstruct->setFinalPlyFile("finally.ply");

        m_threadPool.start(m_reconstruct);

        ui->reconButton->setEnabled(false);
    }
}

void ScanTheThing::on_actionQuit_triggered()
{
    this->close();
}

void ScanTheThing::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this);
}

void ScanTheThing::on_actionAbout_triggered()
{
    QMessageBox::about(this, "Scan The Thing", "You're on your own :)");
}

void ScanTheThing::on_actionReset_triggered()
{
    QString p = QApplication::applicationFilePath();
    std::string p_std = p.toStdString();
    /*
     * This is terrible. We relaunch the process and clear all data.
     * It's the only way, I promise.
     */
    execve(p_std.c_str(), nullptr, environ);
}
