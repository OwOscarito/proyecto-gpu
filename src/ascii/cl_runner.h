#pragma once

#include <CL/cl.h>
#include <CL/cl_platform.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

class ClRunner {
 public:
  ClRunner();
  ~ClRunner();

  void init();
  void setup_kernel(const fs::path &kernel_path, const std::string &entrypoint);
  void set_buffer_argument(cl_uint index,
                           const std::string &name,
                           size_t size,
                           cl_mem_flags flags,
                           void *data = nullptr);
  void write_buffer(const std::string &name, const void *data);
  void write_buffer(const std::string &name, const void *data, size_t size);

  template <typename T>
  void set_argument(cl_int index, const T &value);

  template <typename T>
  std::vector<T> read_buffer(const std::string &buffer_name);

  void run_kernel(size_t global_size,
                  size_t local_size = 0,
                  cl_uint work_dim = 1);

 private:
  cl_platform_id platform;
  cl_device_id device;
  cl_context context;
  cl_command_queue queue;
  cl_program program;
  cl_kernel kernel;

  std::unordered_map<std::string, cl_mem> buffers;
  std::unordered_map<std::string, size_t> buffer_sizes;

  bool init_device(std::vector<cl_platform_id> platforms,
                   cl_device_type device_type);
  void check_error(cl_int err, const std::string &msg);
};

template <typename T>
void ClRunner::set_argument(cl_int index, const T &value) {
  cl_int err;
  err = clSetKernelArg(this->kernel, index, sizeof(T), &value);
  check_error(err, "Couldn't set arg[" + std::to_string(index) + "]");
}

template <typename T>
std::vector<T> ClRunner::read_buffer(const std::string &name) {
  auto buffer = this->buffers.at(name);

  size_t num_elements = this->buffer_sizes[name] / sizeof(T);
  std::vector<T> buffer_data(num_elements);

  cl_int err = clEnqueueReadBuffer(this->queue,
                                   buffer,
                                   CL_TRUE,
                                   0,
                                   this->buffer_sizes[name],
                                   buffer_data.data(),
                                   0,
                                   nullptr,
                                   nullptr);
  check_error(err, "Couldn't read buffer: " + name);

  return buffer_data;
}
