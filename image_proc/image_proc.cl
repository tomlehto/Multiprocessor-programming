__kernel void image_proc(__global unsigned char* input, __global unsigned char* output, unsigned int width,unsigned int height)
{
    int x = get_global_id(0);
    int y = get_global_id(1);
    int tmp;
    if (x % width < 2 || x % width >= width-2 || x < 2*width || x > width * height - 2 * width)
    {
        output[x]  = input[x];
    }
    else
    {
        tmp = 0;
        for (int i = -2; i < 3; i++)
        {
            for (int j = -2; j < 3; j++)
            {
                tmp += input[(y+i)*width + x+j];
            }
        }
        //output[y*width + x]  = input[y*width + x];
        output[y*width + x] = tmp * 0.04;
        //output[y*width + x] = 255;
    }
}