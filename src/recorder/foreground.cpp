#include "foreground_info.h"

XWindow m_window;

#include "recorder/myregister.hpp"
#include "bat/bat.hpp"
#include "recorder/dataio.hpp"
#include <cmath>

#define REGISTER

static int diff(Color a, Color b) {
    int dr = int(a.r)-int(b.r);
    int dg = int(a.g)-int(b.g);
    int db = int(a.b)-int(b.b);
    //cout << dr*dr+dg*dg+db*db << endl;
    return dr*dr+dg*dg+db*db;// > 10*10;
}

int foreground_main() {
    int sw = 512, sh = 424;
    int Iw = 1920, Ih = 1080;

    MyScreen screen(sw, sh);

    /* Fetch some window pointers, allowing us to embed the window in Qt */
    m_window.display = screen.display;
    m_window.window = screen.win;

    f_info.setWindowPtr(&m_window);

    Surface sf(sw, sh);
    Clock clock;

    MyKinect kinect;

    /* Give signal back if error occurs */
    if(kinect.numDevices <= 0)
    {
        f_info.kinectNotFound();
        return 1;
    }

    float*depth;
    Color*col;


#if defined REGISTER
    Registration reg(kinect.dev->getIrCameraParams(),
                     kinect.dev->getColorCameraParams());
    Frame undistorted(depth_width, depth_height, 4),
            registered(depth_width, depth_height, 4),
            registered2(depth_width, depth_height, 4);
#endif

    Surface shrink(sw, sh);
    Surface shrinktmp(sw, sh);
    Surface bg(sw, sh);
    Surface avg(Iw, Ih);

    std::vector<float> mindepth;
    mindepth.resize(sw*sh);
    for (int i = 0; i < sw*sh; i++) mindepth[i] = 1e9, avg.pixels[i].a = 0;
    int saved = 0;
    int mode = 1, save = 0, disp = 0;
    while (1) {
        while (screen.gotEvent()) {
            Event e = screen.getEvent();
            int key = e.key, type = e.type;
//            if (e.type == KeyPress and e.key == K_ESCAPE) {
//                cout << "Recorded " << saved
//                     << " images (~"
//                     << saved*((depth_width*depth_height*2+sw*sh*4)>>20)
//                     << " MB)" << endl;
//                return save ? 0 : 2;
//            } else
//                if (e.type == KeyPress and e.key == K_SPACE)
//                mode = 2;
//            else if (e.type == KeyPress and e.key == K_s)
//                save = 1;
//            else if (e.type == KeyPress and e.key == K_TAB) disp ^= 1;
        }

        /* Use QObject as trigger for events, not keys */
        if(f_info.calibrated() && mode != 2)
        {
            f_info.finishedCalibration();
            mode = 2;
        }
        if(f_info.scanningTime())
            save = 1;
        /* Tell host if anything was scanned */
        if(f_info.endScan())
        {
            f_info.finishedScanning();
            return save ? 0 : 2;
        }

        kinect.waitForFrame();
        kinect.getColorAndDepth((uint**)&col, &depth);

#ifdef REGISTER
        reg.apply(kinect.frames[Frame::Color], kinect.frames[Frame::Depth], &undistorted, &registered);
        shrink.pixels = (Color*)registered.data;

        kinect.frames[Frame::Color]->data = (unsigned char*)avg.pixels;
        reg.apply(kinect.frames[Frame::Color], kinect.frames[Frame::Depth], &undistorted, &registered2);
        bg.pixels = (Color*)registered2.data;
#endif

        if (mode == 2) {
            for (int j = 0; j < sh; j++) {
                for (int i = 0; i < sw; i++) {
                    if (mindepth[i+j*sw] > depth[i+j*sw]+10
                            && depth[i+j*sw]
                            && depth[i+j*sw] < 1500
                            && diff(shrink.pixels[i+j*sw], bg.pixels[i+j*sw]) > 30*30) {
                        //shrink.pixels[i+j*sw] = avg.pixels[i+j*sw];//int(sin(diff(shrink.pixels[i+j*sw], avg.pixels[i+j*sw])*.1)*127+127)*0x10101;;
#ifndef REGISTER
                        shrink.pixels[i+j*sw] = int(sin(depth[i+j*sw]*.1)*127+127)*0x10101;
#endif
                    }
                    else {
                        shrink.pixels[i+j*sw] = 0;
                        depth[i+j*sw] = 0;
                    }
                }
            }
            for (int j = 2; j < sh-2; j++) {
                for (int i = 2; i < sw-2; i++) {
                    int c = 0;
                    for (int k = -2; k < 3; k++)
                        for (int l = -2; l < 3; l++)
                            c += shrink.pixels[i+l+(j+k)*sw] != 0;
                    shrinktmp.pixels[i+j*sw] = (c>=20)*shrink.pixels[i+j*sw];
                }
            }
            for (int j = 2; j < sh-2; j++) {
                for (int i = 2; i < sw-2; i++) {
                    shrink.pixels[i+j*sw] = shrinktmp.pixels[i+j*sw];
                }
            }

        } else {
            for (int j = 0; j < sh; j++) {
                for (int i = 0; i < sw; i++) {
                    float&d = depth[i+j*sw];
                    if (d)
                        mindepth[i+j*sw] = std::min(mindepth[i+j*sw], d);
                    shrink.pixels[i+j*sw] = int(sin(mindepth[i+j*sw]*.1)*127+127)*0x10101;
                }
            }
#if defined REGISTER
            for (int j = 0; j < Ih; j++) {
                for (int i = 0; i < Iw; i++) {

                    if (col[i+j*Iw]) {
                        int a = avg.pixels[i+j*Iw].a;
                        float ia = 1./(avg.pixels[i+j*Iw].a+1);
                        avg.pixels[i+j*Iw].r = (avg.pixels[i+j*Iw].r*a+col[i+j*Iw].r)*ia;
                        avg.pixels[i+j*Iw].g = (avg.pixels[i+j*Iw].g*a+col[i+j*Iw].g)*ia;
                        avg.pixels[i+j*Iw].b = (avg.pixels[i+j*Iw].b*a+col[i+j*Iw].b)*ia;
                        avg.pixels[i+j*Iw].a++;
                    }
#endif
                }
            }
        }
        screen.putSurface(disp?bg:shrink);

        if (save) {
#if defined REGISTER
            saveIMG(shrink, sw, sh, saved);
#endif
            saveZB(depth, 512, 424, saved);
            saved++;

            f_info.setImages(saved);
        }

        clock.tick(30);
        clock.print();
    }
}
