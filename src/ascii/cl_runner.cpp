#include "cl_runner.h"

#include <CL/cl.h>
#include <CL/cl_platform.h>

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
    throw std::runtime_error(msg + " [Code: " + std::to_string(err) + "]");
  }
}
bool ClRunner::init_device(std::vector<cl_platform_id> platforms,
                           cl_device_type device_type) {
  cl_int err;
  for (std::size_t i = 0; i < platforms.size(); ++i) {
    this->platform = platforms[i];
    err =
        clGetDeviceIDs(this->platform, device_type, 1, &this->device, nullptr);
    if (err == CL_SUCCESS) return true;
  }
  return false;
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

  if (!init_device(platforms, CL_DEVICE_TYPE_GPU) &&
      !init_device(platforms, CL_DEVICE_TYPE_DEFAULT) &&
      !init_device(platforms, CL_DEVICE_TYPE_CPU) &&
      !init_device(platforms, CL_DEVICE_TYPE_ALL)) {
    throw std::runtime_error("Couldn't set device");
  }

  // Init OpenCL context
  this->context =
      clCreateContext(nullptr, 1, &this->device, nullptr, nullptr, &err);
  check_error(err, "Couldn't create context");

  // Init OpenCL command queue
  cl_queue_properties properties[] = {
      CL_QUEUE_PROPERTIES,
      CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_PROFILING_ENABLE,
      0  // Terminator
  };
  this->queue = clCreateCommandQueueWithProperties(
      this->context, this->device, properties, &err);
  check_error(err, "Couldn't create command queue");

  std::cout << "OpenCL initialized successfully" << std::endl;
}

void ClRunner::setup_kernel(const fs::path &kernel_path,
                            const std::string &entrypoint = "main") {
  std::ifstream kernel_file(kernel_path);
  if (!kernel_file.is_open()) {
    throw std::runtime_error("Failed to open kernel file:" +
                             kernel_path.string());
  }
  std::ostringstream stream;
  stream << kernel_file.rdbuf();  // Read the entire file into the stream
  std::string kernel_string = stream.str();
  const char *kernel_src = kernel_string.c_str();

  cl_int err;
  this->program =
      clCreateProgramWithSource(this->context, 1, &kernel_src, nullptr, &err);
  check_error(err,
              "Failed to create program with source [" + kernel_path.string() +
                  "]:\n" + kernel_string);

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

  std::cout << "Kernel loaded" << kernel_path << std::endl;
}

void ClRunner::set_buffer_argument(cl_uint index,
                                   const std::string &name,
                                   size_t size,
                                   cl_mem_flags flags,
                                   void *data) {
  cl_int err;
  cl_mem buffer = clCreateBuffer(this->context, flags, size, data, &err);
  check_error(err, "Couldn't create buffer:" + name);

  this->buffers[name] = buffer;
  this->buffer_sizes[name] = size;

  err = clSetKernelArg(this->kernel, index, sizeof(cl_mem), &buffer);
  check_error(err, "Couldn't set arg:" + name);
}

void ClRunner::write_buffer(const std::string &name, const void *data) {
  auto buffer_size = this->buffer_sizes.at(name);
  write_buffer(name, data, buffer_size);
}

void ClRunner::write_buffer(const std::string &name,
                            const void *data,
                            size_t size) {
  auto buffer = this->buffers.at(name);

  cl_int err = clEnqueueWriteBuffer(
      this->queue, buffer, CL_TRUE, 0, size, data, 0, nullptr, nullptr);
  check_error(err, "Couldn't write buffer: " + name);
}

void ClRunner::run_kernel(size_t global_size,
                          size_t local_size,
                          cl_uint work_dim) {
  if (!this->kernel) {
    throw std::runtime_error("No kernel loaded!");
  }

  if (local_size == 0) {
    cl_uint max_groups_size;
    cl_int err = clGetDeviceInfo(device,
                                 CL_DEVICE_MAX_WORK_GROUP_SIZE,
                                 sizeof(max_groups_size),
                                 &max_groups_size,
                                 nullptr);
    check_error(err, "Couldn't get device info");

    local_size = std::min((size_t)256, (size_t)max_groups_size);
  }

  // 2. Enqueue the kernel execution
  cl_int err = clEnqueueNDRangeKernel(this->queue,
                                      this->kernel,
                                      work_dim,
                                      nullptr,
                                      &global_size,  // Work-items total
                                      &local_size,  // Work-items per work-group
                                      0,
                                      nullptr,
                                      nullptr);
  check_error(err, "Coudln't to enqueue kernel");

  err = clFinish(this->queue);
  check_error(err, "Coudln't finish queue");
}
