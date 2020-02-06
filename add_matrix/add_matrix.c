/*Tomi Lehto 2508005, Julius Rintamäki 2507255
 *OpenCL + Plain C implementation for addition of two 100x100 matrices */

#define M_ROWS 100
#define M_COLS 100
#define MAX_SOURCE_SIZE (0x100000)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <CL/cl.h>

void print_matrix(int* m);
int compare_matrices(int* m1, int* m2);
void add_matrices(int* m1, int* m2, int* result);
void generate_test_matrix(int* result);
void print_device_info(cl_device_id device_id);

int main()
{
    FILE *fp;
    char file_name[] = "./add_matrix.cl";
    char *source_str;
    size_t source_size;
    cl_mem m1_cl;
    cl_mem m2_cl;
    cl_mem m3_cl;
    cl_int cl_error = CL_SUCCESS;
    cl_platform_id platform = NULL;
    cl_device_id device = NULL;
    cl_context context = NULL;
    cl_command_queue cmd_queue = NULL;
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    size_t global_work_size = M_ROWS * M_COLS;
    size_t local_work_size = 4;
    clock_t start, end;

    /* Load kernel source code */
    fp = fopen(file_name, "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);

    /* Allocate memory for input and output matrices */
    int* m1 = malloc(M_ROWS * M_COLS * sizeof(*m1));
    int* m2 = malloc(M_ROWS * M_COLS * sizeof(*m2));
    int* m3 = malloc(M_ROWS * M_COLS * sizeof(*m3));
    int* m3_c = malloc(M_ROWS * M_COLS * sizeof(*m3_c));

    /* Generate random test matrices */	
    generate_test_matrix(m1);
    generate_test_matrix(m2);

    cl_error = clGetPlatformIDs (1, &platform, NULL);

    cl_error = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL);

    print_device_info(device);

    /* Create OpenCL context */
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &cl_error);

    cmd_queue = clCreateCommandQueue(context, device, 0, NULL);

    /* Allocate and initialize GPU memory for input and output matrices */
    m1_cl = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * M_ROWS * M_COLS, m1, &cl_error);
    m2_cl = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * M_ROWS * M_COLS, m1, &cl_error);
    m3_cl = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * M_ROWS * M_COLS, NULL, &cl_error);

    /* Create kernel program from the source */
    program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &cl_error);
        
    /* Build kernel program */
    cl_error = clBuildProgram(program, 1, &device, NULL, NULL, NULL);

    kernel = clCreateKernel(program, "add_matrix", &cl_error);

    /* Set kernel arguments */
    cl_error = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&m1_cl);
    cl_error = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&m2_cl);
    cl_error = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void*)&m3_cl);

    size_t result;
    size_t size_ret;
    cl_error = clGetKernelWorkGroupInfo(kernel, NULL,  CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), (void*)&result, &size_ret);

    /* Launch kernel */
    start = clock();
    cl_error = clEnqueueNDRangeKernel(cmd_queue, kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    end = clock();
    printf("Execution time for OpenCL: %ld us\n", (end-start) / (CLOCKS_PER_SEC / 1000000));

    // Copy the output in GPU memory back to CPU memory
    cl_error = clEnqueueReadBuffer(cmd_queue, m3_cl, CL_TRUE, 0, M_COLS * M_ROWS * sizeof(int), m3, 0, NULL, NULL);

    //print_matrix(m3);
    // Verify results against plain C implementation
    start = clock();
    add_matrices(m1,m2,m3_c);
    end = clock();
    printf("Execution time for plain C: %ld us\n", (end-start) / (CLOCKS_PER_SEC / 1000000));

    if (compare_matrices(m3, m3_c))
    {
        printf("Matrices are not equal\n");
    }
    else
    {
        printf("OpenCL implementation produced same result as plain C implementation!\n");
    }
    //print_matrix(m3_c);

    free(m1);
    free(m2);
    free(m3);
    free(m3_c);

    clFlush(cmd_queue);
    clFinish(cmd_queue);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(cmd_queue);
    clReleaseContext(context);
    clReleaseMemObject(m1_cl);
    clReleaseMemObject(m2_cl);
    clReleaseMemObject(m3_cl);
    return 0;

}

void print_matrix(int* m)
{
	for (int i = 0; i < M_ROWS; i++)
	{
		for (int j = 0; j < M_COLS; j++)
		{
			printf("%d ", m[i*M_COLS + j]);
		}
		printf("\n");
	}
}

int compare_matrices(int* m1, int* m2)
{
	for (int i = 0; i < M_ROWS; i++)
	{
		for (int j = 0; j < M_COLS; j++)
		{
			if (m1[i*M_COLS + j] != m2[i*M_COLS + j])
			{
				return 1;
			}
		}
	}
	return 0;
}

void add_matrices(int* m1, int* m2, int* result)
{
	for (int i = 0; i < M_ROWS; i++)
	{
		for (int j = 0; j < M_COLS; j++)
		{
			result[i*M_COLS + j] = m1[i*M_COLS + j] + m2[i*M_COLS + j];
		}

	}
}

void generate_test_matrix(int* result)
{
	srand(time(NULL));
	for (int i = 0; i < M_ROWS; i++)
	{
		for (int j = 0; j < M_COLS; j++)
		{
			result[i*M_COLS + j] = rand();
		}
	}
}

void print_device_info(cl_device_id device_id)
{
    char* value;
    size_t valueSize;
    cl_uint maxComputeUnits;

    // print device name
    clGetDeviceInfo(device_id, CL_DEVICE_NAME, 0, NULL, &valueSize);
    value = (char*) malloc(valueSize);
    clGetDeviceInfo(device_id, CL_DEVICE_NAME, valueSize, value, NULL);
    printf("Device: %s\n", value);
    free(value);

    // print hardware device version
    clGetDeviceInfo(device_id, CL_DEVICE_VERSION, 0, NULL, &valueSize);
    value = (char*) malloc(valueSize);
    clGetDeviceInfo(device_id, CL_DEVICE_VERSION, valueSize, value, NULL);
    printf(" -Hardware version: %s\n", value);
    free(value);

    // print software driver version
    clGetDeviceInfo(device_id, CL_DRIVER_VERSION, 0, NULL, &valueSize);
    value = (char*) malloc(valueSize);
    clGetDeviceInfo(device_id, CL_DRIVER_VERSION, valueSize, value, NULL);
    printf(" -Software version: %s\n", value);
    free(value);

    // print c version supported by compiler for device
    clGetDeviceInfo(device_id, CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &valueSize);
    value = (char*) malloc(valueSize);
    clGetDeviceInfo(device_id, CL_DEVICE_OPENCL_C_VERSION, valueSize, value, NULL);
    printf(" -OpenCL C version: %s\n", value);
    free(value);

    // print parallel compute units
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_COMPUTE_UNITS,
            sizeof(maxComputeUnits), &maxComputeUnits, NULL);
    printf(" -Parallel compute units: %d\n", maxComputeUnits);

}