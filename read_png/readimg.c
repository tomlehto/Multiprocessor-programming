/* Tomi Lehto 2508005, Julius Rintamäki 2507255 */

#include "lodepng.h"

#include <stdio.h>
#include <stdlib.h>

void threshold_image(unsigned char* image, unsigned int width, unsigned int height);

int main(int argc, char *argv[])
{
    const char* input_file  = argc > 1 ? argv[1] : "lena.png";
    const char* output_file = argc > 2 ? argv[2] : "output.png";

    unsigned error;
    unsigned char* image = 0;
    unsigned width, height;

    error = lodepng_decode_file(&image, &width, &height, input_file, LCT_GREY, 8);
    if (error) 
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        printf("usage: ./readimg input_filename [output_filename]\n");
        return 0;
    }
    threshold_image(image, width, height);
    lodepng_encode_file(output_file, image, width, height, LCT_GREY, 8);
    free(image);

    return 0;
}

void threshold_image(unsigned char* image, unsigned int width, unsigned int height)
{
    unsigned char* pixel;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            pixel = &(image[y * width + x]);
            if (*pixel < 128)
            {
                *pixel = 0;
            }
        }
    }
}
