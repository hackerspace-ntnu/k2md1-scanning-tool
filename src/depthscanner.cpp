#include "recorder.h"
#include "depthscanner.h"
#include <QFile>
#include <QDir>

#include "scanthething.h"

#include "foreground_info.h"

DepthScanner::DepthScanner(ScanTheThing *interface, QObject *parent) : QObject(parent)
{
    m_userInterface = interface;
    connect(&f_info, SIGNAL(kinectNotFound()),
            this, SLOT(pushErrorMessage()));
    connect(&f_info, SIGNAL(finishedCalibration()),
            interface, SLOT(activateScanning()));
    connect(&f_info, SIGNAL(finishedScanning()),
            interface, SLOT(activateReconstruction()));
    connect(&f_info, SIGNAL(imagesChanged(int)),
            interface, SLOT(updateImageCount(int)));
}

void DepthScanner::run()
{
    m_userInterface->setProgress(true, 0, 0, "Calibrating");

    int ret = foreground_main();

    if(ret == 1)
        m_userInterface->setProgress(false, 100, 100, "Failed to calibrate");
    else if(ret == 2)
        m_userInterface->setProgress(false, 100, 100, "Nothing scanned");
}

void DepthScanner::pushErrorMessage()
{
    m_userInterface->displayErrorMessage("Failed to detect any Kinect 2 devices");
}

void DepthScanner::setState(DepthScanner::ScannerState state)
{
    if (m_state == state)
        return;

    m_state = state;

    switch(state)
    {
    case Calibrated:
        f_info.setCalibrated(true);
        f_info.finishedCalibration();
        break;
    case Scanning:
        f_info.setScanningTime(true);
        break;
    case Scanned:
        f_info.setEndScan(true);
        f_info.finishedScanning();
        break;
    }

    emit stateChanged(state);
}
