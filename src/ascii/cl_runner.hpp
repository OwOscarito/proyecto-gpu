#pragma once

#include <CL/cl.h>
#include <CL/cl_platform.h>

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

class ClRunner {
 public:
  ClRunner();
  ~ClRunner();

  void init();

  template <typename... Args>
  requires(std::is_convertible_v<Args, std::string> &&...) auto load_program(
      const fs::path &path,
      Args &&...names);
  template <typename T>
  void set_argument(cl_kernel kernel, cl_int index, const T &value);

  cl_mem create_buffer(size_t size, cl_mem_flags flags, void *data = nullptr);
  void write_buffer(cl_mem buffer, size_t size, const void *data);
  template <typename T>
  std::vector<T> read_buffer(cl_mem buffer, size_t size);

  void run_kernel(cl_kernel kernel,
                  size_t *global_size,
                  size_t *local_size,
                  cl_uint work_dim = 1);

 private:
  cl_platform_id platform = nullptr;
  cl_device_id device = nullptr;
  cl_context context = nullptr;
  cl_command_queue queue = nullptr;
  cl_program program = nullptr;

  std::vector<cl_kernel> kernels;
  std::vector<cl_mem> buffers;

  bool init_device(std::vector<cl_platform_id> &platforms,
                   cl_device_type device_type);
  void load_program(const fs::path &program_path);
  cl_kernel load_kernel(const std::string &kernel_name);
  void check_error(cl_int err, const std::string &msg);
};

template <typename... Args>
requires(std::is_convertible_v<Args, std::string>
             &&...) auto ClRunner::load_program(const fs::path &program_path,
                                                Args &&...names) {
  load_program(program_path);
  return std::make_tuple(load_kernel(std::forward<Args>(names))...);
}

template <typename T>
void ClRunner::set_argument(cl_kernel kernel, cl_int index, const T &value) {
  cl_int err;
  err = clSetKernelArg(kernel, index, sizeof(T), &value);
  check_error(err,
              "Failed to set kernel argument[" + std::to_string(index) + "]");
}

template <typename T>
std::vector<T> ClRunner::read_buffer(cl_mem buffer, size_t buffer_size) {
  size_t buffer_length = buffer_size / sizeof(T);
  std::vector<T> buffer_data(buffer_length);

  cl_int err = clEnqueueReadBuffer(this->queue,
                                   buffer,
                                   CL_TRUE,
                                   0,
                                   buffer_size,
                                   buffer_data.data(),
                                   0,
                                   nullptr,
                                   nullptr);
  check_error(err, "Failed to read buffer");

  return buffer_data;
}
