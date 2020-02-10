__kernel void add_matrix(const __global int* m1, const __global int* m2, __global int* result)
{
    int i = get_global_id(0);

    result[i] = m1[i] + m2[i];
}

