/* Tomi Lehto 2508005, Julius Rintamäki 2507255
 * OpenCL implementation for 5x5 box blur (average) of images*/

#include "../LodePNG/lodepng.h"

#include <stdio.h>
#include <stdlib.h>

unsigned char* read_image(char* filename, unsigned int* width, unsigned int* height);
void write_image(char* filename, unsigned char* image, unsigned int width, unsigned int height);

int main(int argc, char *argv[])
{
    unsigned char* image = NULL;
    unsigned int image_width = 0, image_height = 0;

    image = read_image(NULL, &image_width, &image_height);

    if (image == NULL)
    {
        printf("Failure in reading image!\n");
        return 1;
    }

    write_image(NULL, image, image_width, image_height);

    return 0;
}
        
unsigned char* read_image(char* filename, unsigned int* width, unsigned int* height)
{
    const char* input_file  = filename != NULL ? filename : "lena.png";

    unsigned error;
    unsigned char* image = NULL;

    error = lodepng_decode_file(&image, width, height, input_file, LCT_GREY, 8);
    if (error) 
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        printf("usage: ./readimg input_filename [output_filename]\n");
        return NULL;
    }
    return image;
}

void write_image(char* filename, unsigned char* image, unsigned int width, unsigned int height)
{
    const char* output_file = filename != NULL ? filename : "output.png";

    lodepng_encode_file(output_file, image, width, height, LCT_GREY, 8);
    free(image);
}