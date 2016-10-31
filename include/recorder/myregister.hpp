#include "wrapper.hpp"
#include <libfreenect2/registration.h>
#include <cmath>

using namespace std;

float&mymin(float&a, float&b) {
  return a<b?a:b;
}
float&mymax(float&a, float&b) {
  return a>b?a:b;
}


struct MyRegistration {
  Registration*reg;
  float*lastx, *lasty, *thisx, *thisy;
  int sw, sh;
  MyRegistration(MyKinect&kinect, int w, int h) {
    sw = w, sh = h;
    reg = new Registration(kinect.dev->getIrCameraParams(), kinect.dev->getColorCameraParams());
    lastx = new float[512];
    lasty = new float[512];
    thisx = new float[512];
    thisy = new float[512];
  }
  ~MyRegistration() {
    delete reg;
    delete lastx;
    delete lasty;
    delete thisx;
    delete thisy;
  }
  void projectDepth(float*zbuf, float*depth) {
    for (int i = 0; i < sw*sh; i++) zbuf[i] = 1e9;
    for (int j = 0; j < depth_height; j++) {
      for (int i = 0; i < depth_width; i++) {
	float&z = depth[i+j*512];
	reg->apply(i, j, z, thisx[i], thisy[i]);
	thisx[i] *= sw/1920.f;
	thisy[i] *= sh/1080.f;
	if (thisx[i] < 0 or thisx[i] >= sw or thisy[i] < 0 or thisy[i] >= sh) z = 0;
	if (!i or !j) continue;
	float&z1 = depth[i-1+j*512-512];
	float&z2 = depth[i+j*512-512];
	float&z3 = depth[i-1+j*512];
	float&z4 = depth[i+j*512];
	if (!z1 or !z2 or !z3 or !z4) continue;
	float mi = mymin(mymin(z1, z2), mymin(z3, z4));
	float ma = mymax(mymax(z1, z2), mymax(z3, z4));
	if (ma-mi > 100) continue;
	renderTri(zbuf, 
		  lastx[i-1], lasty[i-1], z1, 
		  thisx[i], thisy[i], z4, 
		  thisx[i-1], thisy[i-1], z3);
	renderTri(zbuf, 
		  lastx[i-1], lasty[i-1], z1, 
		  lastx[i], lasty[i], z2, 
		  thisx[i], thisy[i], z4);
      }
      float*tmp;
      tmp = lastx;
      lastx = thisx;
      thisx = tmp;

      tmp = lasty;
      lasty = thisy;
      thisy = tmp;
    }
  }
  inline void innerloop(int&j, float&x1, float&f1, float&x2, float&f2, float z0, float&zfx, float*&zbuf) {
    for (int i = x1+j*f1; i < x2+j*f2; i++) {
      float z = z0+i*zfx;
      if (z < zbuf[i+j*sw]) {
	zbuf[i+j*sw] = z;
      }
    }
  }

  void cross(float ax, float ay, float az, float bx, float by, float bz, float&nx, float&ny, float&nz) {
    nx = ay*bz-az*by;
    ny = az*bx-ax*bz;
    nz = ax*by-ay*bx;
  }
  //Assumes
  //ay < by, cy
  //bx < cx
  void renderTri(float*&zbuf, float&ax, float&ay, float&az, float&bx, float&by, float&bz, float&cx, float&cy, float&cz) {
    //if (ay > by or ay > cy or bx < cx) return;
  
    float ab = (bx-ax)/(by-ay), ac = (cx-ax)/(cy-ay), bc = (cx-bx)/(cy-by);
    float x1 = ax-ay*ac+1-1e-6, f1 = ac, x2 = ax-ay*ab-1e-6, f2 = ab;
  
    float nx, ny, nz;
    cross(ax-bx, ay-by, az-bz, ax-cx, ay-cy, az-cz, nx, ny, nz);
    float na = nx*ax+ny*by+nz*bz, inz = 1./nz;
    nx *= -inz;
    ny *= -inz;
    nz = na*inz;

    //if (nx*nx > 100 or ny*ny > 100) return;

    float ly = mymin(by, cy), ly2 = mymax(by, cy);
    for (int j = ceil(ay); j < ly; j++) 
      innerloop(j, x1, f1, x2, f2, nz+j*ny, nx, zbuf);
    if (by < cy) {
      x2 = bx-by*bc;
      f2 = bc;
    } else {
      x1 = bx-by*bc;
      f1 = bc;
    }
    for (int j = ceil(ly); j < ly2; j++) 
      innerloop(j, x1, f1, x2, f2, nz+j*ny, nx, zbuf);
  }
};
