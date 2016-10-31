#pragma once

#include "clinit.hpp"
#include <sys/time.h>
#include <iostream>
using std::cout;
using std::endl;
#include "identifyclerror.hpp"

#include <cmath>
#include "linalg.hpp"
#include <iomanip>
#include <algorithm>

#if defined DEBUG
#include "/home/johan/hackerspace/hackerspace/mapbot/k2-md1/src/cl/visualizer.hpp"
#endif


const int w = 512, h = w;
const int scales = 5;
int plotting = 1;
#if defined DEBUG
Color plotimg[w*h];
void plotGpu(cl::Image2D&img, int w, int h) {
  cl::size_t<3> origo, size;
  origo[0] = origo[1] = origo[2] = 0;
  size[0] = w, size[1] = h, size[2] = 1;
  queue.enqueueReadImage(img, 1, origo, size, 0, 0, plotimg);
  plotColor(plotimg, w, h);
}
#endif

float mycross(float*a, float*b, int i) {
  if (i == 0) 
    return a[4]*b[8]-a[8]*b[4];
  if (i == 1)
    return a[8]*b[0]-a[0]*b[8];
  return a[0]*b[4]-a[4]*b[0];
}

void fix(float*A, float*b, se3 pose) {
  float T[12];
  (pose*-1).exp(T);
  /*for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 4; j++) cout << T[i*4+j] << ' ';
    cout << endl;
    }*/
  float trans[36];
  for (int i = 0; i < 3; i++) {
    trans[i*6+0] = T[i*4+0];
    trans[i*6+1] = T[i*4+1];
    trans[i*6+2] = T[i*4+2];
    trans[i*6+3] = 0;
    trans[i*6+4] = 0;
    trans[i*6+5] = 0;
  }
  for (int i = 0; i < 3; i++) {
    trans[i*6+18+0] = mycross(T+3, T+0, i);
    trans[i*6+18+1] = mycross(T+3, T+1, i);
    trans[i*6+18+2] = mycross(T+3, T+2, i);
    trans[i*6+18+3] = T[i*4+0];
    trans[i*6+18+4] = T[i*4+1];
    trans[i*6+18+5] = T[i*4+2];
  }
  /*for (int i = 0; i < 6; i++) {
    se3 test;
    for (int j = 0; j < 6; j++) test.l[j] = i==j;
    se3 out = pose*test*(pose*-1);
    for (int j = 0; j < 6; j++) 
      cout << trans[i*6+j] << ',' << out.l[j] << "  ";
    cout << endl;
  }
  cout << endl;*/
  float tmp[36], newb[6];
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 6; j++) {
      tmp[i*6+j] = 0;
      for (int k = 0; k < 6; k++) tmp[i*6+j] += trans[i*6+k]*A[k*6+j];
    }
  }
  for (int i = 0; i < 6; i++) {
    newb[i] = 0;
    for (int j = 0; j < 6; j++) newb[i] += trans[i*6+j]*b[j];
  }
  for (int i = 0; i < 6; i++) b[i] = -newb[i];

  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 6; j++) {
      A[i*6+j] = 0;
      for (int k = 0; k < 6; k++) A[i*6+j] += tmp[i*6+k]*trans[j*6+k];
    }
  }
}

struct DepthImage {
  cl::Image2D source_data[scales];
  cl::Image2D dest_data[scales];  
};

struct SimpleDVO {
  cl::Kernel prepare_source;
  cl::Kernel shrink;
  cl::Kernel shrinkPacked;
  cl::Kernel prepare_dest;
  cl::Kernel wrap;
  cl::Kernel sumAb;
  cl::Buffer device_transform;
  cl::Buffer device_projection[scales];
  cl::Buffer device_Ab_part;
  cl::Buffer device_Ab;
  cl::Image2D plot_img;

  float host_img[w*h];

  SimpleDVO() {
#if defined DEBUG
    const char args[] = "-cl-denorms-are-zero -cl-finite-math-only -DDEBUG";
#else
    const char args[] = "-cl-denorms-are-zero -cl-finite-math-only";
#endif
    cl::Program program = createProgram("dvo.cl", args);
    cl::Program dualprogram = createProgram("dualdvo.cl", args);
    prepare_source = cl::Kernel(program, "PackSource");
    shrink = cl::Kernel(program, "shrink");
    shrinkPacked = cl::Kernel(program, "shrinkPacked");
    prepare_dest = cl::Kernel(program, "PrepareDest");
    wrap = cl::Kernel(dualprogram, "VectorWrap");
    sumAb = cl::Kernel(program, "SumAb");

    device_transform = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(float)*12);
    for (int i = 0; i < scales; i++)
      device_projection[i] = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(float)*4);
    int parts = (w/8)*(h/8);
    device_Ab_part = cl::Buffer(context, CL_MEM_READ_WRITE, sizeof(float)*32*parts);
    device_Ab = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(float)*32);

    plot_img = cl::Image2D(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_UNORM_INT8), w, h);
  }
  DepthImage createSource(float*host_img) {return createDepthImage(host_img);}
  DepthImage createDest(float*host_img) {return createDepthImage(host_img);}
  DepthImage createDepthImage(float*host_img) {
    DepthImage result;


    cl::Image2D src_depth;
    src_depth = cl::Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_R, CL_FLOAT), w, h, 0, host_img);

    for (int i = 0; i < scales; i++) 
      result.source_data[i] = cl::Image2D(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_HALF_FLOAT), w>>i+1, h>>i+1);

    prepare_source.setArg(0, src_depth);
    prepare_source.setArg(1, result.source_data[0]);
    queue.enqueueNDRangeKernel(prepare_source, cl::NullRange, cl::NDRange(w>>1, h>>1), cl::NullRange);

    for (int i = 1; i < scales; i++) {
      shrinkPacked.setArg(0, result.source_data[i-1]);
      shrinkPacked.setArg(1, result.source_data[i]);
      queue.enqueueNDRangeKernel(shrinkPacked, cl::NullRange, cl::NDRange(w>>i+1, h>>i+1), cl::NullRange);
    }


    cl::Image2D dest_depth[scales];

    dest_depth[0] = cl::Image2D(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, cl::ImageFormat(CL_R, CL_FLOAT), w, h, 0, host_img);

    for (int i = 0; i < scales; i++) {
      result.dest_data[i] = cl::Image2D(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_RGBA, CL_HALF_FLOAT), w>>i, h>>i);

      if (i) 
	dest_depth[i] = cl::Image2D(context, CL_MEM_READ_WRITE, cl::ImageFormat(CL_R, CL_HALF_FLOAT), w>>i, h>>i);
    }
    for (int i = 0; i < scales; i++) {
      if (i) {
	shrink.setArg(0, dest_depth[i-1]);
	shrink.setArg(1, dest_depth[i]);
	queue.enqueueNDRangeKernel(shrink, cl::NullRange, cl::NDRange(w>>i, h>>i), cl::NullRange);
      }
      prepare_dest.setArg(0, dest_depth[i]);
      prepare_dest.setArg(1, result.dest_data[i]);
      queue.enqueueNDRangeKernel(prepare_dest, cl::NullRange, cl::NDRange(w>>i, h>>i), cl::NullRange);
    }
    return result;
  }

  void calcGaussNewton(DepthImage&src, DepthImage&dest, int scale, float*Ab, float ivar = 500.f) {
    wrap.setArg(0, src.source_data[scale]);
    wrap.setArg(1, dest.dest_data[scale]);
    wrap.setArg(2, device_transform);
    wrap.setArg(3, device_projection[scale]);
    wrap.setArg(4, device_Ab_part);
    wrap.setArg(5, plot_img);
    wrap.setArg(6, ivar);

    sumAb.setArg(0, device_Ab_part);
    sumAb.setArg(1, device_Ab);
    int parts = (w*h>>6)>>scale*2+2;
    //cout << parts << endl;
    sumAb.setArg(2, sizeof(int), &parts);

    queue.enqueueNDRangeKernel(wrap, cl::NullRange, cl::NDRange(w>>scale+1, h>>scale+1), cl::NDRange(8, 8));

#if defined DEBUG
    if (plotting)
      plotGpu(plot_img, w>>scale, h>>scale);
#endif

    queue.enqueueNDRangeKernel(sumAb, cl::NullRange, cl::NDRange(32), cl::NullRange);
    queue.enqueueReadBuffer(device_Ab, 0, 0, sizeof(float)*29, Ab);
  }

  void calcPose(DepthImage&src, DepthImage&dest, se3&pose) {
    float ivar = 250;

    float Dcx = 255.559, Dcy = 207.671+(512-424)/2.f, Dpers = 365.463;

    for (int scale = 0; scale < scales; scale++) {
      float f = 1.f/(1<<scale);
      float scaledprojection[4] = {(Dcx+0.5f)*f-0.5f, (Dcy+0.5f)*f-0.5f, Dpers*f, 1.f/(f*Dpers)};

      queue.enqueueWriteBuffer(device_projection[scale], 0, 0, sizeof(float)*4, scaledprojection);
    }

    float transform[12];

    float Ab[29*2];
    for (int scale = scales-1; scale >= 0; scale--) {
      float lasterror = 1e9;
      se3 lastpose = pose;
      for (int iter = 0; iter < 10; iter++) {
#if defined DEBUG
	cout << "Scale / iteration: " << scale << ' ' << iter << endl;
#endif
	pose.exp(transform,0);
	queue.enqueueWriteBuffer(device_transform, 0, 0, sizeof(float)*12, transform);
	calcGaussNewton(src, dest, scale, Ab, ivar);
	(pose*-1).exp(transform,0);
	queue.enqueueWriteBuffer(device_transform, 0, 0, sizeof(float)*12, transform);
	calcGaussNewton(dest, src, scale, Ab+29, ivar);


	float error = (Ab[28]+Ab[28+29])/(Ab[27]+Ab[27+29]);
	if (error != error or error > lasterror*.99) {
	  pose = lastpose;
	  break;
	}
	lastpose = se3(pose.l);
	lasterror = error;
	

	float A[36*2], b[6*2], x[6] = {0}, tmp1[6], tmp2[6];
	for (int k = 0; k < 2; k++) {
	  int c = 0;
	  for (int j = 0; j < 6; j++) 
	    for (int i = 0; i <= j; i++) 
	      A[i+j*6+36*k] = Ab[29*k+c++];
	  for (int j = 0; j < 6; j++)
	    for (int i = j+1; i < 6; i++)
	      A[i+j*6+36*k] = A[j+i*6+36*k];
	  for (int i = 0; i < 6; i++) b[6*k+i] = -Ab[29*k+c++];
	}
	fix(A, b, pose);
	for (int i = 0; i < 36; i++) A[i] += A[i+36];
	for (int i = 0; i < 6; i++) b[i] += b[i+6];

	solve(A, b, x, tmp1, tmp2, 6);
	//for (int i = 0; i < 6; i++) cout << x[i] << ' ';
	//cout << endl;
	se3 updateView(x);
	//pose = updateView*pose;
	pose = pose*(updateView*-1);

	//p = p*-f(-p);
	//p = f(p)*p;
      }
    }
  }
};
