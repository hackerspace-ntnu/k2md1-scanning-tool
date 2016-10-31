#ifndef DEPTHORIENT_H
#define DEPTHORIENT_H

#include <QObject>
#include <QRunnable>

#include "orientation_info.h"

class DepthOrient : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit DepthOrient(QObject* parent = 0);
    void run();

signals:
    void finishedProcessing(int last);

public slots:
    void processImages(QString directory, int start, int count);
};

#endif // DEPTHORIENT_H
