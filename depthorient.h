#ifndef DEPTHORIENT_H
#define DEPTHORIENT_H

#include <QObject>

class DepthOrient : public QObject
{
    Q_OBJECT

public:
    explicit DepthOrient(QObject* parent = 0);

signals:
    void finishedProcessing(int last);

public slots:
    void processImages(QString directory, int start, int count);
};

#endif // DEPTHORIENT_H
