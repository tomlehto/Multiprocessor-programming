__kernel void lpf(__global unsigned char* image_in, unsigned int w, unsigned int h, __global unsigned char* image_out)
{
    const int j = get_global_id(0);

    for (int i = 0; i < w; i++)
    {
        /* Copy first pixel of row */
        if (i == 0)
        {
            image_out[j*w + i] = image_in[j*w + i];
        }
        else
        {
            image_out[j*w + i] = (image_in[j*w + i] + image_in[j*w + i - 1]) / 2;
        }
    }
}