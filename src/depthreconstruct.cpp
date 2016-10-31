#include "depthreconstruct.h"

#include <QProcess>

#include "meshing.h"
#include "tracker.h"

DepthReconstruct::DepthReconstruct(QObject *parent) : QObject(parent)
{
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

        normals_main(2, app_args);
        finishedNormals();
    }
    {
        QProcessEnvironment curr_env;
        QString appdir = curr_env.value("APPDIR");
        QProcess ssd_recon;
        ssd_recon.setProgram(appdir + "/bin/ssd_recon");
        ssd_recon.setArguments({"-octreeLevels", "9",
                                "-weights","1","1","5",
                                "-samplesPerNode","10",
                                OUTPUT_PLY_FILE,
                                finalPlyFile()
                               });
        ssd_recon.start();
        finishedReconstruction();
    }
}
