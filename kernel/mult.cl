__kernel void mult (
  __global const float *input,
  __global float *output,
  const float val,
  const int size
) {
  int idx = get_global_id(0);
  if (idx < size) {
    output[idx] = input[idx] * val;
  }
}
