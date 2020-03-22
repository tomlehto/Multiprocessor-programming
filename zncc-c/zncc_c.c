/* Tomi Lehto 2508005, Julius Rintamäki 2507255 */

#define MAX_DISP 50
#define WIN_SIZE 5
#define THRESHOLD 12

#include "../LodePNG/lodepng.h"

#include <stdio.h>
#include <stdlib.h>

void disparity(unsigned char* il, unsigned char* ir, unsigned int m, unsigned int n, unsigned char* disparity_l2r, unsigned char* disparity_r2l);
void post_processing(unsigned char* disparity_l2r, unsigned char* disparity_r2l, unsigned char* result);

int main(int argc, char *argv[])
{
    const char* input_file_left  = argc > 1 ? argv[1] : "imageL.png";
    const char* input_file_right  = argc > 1 ? argv[1] : "imageR.png";
    const char* output_file = argc > 2 ? argv[2] : "output.png";

    unsigned error;
    /* Left image */
    unsigned char* il = 0;
    /* Left image */
    unsigned char* ir = 0;
    /* [M,N]*/
    unsigned int m, n;
    unsigned char* disparity_l2r;
    unsigned char* disparity_r2l;
    unsigned char* final_result;

    error = lodepng_decode_file(&il, &m, &n, input_file_left, LCT_GREY, 8);
    if (error) 
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        printf("usage: ./zncc_c input_filename1 input_filename2 [output_filename]\n");
        return 0;
    }
    error = lodepng_decode_file(&ir, &m, &n, input_file_right, LCT_GREY, 8);
    if (error) 
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        printf("usage: ./zncc_c input_filename1 input_filename2 [output_filename]\n");
        return 0;
    }

    /* Disparity */
    disparity_l2r = calloc(m * n, sizeof(unsigned char));
    disparity_r2l = calloc(m * n, sizeof(unsigned char));
    disparity(il, ir, m, n, disparity_l2r, disparity_r2l);


    /* Post-processing */
    final_result = calloc(m * n, sizeof(unsigned char));
    post_processing(disparity_l2r, disparity_r2l, final_result);

    /* Encode result */
    lodepng_encode_file(output_file, final_result, m, n, LCT_GREY, 8);
    free(image);

    return 0;
}

void disparity(unsigned char* il, unsigned char* ir, unsigned int m, unsigned int n, unsigned char* disparity_l2r, unsigned char* disparity_r2l)
{

}

void post_processing(unsigned char* disparity_l2r, unsigned char* disparity_r2l, unsigned char* result)
{

}
