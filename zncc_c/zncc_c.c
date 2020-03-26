/* Tomi Lehto 2508005, Julius Rintamäki 2507255 */

#define MAX_DISP 50
#define MIN_DISP 0
#define WIN_SIZE 5
#define THRESHOLD 12

#include "../LodePNG/lodepng.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void zncc(unsigned char* il, unsigned char* ir, unsigned int w, unsigned int h, int d_max, int d_min, unsigned char* d_map);
void post_processing(unsigned char* disparity_l2r, unsigned char* disparity_r2l, unsigned char* result, int img_size);

int main(int argc, char *argv[])
{
    
    const char* input_file_left  = /*argc > 1 ? argv[1] :*/ "imageL.png";
    const char* input_file_right  = /*argc > 2 ? argv[2] :*/ "imageR.png";
    const char* output_file = /*argc > 3 ? argv[3] :*/ "output.png";

    unsigned error;
    /* Left image */
    unsigned char* il = 0;
    /* Left image */
    unsigned char* ir = 0;
    /* width, height */
    unsigned int w, h;
    unsigned char* disparity_l2r;
    unsigned char* disparity_r2l;
    unsigned char* final_result;

    error = lodepng_decode_file(&il, &w, &h, input_file_left, LCT_GREY, 8);
    if (error) 
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        printf("usage: ./zncc_c input_filename1 input_filename2 [output_filename]\n");
        return 0;
    }
    error = lodepng_decode_file(&ir, &w, &h, input_file_right, LCT_GREY, 8);
    if (error) 
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        printf("usage: ./zncc_c input_filename1 input_filename2 [output_filename]\n");
        return 0;
    }

    /* Disparity */
    disparity_l2r = calloc(w * h, sizeof(unsigned char));
    disparity_r2l = calloc(w * h, sizeof(unsigned char));

    zncc(il, ir, w, h, MIN_DISP, MAX_DISP, disparity_l2r);
    zncc(ir, il, w, h, -MAX_DISP, MIN_DISP, disparity_r2l);

    lodepng_encode_file("l2r.png", disparity_l2r, w, h, LCT_GREY, 8);
    lodepng_encode_file("r2l.png", disparity_r2l, w, h, LCT_GREY, 8);

    /* Post-processing */
    final_result = calloc(w * h, sizeof(unsigned char));
    post_processing(disparity_l2r, disparity_r2l, final_result, w*h);

    /* Encode result */
    lodepng_encode_file(output_file, final_result, w, h, LCT_GREY, 8);
    free(disparity_l2r);
    free(disparity_r2l);
    free(final_result);

    return 0;
}

void zncc(unsigned char* il, unsigned char* ir, unsigned int w, unsigned int h, int d_min, int d_max, unsigned char* d_map)
{
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

    for (int i = 0; i < h; i++)
    for (int j = 0; j < w; j++)
    {
        current_max = -1;
        best_disparity_value = d_max;

        for (int d = d_min; d <= d_max; d++)
        {
            /* ---------------------------------------- */
            /* CALCULATE THE MEAN VALUE FOR EACH WINDOW */
            /* ---------------------------------------- */
            sum_left_window = 0;
            sum_right_window = 0;
            for (int win_i = -WIN_SIZE / 2; win_i < WIN_SIZE/2; win_i++)
            for (int win_j = -WIN_SIZE / 2; win_j < WIN_SIZE/2; win_j++) 
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
            /* Calculate means by diving with amount of pixels in window */
            sum_left_window /= (WIN_SIZE * WIN_SIZE);
            sum_right_window /= (WIN_SIZE * WIN_SIZE);

            /* ---------------------------------------- */
            /* CALCULATE THE ZNCC VALUE FOR EACH WINDOW */
            /* ---------------------------------------- */
            nominator = 0;
            denominator_1 = 0;
            denominator_2 = 0;

            for (int win_i = -WIN_SIZE / 2; win_i < WIN_SIZE/2; win_i++)
            for (int win_j = -WIN_SIZE / 2; win_j < WIN_SIZE/2; win_j++) 
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
            zncc_value = nominator / (sqrt(denominator_1) * sqrt(denominator_2));

            if (zncc_value >= current_max)
            {
                current_max = zncc_value;
                best_disparity_value = d;
            }
        }
        d_map[i * w + j] = (unsigned char) abs(best_disparity_value);
    }
}

void post_processing(unsigned char* disparity_l2r, unsigned char* disparity_r2l, unsigned char* result, int img_size)
{
    unsigned char max = 0;
    unsigned char min = 255;
    printf("Post \n");

    for (int idx = 0; idx < img_size; idx++)
    {
        if (abs(disparity_l2r[idx] - disparity_r2l[idx]) > THRESHOLD)
        {
            result[idx] = 0;
        }
        else{
            result[idx] = disparity_l2r[idx];
        }
    }
    //Normalize

    for (int i = 0; i < img_size; i++)
    {
        if (result[i] > max)
        {
            max = result[i];
        }
        if (result[i] < min)
        {
            min = result[i];
        }
    }

    for (int i = 0; i < img_size; i++)
    {
        result[i] = (unsigned char) (255*(result[i]- min)/(max - min));
    }


}
