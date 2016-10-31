#include <iostream>

#include "bat/bat.hpp"
#include "pointcloud/trackball.hpp"

using namespace std;

#include "recorder/dataio.hpp"

static int N_FRAMES = 50;
#define USE_COLOR

extern const char* POSE_FILE;

struct Point {
    Color col;
    float x, y, z;
    Point() {}
    Point(float x_, float y_, float z_, Color&col) : x(x_),y(y_),z(z_),col(col) {}
};

int showall = 0, showframe = 0, frame = 0;

static void renderPoints(Surface&sf, float*zbuf, Point*point, int points, vec3 pos,
                         vec3 side, vec3 up, vec3 viewdir, float&pers) {
    for (int i = 0; i < sf.w*sf.h; i++) {
        sf.pixels[i] = 0x503080;
        zbuf[i] = 1e9;
    }

    float ccx = sf.w*.5-.5, ccy = sf.h*.5-.5;
    int inc = !showall*19+1;
    for (int i = 0; i < points; i += inc) {
        float px = point[i].x, py = point[i].y, pz = point[i].z;

        px -= pos.x;
        py -= pos.y;
        pz -= pos.z;
        float x = side.x*px+side.y*py+side.z*pz;
        float y = up.x*px+up.y*py+up.z*pz;
        float z = viewdir.x*px+viewdir.y*py+viewdir.z*pz, iz = 1.f/z;
        int ix = x*pers*iz+ccx;
        int iy = y*pers*iz+ccy;
        if (z > 0 and ix >= 0 and iy >= 0 and ix < sf.w and iy < sf.h and zbuf[ix+iy*sf.w] > z) {
            zbuf[ix+iy*sf.w] = z;
#if defined USE_COLOR
            sf.pixels[ix+iy*sf.w] = point[i].col;
#else
            sf.pixels[ix+iy*sf.w] = (uint)(sin(z/5)*127+127)*0x10101;
#endif
        }
    }
}

int points_main(int argc, char**argv) {
    if (argc > 1) {
        sscanf(argv[1], "%d", &N_FRAMES);
    } else {
        cout << "Using " << N_FRAMES << " frames by default, change by running " << argv[0]
             << " <N frames>" << endl;
    }


    int sw = 1920/2, sh = 1080/2;
    int Iw = 1920/2, Ih = 1080/2;
    int Dw = 512, Dh = 424;
    MyScreen screen(sw, sh);
    Surface sf(sw, sh);
    Clock clock;

    float*zbuf = new float[sw*sh];
    Surface I(Dw, Dh);

    float ccx = Iw*.5-.5, ccy = Ih*.5-.5, pers = 1081.37f/2;
    float Dcx = 255.559, Dcy = 207.671, Dpers = 365.463;

    float rotmat[9];
    float px, py, pz;

    FILE*fp = fopen(POSE_FILE, "r");

    float ifx = 1.f/Dpers;
    int points = 0;
    Point*point = new Point[300000000];
    int*pstart = new int[N_FRAMES+1];
    for (int i = 0; i < N_FRAMES; i++) {
        float dx, dy, dz, dxyz[3];

        for (int k = 0; k < 3; k++) {
            for (int j = 0; j < 3; j++)
                fscanf(fp, "%f", &rotmat[k*3+j]);
            fscanf(fp, "%f", &dxyz[k]);
        }
        for (int k = 0; k < 4; k++)
            fscanf(fp, "%f", &dx);
        dx = dxyz[0];
        dy = dxyz[1];
        dz = dxyz[2];
        //if (i != 0 and i != 70) continue;

        //cout << dx << ' ' << dy << ' ' << dz << endl;
        //for (int k = 0; k < 9; k++) cout << rotmat[k] << endl;

        loadZB(zbuf, Dw, Dh, i);
#if defined USE_COLOR
        loadSurface(I, Dw, Dh, i);
#endif

        pstart[i] = points;
        for (int l = 0; l < Dh; l++) {
            for (int k = 0; k < Dw; k++) {
                float tz = zbuf[k+l*Dw];
                //if (tz < 500 && tz > 10) cout << tz << endl;
                if (tz > 1e8 || tz == 0) continue;
                float tx = (k-Dcx)*tz*ifx, ty = (l-Dcy)*tz*ifx;
                float x = tx*rotmat[0]+ty*rotmat[1]+tz*rotmat[2]+dx*1000.f;
                float y = tx*rotmat[3]+ty*rotmat[4]+tz*rotmat[5]+dy*1000.f;
                float z = tx*rotmat[6]+ty*rotmat[7]+tz*rotmat[8]+dz*1000.f;
#if defined USE_COLOR
                Color col = I.pixels[k+l*Dw];
                if (col == 0) continue;
#else
                Color col = 0xffffff;
#endif	//if (rand()%10 == 0) 
                point[points++] = Point(x, y, z, col);
            }
        }
    }
    pstart[N_FRAMES] = points;
    //cout << points << endl;
    fclose(fp);

    MyTrackball track(screen);
    track.speed = 30;

    while (1) {
        while (screen.gotEvent()) {
            Event e = screen.getEvent();
            if (e.type == KeyPress and e.key == K_ESCAPE) return 0;
            else if (e.type == KeyPress) {
                if (e.key == K_SPACE) showall ^= 1;
                else if (e.key == K_TAB) showframe ^= 1;
                else if (e.key == K_LEFT) frame -= 1;
                else if (e.key == K_RIGHT) frame += 1;
            }
            track.handleEvent(e);
        }
        track.update();
        vec3 side = up^viewdir, pos = track.pos;

        if (showframe) {
            frame = (frame%N_FRAMES+N_FRAMES)%N_FRAMES;
            renderPoints(sf, zbuf, point+pstart[frame], pstart[frame+1]-pstart[frame], pos, side, up, viewdir, pers);
        } else
            renderPoints(sf, zbuf, point, points, pos, side, up, viewdir, pers);
        //for (int i = 0; i < sw*sh; i += 2)
        //	sf.pixels[i] = I[i]*0x10101;
        screen.putSurface(sf);
        clock.tick(30);
    }
}
