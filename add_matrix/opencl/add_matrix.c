/*Tomi Lehto 2508005, Julius Rintamäki 2507255
 *Plain C implementation for addition of two 100x100 matrices */

#define M_ROWS 10
#define M_COLS 10
#define MAX_SOURCE_SIZE (0x100000)

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <CL/cl.h>

int compare_matrices(int* m1, int* m2);
void add_matrices(int* m1, int* m2, int* result);
void generate_test_matrix(int* result);
void print_matrix(int* m);

int main()
{
	int clerror = CL_SUCCESS;
	cl_program program = NULL;
	cl_int ret;
	cl_mem m1_cl;
	cl_mem m2_cl;
	cl_mem m3_cl;
	FILE *fp;
	char file_name[] = "./add_matrix.cl";
	char *source_str;
	size_t source_size;
	
	/* Load kernel source code */
	fp = fopen(file_name, "r");
	if (!fp) {
		fprintf(stderr, "Failed to load kernel.\n");
		exit(1);
	}
	source_str = (char*)malloc(MAX_SOURCE_SIZE);
	source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
	fclose(fp);
	
	int* m1 = malloc(M_ROWS * M_COLS * sizeof(*m1));
	int* m2 = malloc(M_ROWS * M_COLS * sizeof(*m2));
	int* m3 = malloc(M_ROWS * M_COLS * sizeof(*m3));
	int* m3_c = malloc(M_ROWS * M_COLS * sizeof(*m3_c));
	
	generate_test_matrix(m1);
	generate_test_matrix(m2);

	// Query platform ID
	cl_platform_id platform;
	ret = clGetPlatformIDs (1, &platform, NULL);
	printf("%d\n", ret);

	// Setup context properties
	cl_context_properties props[3];
	props[0] = (cl_context_properties)CL_CONTEXT_PLATFORM;
	props[1] = (cl_context_properties)platform;
	props[2] = (cl_context_properties)0;

	// Create a context to run OpenCL on our CUDA-enabled NVIDIA GPU
	cl_context CPUContext = clCreateContextFromType(props, CL_DEVICE_TYPE_CPU,NULL, NULL, &ret);
	printf("%d\n", ret);

	// Get the list of GPU devices associated with this context
	size_t ParmDataBytes;
	clGetContextInfo(CPUContext, CL_CONTEXT_DEVICES, 0, NULL, &ParmDataBytes);
	cl_device_id* CPUDevices = (cl_device_id*)malloc(ParmDataBytes);
	clGetContextInfo(CPUContext, CL_CONTEXT_DEVICES, ParmDataBytes, CPUDevices, NULL);
	printf("%d\n", ret);

	// Create a command-queue on the first GPU device
	cl_command_queue CPUCommandQueue = clCreateCommandQueue(CPUContext, CPUDevices[0], 0, NULL);

	// Allocate GPU memory for source vectors AND initialize from CPU memory
	m1_cl = clCreateBuffer(CPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * M_ROWS * M_COLS, m1, &ret);
	printf("%d\n", ret);
	m2_cl = clCreateBuffer(CPUContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * M_ROWS * M_COLS, m1, &ret);
	printf("%d\n", ret);
	m3_cl = clCreateBuffer(CPUContext, CL_MEM_WRITE_ONLY, sizeof(int) * M_ROWS * M_COLS, NULL, &ret);
	printf("%d\n", ret);

	/* Create Kernel Program from the source */
	program = clCreateProgramWithSource(CPUContext, 1,(const char **)&source_str, (const size_t *)&source_size, &ret);
	printf("%d\n", ret);
	 
	/* Build Kernel Program */
	ret = clBuildProgram(program, 1, CPUDevices, NULL, NULL, NULL);
	printf("%d\n", ret);

	cl_kernel add_matrix_kernel = clCreateKernel(program, "add_matrix", &ret);
	printf("%d\n", ret);

	size_t global_work_size;
	size_t local_work_size;

	global_work_size = 100;
	local_work_size = 25;

	// In the next step we associate the GPU memory with the Kernel arguments
	ret = clSetKernelArg(add_matrix_kernel, 0, sizeof(cl_mem), (void*)&m1_cl);
	printf("%d\n", ret);
	ret = clSetKernelArg(add_matrix_kernel, 1, sizeof(cl_mem), (void*)&m2_cl);
	printf("%d\n", ret);
	ret = clSetKernelArg(add_matrix_kernel, 2, sizeof(cl_mem), (void*)&m3_cl);
	printf("%d\n", ret);

	// Launch the Kernel on the GPU
	ret = clEnqueueNDRangeKernel(CPUCommandQueue, add_matrix_kernel, 1, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
	printf("%d\n", ret);

	// Copy the output in GPU memory back to CPU memory
	ret = clEnqueueReadBuffer(CPUCommandQueue, m3_cl, CL_TRUE, 0, M_COLS * M_ROWS * sizeof(int), m3, 0, NULL, NULL);
	printf("%d\n", ret);

	print_matrix(m3);
	// Verify results against plain C implementation
	add_matrices(m1,m2,m3_c);
	
	if (compare_matrices(m3, m3_c))
	{
		printf("Matrices are not equal\n");
	}
	else
	{
		printf("Matrices are equal\n");
	}
	print_matrix(m3_c);
	
	free(m1);
	free(m2);
	free(m3);
	free(m3_c);
	
	free(CPUDevices);
	clReleaseKernel(add_matrix_kernel);
	clReleaseProgram(program);
	clReleaseCommandQueue(CPUCommandQueue);
	clReleaseContext(CPUContext);
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
			result[i*M_COLS + j] = 1 /*rand()*/;
		}
	}
}
