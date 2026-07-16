#include "cl_runner.h"

#include <CL/cl.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

ClRunner::ClRunner()
    : platform(nullptr),
      device(nullptr),
      context(nullptr),
      queue(nullptr),
      program(nullptr),
      kernel(nullptr) {}

ClRunner::~ClRunner() {
  for (auto &[name, buf] : buffers) {
    clReleaseMemObject(buf);
  }
  if (this->kernel) clReleaseKernel(kernel);
  if (this->program) clReleaseProgram(program);
  if (this->queue) clReleaseCommandQueue(queue);
  if (this->context) clReleaseContext(context);
}

void ClRunner::check_error(cl_int err, const std::string &msg) {
  if (err != CL_SUCCESS) {
    throw std::runtime_error(msg + " [" + std::to_string(err) + "]");
  }
}

void ClRunner::init() {
  // Init OpenCL Platform
  cl_uint num_platforms;
  cl_int err;

  err = clGetPlatformIDs(0, nullptr, &num_platforms);
  check_error(err, "Failed to get platform count");
  if (num_platforms == 0) {
    throw std::runtime_error("No OpenCL platforms found");
  }
  std::vector<cl_platform_id> platforms(num_platforms);
  err = clGetPlatformIDs(num_platforms, platforms.data(), nullptr);
  check_error(err, "Failed to get platform IDs");

  this->platform = platforms[0];

  // Init OpenCL Device
  cl_uint num_devices;
  err = clGetDeviceIDs(
      platform, CL_DEVICE_TYPE_GPU, 1, &this->device, &num_devices);
  if (err != CL_SUCCESS || num_devices == 0) {
    // CPU Fallback
    err = clGetDeviceIDs(
        platform, CL_DEVICE_TYPE_CPU, 1, &this->device, &num_devices);
    check_error(err, "Failed to get any OpenCL device");
  }

  // Init OpenCL context
  this->context =
      clCreateContext(nullptr, 1, &this->device, nullptr, nullptr, &err);
  check_error(err, "Failed to create context");

  // Init OpenCL command queue
  cl_queue_properties properties[] = {
      CL_QUEUE_PROPERTIES,
      CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE,
      0  // Terminator
  };
  this->queue = clCreateCommandQueueWithProperties(
      this->context, this->device, properties, &err);
  check_error(err, "Failed to create command queue");

  std::cout << "OpenCL initialized successfully! (◕‿◕)" << std::endl;
}

void ClRunner::setup_kernel(const fs::path &kernel_path,
                            const std::string &entrypoint = "main") {
  std::ifstream kernel_file(kernel_path);
  if (!kernel_file.is_open()) {
    throw std::runtime_error("Failed to open kernel file");
  }
  std::ostringstream stream;
  stream << kernel_file.rdbuf();  // Read the entire file into the stream
  std::string kernel_string = stream.str();
  const char *kernel_src = kernel_string.c_str();

  cl_int err;
  this->program =
      clCreateProgramWithSource(context, 1, &kernel_src, nullptr, &err);
  check_error(err, "Failed to create program");

  err = clBuildProgram(program, 1, &this->device, nullptr, nullptr, nullptr);

  if (err != CL_SUCCESS) {
    size_t log_size;
    clGetProgramBuildInfo(this->program,
                          this->device,
                          CL_PROGRAM_BUILD_LOG,
                          0,
                          nullptr,
                          &log_size);

    std::vector<char> log(log_size);
    clGetProgramBuildInfo(this->program,
                          this->device,
                          CL_PROGRAM_BUILD_LOG,
                          log_size,
                          log.data(),
                          nullptr);
    std::cerr << "Kernel build error:\n" << log.data() << std::endl;
    throw std::runtime_error("Kernel compilation failed");
  }

  this->kernel = clCreateKernel(this->program, entrypoint.c_str(), &err);
  check_error(err, "Failed to create kernel");

  std::cout << "Kernel loades" << kernel_path << std::endl;
}

void ClRunner::setup_buffer(cl_uint kernel_arg,
                            const std::string &name,
                            size_t size,
                            cl_mem_flags flags) {
  cl_int err;
  cl_mem buffer = clCreateBuffer(context, flags, size, nullptr, &err);
  check_error(err, "Couldn't create buffer:" + name);

  this->buffers[name] = buffer;
  this->buffer_sizes[name] = size;

  err = clSetKernelArg(kernel, kernel_arg, sizeof(cl_mem), &buffer);
  check_error(err, "Couldn't set arg:" + name);
}

void ClRunner::write_buffer(const std::string &name, const void *host_ptr) {
  auto buffer_size = buffer_sizes.at(name);
  write_buffer(name, host_ptr, buffer_size);
}

void ClRunner::write_buffer(const std::string &name,
                            const void *host_ptr,
                            size_t size) {
  auto buffer = buffers.at(name);

  cl_int err = clEnqueueWriteBuffer(
      queue, buffer, CL_TRUE, 0, size, host_ptr, 0, nullptr, nullptr);
  check_error(err, "Couldn't write buffer: " + name);
}
