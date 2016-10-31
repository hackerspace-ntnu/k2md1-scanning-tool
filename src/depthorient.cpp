#include "depthorient.h"
#include <QThread>

#include "meshing.h"
#include "pointcloud.h"
#include "tracker.h"

DepthOrient::DepthOrient(QObject* parent) : QObject(parent)
{

}

void DepthOrient::run()
{
    qDebug("Hello from thread %p", QThread::currentThread());
}

void DepthOrient::processImages(QString directory, int start, int count)
{

}
