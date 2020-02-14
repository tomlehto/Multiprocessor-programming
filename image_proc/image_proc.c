/* Tomi Lehto 2508005, Julius Rintamäki 2507255
 * OpenCL implementation for 5x5 box blur (average) of images*/

#define MAX_SOURCE_SIZE (0x100000)
#define CHECK_OUTPUT(_ret_value) if (_ret_value != 0) printf("\nError %d in line %d\n\n", _ret_value, __LINE__)

#include "../LodePNG/lodepng.h"

#include <stdio.h>
#include <stdlib.h>

#include <CL/cl.h>


void write_image(char* filename, unsigned char* image, unsigned int width, unsigned int height);
unsigned char* read_image(char* filename, unsigned int* width, unsigned int* height);
cl_device_id init_opencl_device(cl_device_type device_type);
void print_device_info(cl_device_id device_id);
cl_kernel create_kernel(char* filename, char* kernel_func_name, cl_context context, cl_device_id device);
void cleanup(cl_command_queue cmd_queue, cl_kernel kernel,
             cl_context context, cl_mem buf1, cl_mem buf2);


int main(int argc, char *argv[])
{
    unsigned char* image_in = NULL;
    unsigned char* image_out = NULL;
    unsigned int image_width = 0, image_height = 0;
    cl_device_id device = NULL;
    size_t global_work_size, local_work_size, image_buffer_size;
    cl_context context = NULL;
    cl_kernel kernel;
    cl_int cl_error = CL_SUCCESS;
    cl_command_queue cmd_queue = NULL;
    cl_mem input_buffer_cl, output_buffer_cl;
    
    image_in = read_image(NULL, &image_width, &image_height);
    if (image_in == NULL)
    {
        printf("Failure in reading image!\n");
        return 1;
    }
    image_buffer_size = sizeof(unsigned char) * image_width * image_height;

    /*-----------OpenCL part---------------------------------------------*/
    /* Initialize device */
    device = init_opencl_device(CL_DEVICE_TYPE_DEFAULT);
    global_work_size = image_width * image_height;
    local_work_size = 32;

    /* Create OpenCL context */
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &cl_error);
    CHECK_OUTPUT(cl_error);

    /* Create command queue */
    cmd_queue = clCreateCommandQueue(context, device, 0, &cl_error);
    CHECK_OUTPUT(cl_error);

    /* Allocate and initialize device memory for input and output matrices */
    input_buffer_cl  = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image_buffer_size, image_in, &cl_error);
    CHECK_OUTPUT(cl_error);
    output_buffer_cl = clCreateBuffer(context, CL_MEM_WRITE_ONLY, image_buffer_size, NULL, &cl_error);
    CHECK_OUTPUT(cl_error);

    /* Create kernel */
    kernel = create_kernel("image_proc.cl", "image_proc", context, device);

    /* Set kernel arguments */
    cl_error = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&input_buffer_cl);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&output_buffer_cl);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 2, sizeof(unsigned int), (void*)&image_width);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 3, sizeof(unsigned int), (void*)&image_height);
    CHECK_OUTPUT(cl_error);


    /* Launch kernel */
    cl_error = clEnqueueNDRangeKernel(cmd_queue, kernel, CL_TRUE, NULL, &global_work_size, &local_work_size, 0, NULL, NULL);
    CHECK_OUTPUT(cl_error);

    /* Copy the output from device memory back to host memory */
    image_out = (unsigned char*) malloc(image_buffer_size);
    cl_error = clEnqueueReadBuffer(cmd_queue, output_buffer_cl, CL_TRUE, 0, image_buffer_size, image_out, 0, NULL, NULL);
    CHECK_OUTPUT(cl_error);
    /* Cleanup */
    cleanup(cmd_queue, kernel, context, input_buffer_cl, output_buffer_cl);
    /*-------------------------------------------------------------------*/

    write_image(NULL, image_out, image_width, image_height);

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


cl_device_id init_opencl_device(cl_device_type device_type)
{
    cl_platform_id platform = NULL;
    cl_device_id device = NULL;
    cl_int error;

    error = clGetPlatformIDs(1, &platform, NULL);
    CHECK_OUTPUT(error);

    error = clGetDeviceIDs(platform, device_type, 1, &device, NULL);
    CHECK_OUTPUT(error);
    print_device_info(device);

    return device;
}

void print_device_info(cl_device_id device_id)
{
    char* value;
    size_t valueSize;
    cl_uint maxComputeUnits;
    size_t max_wg_size;

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

    //print max workgroup size and return it
    clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE,
            sizeof(size_t), &max_wg_size, NULL);
    printf(" -Maximum workgroup size: %ld\n", max_wg_size);

}

cl_kernel create_kernel(char* filename, char* kernel_func_name, cl_context context, cl_device_id device)
{
    FILE *fp;
    char *source_str;
    size_t source_size;
    
    cl_program program = NULL;
    cl_kernel kernel = NULL;
    cl_int cl_error = CL_SUCCESS;
    
    /* Load kernel source code */
    fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*)malloc(MAX_SOURCE_SIZE);
    source_size = fread(source_str, 1, MAX_SOURCE_SIZE, fp);
    fclose(fp);


    /* Create kernel program from the source */
    program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &cl_error);
    CHECK_OUTPUT(cl_error);

    /* Build kernel program */
    cl_error = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    CHECK_OUTPUT(cl_error);

    kernel = clCreateKernel(program, kernel_func_name, &cl_error);
    CHECK_OUTPUT(cl_error);
    
    //clReleaseProgram(program)
    
    return kernel;
}

void cleanup(cl_command_queue cmd_queue, cl_kernel kernel,
             cl_context context, cl_mem buf1, cl_mem buf2)
{
    clFlush(cmd_queue);
    clFinish(cmd_queue);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(cmd_queue);
    clReleaseContext(context);
    clReleaseMemObject(buf1);
    clReleaseMemObject(buf2);
}