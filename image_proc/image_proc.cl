__kernel void image_proc(__global unsigned char* input, __global unsigned char* output)
{
    int i = get_global_id(0);
    output[i]  = input[i];
}
