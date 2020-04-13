#define WIN_W 20
#define WIN_H 14

__kernel void zncc(__global unsigned char* il, __global unsigned char* ir, unsigned int w, unsigned int h, int d_min, int d_max, __global unsigned char* d_map)
{
    const int i = get_global_id(0);
    const int j = get_global_id(1);

    float current_max;
    float sum_left_window;
    float sum_right_window;
    int left_pixel_idx, right_pixel_idx;
    float nominator = 0;
    float denominator_1 = 0;
    float denominator_2 = 0;
    float center_left;
    float center_right;
    float zncc_value;
    int best_disparity_value = 0;

    current_max = -1;
    best_disparity_value = d_max;

    for (int d = d_min; d <= d_max; d++)
    {
        /* ---------------------------------------- */
        /* CALCULATE THE MEAN VALUE FOR EACH WINDOW */
        /* ---------------------------------------- */
        sum_left_window = 0;
        sum_right_window = 0;

        for (int win_i = -WIN_H/2; win_i < WIN_H/2; win_i++)
        {
            for (int win_j = -WIN_W/2; win_j < WIN_W/2; win_j++)    

            {
                /* Check borders */
                if ((i + win_i < 0)     || (i + win_i >= h) ||
                    (j + win_j < 0)     || (j + win_j >= w) ||
                    (j + win_j - d < 0) || (j + win_j - d >= w))

                {
                    continue;
                }
                left_pixel_idx= w * (i + win_i) + j + win_j;
                right_pixel_idx = w * (i + win_i) + j + win_j - d;

                sum_left_window += il[left_pixel_idx];
                sum_right_window += ir[right_pixel_idx];
            }
        }
        /* Calculate means by diving with amount of pixels in window */
        sum_left_window /= (WIN_W * WIN_H);
        sum_right_window /= (WIN_W * WIN_H);

        /* ---------------------------------------- */
        /* CALCULATE THE ZNCC VALUE FOR EACH WINDOW */
        /* ---------------------------------------- */
        nominator = 0;
        denominator_1 = 0;
        denominator_2 = 0;

        for (int win_i = -WIN_H/2; win_i < WIN_H/2; win_i++)
        {
            for (int win_j = -WIN_W/2; win_j < WIN_W/2; win_j++) 
            {
                /* Check borders */
                if (i + win_i < 0     || i + win_i >= h ||
                    j + win_j < 0     || j + win_j >= w ||
                    j + win_j - d < 0 || j + win_j - d >= w)
                {
                    continue;
                }
                left_pixel_idx= w * (i + win_i) + j + win_j;
                right_pixel_idx = w * (i + win_i) + j + win_j - d;

                center_left = il[left_pixel_idx] - sum_left_window;
                center_right = ir[right_pixel_idx] - sum_right_window;

                nominator += center_left * center_right; 
                denominator_1 += center_left * center_left;
                denominator_2 += center_right * center_right;
            }
        }
        zncc_value = nominator / (native_sqrt(denominator_1) * native_sqrt(denominator_2));

        if (zncc_value > current_max)
        {
            current_max = zncc_value;
            best_disparity_value = d;
        }
    }
    d_map[i * w + j] = (unsigned char) abs(best_disparity_value);
}