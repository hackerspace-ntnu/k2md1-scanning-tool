#include "orientation_info.h"
#include "depthreconstruct.h"

#include <QProcess>

#include "scanthething.h"
#include "meshing.h"
#include "pointcloud.h"
#include "tracker.h"

DepthReconstruct::DepthReconstruct(ScanTheThing* interface, QObject *parent) : QObject(parent)
{
    m_userInterface = interface;

    connect(&o_info, SIGNAL(putLoadingInfo(int,int)),
            interface, SLOT(setProgress(int,int)));

    connect(&o_info, &Orientation_Info::dislikePoints,
            [&]()
    {
        m_doFinishReconstruct = false;
    });

    connect(this, &DepthReconstruct::finishedNormals,
            [&]()
    {
        m_userInterface->setProgress(true, 0,0, "Running SSD reconstruction");
    });

    connect(this, &DepthReconstruct::finishedReconstruction,
            [&]()
    {
        m_userInterface->displayErrorMessage("Finished reconstruction!");
    });
}

void DepthReconstruct::run()
{
    QString image_count = QString("%1").arg(numImages());
    std::string image_storage = image_count.toStdString();
    std::string appname = "INTERNAL";

    char* app_args[] = {
        &appname[0], &image_storage[0], nullptr
    };

    VIEW = ".views.txt";
    OUTPUT_PLY_FILE = ".intermediate.ply";

    {
        dualdvo_main(2, app_args);
        finishedDvoSomething();
    }
    {
        points_main(2, app_args);
        finishedPoints();
    }

    if(!m_doFinishReconstruct)
        return;

    {
        normals_main(2, app_args);
        finishedNormals();
    }
    {
        m_userInterface->setProgress(true, 0,0, "Running SSD reconstruction");
        QProcessEnvironment curr_env = QProcessEnvironment::systemEnvironment();
        QString appdir = curr_env.value("APPDIR");
        qDebug("----------------------");
        QProcess::execute(appdir + "/bin/ssd_recon", {"-octreeLevels", "9",
                                                      "-weights","1","1","5",
                                                      "-samplesPerNode","10",
                                                      OUTPUT_PLY_FILE,
                                                      finalPlyFile()
                                                     });
        qDebug("----------------------");
        finishedReconstruction();
    }
}
