#include "cl_runner.hpp"

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

ClRunner::ClRunner() {}

ClRunner::~ClRunner() {
  for (auto buffer : this->buffers) {
    clReleaseMemObject(buffer);
  }
  buffers.clear();

  for (auto kernel : this->kernels) {
    clReleaseKernel(kernel);
  }
  if (this->program) clReleaseProgram(program);
  if (this->queue) clReleaseCommandQueue(queue);
  if (this->context) clReleaseContext(context);
}

void ClRunner::check_error(cl_int err, const std::string &msg) {
  if (err != CL_SUCCESS) {
    throw std::runtime_error("OpenCL Error [Code: " + std::to_string(err) +
                             "]:" + msg);
  }
}
bool ClRunner::init_device(std::vector<cl_platform_id> &platforms,
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
    throw std::runtime_error("Failed to set device");
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
}

void ClRunner::load_program(const fs::path &program_path) {
  std::ifstream program_file(program_path);
  if (!program_file.is_open()) {
    throw std::runtime_error("Failed to open program file [" +
                             program_path.string() + "]");
  }
  std::ostringstream stream;
  stream << program_file.rdbuf();
  std::string program_string = stream.str();
  const char *program_source = program_string.c_str();

  cl_int err;
  this->program = clCreateProgramWithSource(
      this->context, 1, &program_source, nullptr, &err);
  check_error(err, "Failed to create program [" + program_path.string() + "]");

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
    throw std::runtime_error("Program compilation failed");
  }
}

cl_kernel ClRunner::load_kernel(const std::string &kernel_name) {
  cl_int err;
  cl_kernel kernel = clCreateKernel(this->program, kernel_name.c_str(), &err);
  check_error(err, "Failed to create kernel [" + kernel_name + "]");
  this->kernels.push_back(kernel);

  return kernel;
}

cl_mem ClRunner::create_buffer(size_t size, cl_mem_flags flags, void *data) {
  cl_int err;
  cl_mem buffer = clCreateBuffer(this->context, flags, size, data, &err);
  check_error(err, "Failed to create buffer");
  this->buffers.push_back(buffer);

  return buffer;
}

void ClRunner::write_buffer(cl_mem buffer, size_t size, const void *data) {
  cl_int err = clEnqueueWriteBuffer(
      this->queue, buffer, CL_TRUE, 0, size, data, 0, nullptr, nullptr);
  check_error(err, "Failed to write buffer");
}

void ClRunner::run_kernel(cl_kernel kernel,
                          size_t *global_size,
                          size_t *local_size,
                          cl_uint work_dim) {
  cl_int err = clEnqueueNDRangeKernel(this->queue,
                                      kernel,
                                      work_dim,
                                      nullptr,
                                      global_size,  // Work-items total
                                      local_size,   // Work-items per work-group
                                      0,
                                      nullptr,
                                      nullptr);
  check_error(err, "Failed to enqueue kernel");

  err = clFinish(this->queue);
  check_error(err, "Failed to finish queue");
}
