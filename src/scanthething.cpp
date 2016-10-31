#include "scanthething.h"
#include "ui_scanthething.h"

#include <QMessageBox>

#include "depthscanner.h"
#include "depthorient.h"
#include "depthreconstruct.h"

ScanTheThing::ScanTheThing(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ScanTheThing)
{
    ui->setupUi(this);

    resetView();
}

ScanTheThing::~ScanTheThing()
{
    delete ui;
}

void ScanTheThing::on_progressBar_valueChanged(int value)
{
    if(value == 100)
        ui->progressBar->setHidden(true);
    else
        ui->progressBar->setHidden(false);
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
    ui->reconButton->setEnabled(true);
}

void ScanTheThing::setProgress(bool continuous, int progress, int progressMax, const QString &text)
{
    if(continuous)
    {
        ui->progressBar->setMaximum(0);
        ui->progressBar->setValue(0);
    }else{
        ui->progressBar->setMaximum(progressMax);
        ui->progressBar->setValue(progress);
    }
    ui->progressBar->setFormat(text);
}

void ScanTheThing::updateImageCount(int imgs)
{
    ui->scanButton->setText(QString("Scanned images: %1").arg(imgs));
}

void ScanTheThing::resetView()
{
    ui->progressBar->setValue(0);
    ui->progressBar->setMaximum(100);
    ui->progressBar->setHidden(true);

    ui->calibrateButton->setText("Calibrate");
    ui->scanButton->setText("Scan");

    ui->calibrateButton->setEnabled(true);
    ui->scanButton->setEnabled(false);
    ui->reconButton->setEnabled(false);
}

void ScanTheThing::on_calibrateButton_clicked()
{
    if(!m_scanner)
    {
        resetView();

        m_scanner = new DepthScanner(this);

        m_threadPool.start(m_scanner);

        ui->calibrateButton->setText("Stop calibration");
    } else{
        ui->calibrateButton->setEnabled(false);
        m_scanner->setState(DepthScanner::Calibrated);
    }
}

void ScanTheThing::on_scanButton_clicked()
{
    if(m_scanner->state() == DepthScanner::Calibrated)
    {
        m_scanner->setState(DepthScanner::Scanning);
    } else if(m_scanner->state() == DepthScanner::Scanning)
    {
        ui->scanButton->setEnabled(false);
        m_scanner->setState(DepthScanner::Scanned);
    }
}

void ScanTheThing::on_reconButton_clicked()
{
    m_orient = new DepthOrient();
    m_reconstruct = new DepthReconstruct();

    m_threadPool.start(m_orient);
    m_threadPool.start(m_reconstruct);

    ui->calibrateButton->setEnabled(false);
    ui->scanButton->setEnabled(false);
}
