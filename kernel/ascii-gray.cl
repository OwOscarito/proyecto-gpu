__constant char RAMP[] = " .:-=+*#%@";
__constant int RAMP_LEN = 10;

__kernel void ascii(__global const uchar* image,
                            __global char* ascii,
                            int img_w, int img_h,
                            int block_w, int block_h,
                            int zones_w, int zones_h) {
    int zx = get_global_id(0);
    int zy = get_global_id(1);
    if (zx >= zones_w || zy >= zones_h) return;

    float sum = 0.0f;
    int count = 0;
    for (int y = 0; y < block_h; y++) {
      int py = zy * block_h + y;
      if (py >= img_h) continue;
      for (int x = 0; x < block_w; x++) {
        int px = zx * block_w + x;
        if (px < img_w) {
          int idx = (py * img_w + px) * 3;
          sum += 0.299f * image[idx] + 0.587f * image[idx + 1] + 0.114f * image[idx + 2];
          count++;
        }
      }
    }

    float brightness = (count > 0) ? sum / count : 0.0f;
    int char_idx = (int)(brightness / 255.0f * (RAMP_LEN - 1));
    char_idx = clamp(char_idx, 0, RAMP_LEN - 1);

    int zone_idx = zy * zones_w + zx;
    ascii[zone_idx] = RAMP[char_idx];
}
