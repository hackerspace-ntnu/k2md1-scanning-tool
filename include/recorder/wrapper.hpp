#include <iostream>
#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
#include <libfreenect2/packet_pipeline.h>
#include <unistd.h>
#include <stdlib.h>

using namespace libfreenect2;
using namespace std;

static const int depth_width = 512, depth_height = 424, color_width = 1920, color_height = 1080; // Camera resolutions

struct MyKinect {
  Freenect2 freenect2;
  SyncMultiFrameListener*listener;
  FrameMap frames;
  int gotFrame;
  Freenect2Device*dev;
  PacketPipeline*pipeline;
  MyKinect() {
    int devices = freenect2.enumerateDevices();
    if (!devices) {
      cout << "Could not find kinect" << endl;
      cout << "Have you connected it properly?" << endl;
      exit(0);
    }
    std::string serial = freenect2.getDefaultDeviceSerialNumber();
    pipeline = new OpenGLPacketPipeline();
    dev = freenect2.openDevice(serial, pipeline);
    listener = new SyncMultiFrameListener(Frame::Depth|Frame::Color);
    dev->setColorFrameListener(listener);
    dev->setIrAndDepthFrameListener(listener);
    dev->start();
    gotFrame = 0;
  }
  ~MyKinect() {
    dev->stop();
    dev->close();
    delete listener;
  }
  void getColorAndDepth(unsigned int**col, float**depth) {
    if (gotFrame) {
      listener->release(frames);
      gotFrame = 0;
    }
    if (listener->hasNewFrame()) {
      listener->waitForNewFrame(frames);
      gotFrame = 1;

      *depth = (float*)frames[Frame::Depth]->data;
      *col = (unsigned int*)frames[Frame::Color]->data;
    }
  }
  void waitForFrame() {
    while (!listener->hasNewFrame()) {
      usleep(30000);
    }
  }
};
