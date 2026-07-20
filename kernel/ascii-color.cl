__constant char RAMP[] = " .:-=+*#%@";
#define RAMP_LEN 10

__kernel void ascii(__global const uchar* image,
                           __global char* ascii,
                           __global uchar* color,
                           int img_w, int img_h,
                           int block_w, int block_h) {
    int zx = get_global_id(0);
    int zy = get_global_id(1);
    int zones_w = (img_w + block_w - 1) / block_w;
    int zones_h = (img_h + block_h - 1) / block_h;
    if (zx >= zones_w || zy >= zones_h) return;

    float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f;
    int count = 0;
    for (int y = 0; y < block_h; y++) {
        int py = zy * block_h + y;
        if (py >= img_h) continue;
        for (int x = 0; x < block_w; x++) {
            int px = zx * block_w + x;
            if (px < img_w) {
                int idx = (py * img_w + px) * 3;
                sum_r += image[idx];
                sum_g += image[idx + 1];
                sum_b += image[idx + 2];
                count++;
            }
        }
    }

    float r = 0.0f, g = 0.0f, b = 0.0f, brightness = 0.0f;
    if (count > 0) {
        r = sum_r / count;
        g = sum_g / count;
        b = sum_b / count;
        brightness = 0.299f * r + 0.587f * g + 0.114f * b;
    }

    int char_idx = clamp((int)(brightness / 255.0f * (RAMP_LEN - 1)), 0, RAMP_LEN - 1);

    int zone_idx = zy * zones_w + zx;
    ascii_output[zone_idx] = RAMP[char_idx];

    int c_idx = zone_idx * 3;
    color_output[c_idx] = (uchar)r;
    color_output[c_idx + 1] = (uchar)g;
    color_output[c_idx + 2] = (uchar)b;
}
