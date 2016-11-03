#include <QObject>

class Orientation_Info : public QObject
{
    Q_OBJECT

public:
    Orientation_Info(QObject* parent = NULL) : QObject(parent) {}

signals:
    void putLoadingInfo(int value, int max);

    void dislikePoints();
};

extern Orientation_Info o_info;
