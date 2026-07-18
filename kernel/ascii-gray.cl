__kernel void zones(__global const uchar* image,
                    __global float* zones,
                    int img_w, int img_h,
                    int block_w, int block_h) {
    int zx = get_global_id(0);
    int zy = get_global_id(1);

    int zones_w = (img_w + block_w - 1) / block_w;
    int zones_h = (img_h + block_h - 1) / block_h;

    if (zx >= zones_w || zy >= zones_h) return;

    float sum = 0.0f;
    int count = 0;

    for (int y = 0; y < block_h; y++) {
        for (int x = 0; x < block_w; x++) {
            int px = zx * block_w + x;
            int py = zy * block_h + y;

            if (px < img_w && py < img_h) {
                int idx = (py * img_w + px) * 3;
                sum += 0.299f * image[idx] + 0.587f * image[idx + 1] + 0.114f * image[idx + 2];
                count++;
            }
        }
    }

    int zone_idx = zy * zones_w + zx;
    zones[zone_idx] = (count > 0) ? sum / count : 0.0f;
}

__kernel void ascii(__global const float* zones,
                    __global char* ascii_output, // Make sure this name matches your C++ set_argument!
                    int total_zones) {
    int idx = get_global_id(0);
    if (idx >= total_zones) return;

    // FIX 1: Use __constant for string literals
    __constant char ramp[] = " .:-=+*#%@";
    int ramp_len = 10;

    float brightness = zones[idx];
    int char_idx = (int)(brightness / 255.0f * (ramp_len - 1));

    // Safety clamp
    if (char_idx >= ramp_len) char_idx = ramp_len - 1;
    if (char_idx < 0) char_idx = 0;

    ascii_output[idx] = ramp[char_idx];
}
