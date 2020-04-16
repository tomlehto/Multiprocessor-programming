__kernel void lpf(__global unsigned char* image_in, unsigned int w, __global unsigned char* image_out)
{
    __private unsigned char cache[WIDTH]; //WIDTH must be specified with in kernel compile arguments
    const int j = get_global_id(0);

    /* Prefetch row to worker-private cache */
    for(int i = 0; i < w; i++)
    {
        cache[i] = image_in[j*w + i];
    }

    /* Copy first pixel of row */
    image_out[j*w] = cache[0];

    /* Calculate and write rest of pixels with simple LPF */
    for (int i = 1; i < w; i++)
    {
        image_out[j*w + i] = (cache[i] + cache[i - 1]) / 2;
    }

}