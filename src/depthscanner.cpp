#include "recorder.h"
#include "depthscanner.h"
#include <QDir>
#include <QWindow>

#include "scanthething.h"

#include "foreground_info.h"

DepthScanner::DepthScanner(ScanTheThing *interface, QObject *parent) : QObject(parent)
{
    m_userInterface = interface;
    connect(&f_info, SIGNAL(kinectNotFound()),
            this, SLOT(pushErrorMessage()), Qt::QueuedConnection);
    connect(&f_info, SIGNAL(finishedCalibration()),
            interface, SLOT(activateScanning()), Qt::QueuedConnection);
    connect(&f_info, SIGNAL(finishedScanning()),
            interface, SLOT(activateReconstruction()), Qt::QueuedConnection);
    connect(&f_info, SIGNAL(imagesChanged(int)),
            interface, SLOT(updateImageCount(int)), Qt::QueuedConnection);

    /* Allow transmission of X11 window handle */
    connect(&f_info, SIGNAL(windowPtrChanged(void*)),
            this, SLOT(receiveXWindow(void*)), Qt::QueuedConnection);
    connect(this, SIGNAL(sendXWindow(QWindow*)),
            interface, SLOT(insertXWindow(QWindow*)), Qt::QueuedConnection);
}

void DepthScanner::run()
{
    m_userInterface->setProgress(true, 0, 0, "Calibrating");

    int ret = 0;

    try{
        ret = foreground_main();
        m_userInterface->setNumImages(f_info.images());
    }
    catch ( std::exception e)
    {
        qDebug("Failed hard");
    }

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
    case Calibrating:
        break;
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

void DepthScanner::receiveXWindow(void *winPtr)
{
    XWindow* hnd = static_cast<XWindow*>(winPtr);
    QWindow* win = QWindow::fromWinId(hnd->window);
    sendXWindow(win);
}
