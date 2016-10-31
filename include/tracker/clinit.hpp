#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <iostream>
#include <fstream>

using std::cout;
using std::endl;

static std::vector<cl::Device> devices;
static cl::Context context;
static cl::CommandQueue queue;

void initCL() {
  try {
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
		for (int i = 0; i < platforms.size(); i++) {
			try {
				platforms[i].getDevices(CL_DEVICE_TYPE_GPU, &devices);
				cout << "Using platform #" << i+1 << endl;
				break;
			} catch (cl::Error e) {
				if (e.err() == CL_DEVICE_NOT_FOUND) continue; //No GPU on this platform, try next
				std::cout << std::endl << e.what() << " : " << e.err() << std::endl;
			}
		}
    context = cl::Context(devices);
    queue = cl::CommandQueue(context, devices[0]);
  } catch (cl::Error e) {
    std::cout << std::endl << e.what() << " : " << e.err() << std::endl;
  }
}

cl::Program createProgram(std::string name, const char*args = NULL) {
  std::ifstream cl_file(name.c_str());
  std::string cl_string(std::istreambuf_iterator<char>(cl_file), (std::istreambuf_iterator<char>()));
  cl::Program::Sources source(1, std::make_pair(cl_string.c_str(), cl_string.length() + 1));
  cl::Program program(context, source);
  try {
    program.build(devices, args);
  } catch (cl::Error&e) {
    std::cout << "Build Error:\n" << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0]) << std::endl;
  }
  return program;
}
