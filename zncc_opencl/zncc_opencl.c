/* Tomi Lehto 2508005, Julius Rintamäki 2507255 */

#define MAX_DISP 64
#define MIN_DISP 0
//#define WIN_SIZE 5
#define WIN_W 20
#define WIN_H 14
#define THRESHOLD 12

#define MAX_SOURCE_SIZE (0x100000)
#define CHECK_OUTPUT(_ret_value) if (_ret_value != 0) printf("\nError %d in line %d\n\n", _ret_value, __LINE__)

#include "../LodePNG/lodepng.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <CL/cl.h>

void zncc(unsigned char* il, unsigned char* ir, unsigned int w, unsigned int h, int d_max, int d_min, unsigned char* d_map);
void post_processing(unsigned char* disparity_l2r, unsigned char* disparity_r2l, unsigned char* result, int img_size);

cl_device_id init_opencl_device(cl_device_type device_type);
void print_device_info(cl_device_id device_id);
cl_kernel create_kernel(char* filename, char* kernel_func_name, cl_context context, cl_device_id device);
void cleanup(cl_command_queue cmd_queue, cl_kernel kernel,
             cl_context context, cl_mem buf1, cl_mem buf2,
             cl_mem buf3, cl_mem buf4);

int main(int argc, char *argv[])
{
    
    const char* input_file_left  = /*argc > 1 ? argv[1] :*/ "imageL.png";
    const char* input_file_right  = /*argc > 2 ? argv[2] :*/ "imageR.png";
    const char* output_file = /*argc > 3 ? argv[3] :*/ "output.png";

    unsigned error;
    /* Left image */
    unsigned char* il = 0;
    /* Left image */
    unsigned char* ir = 0;
    /* width, height */
    unsigned int w, h;
    int d1, d2;
    unsigned char* disparity_l2r;
    unsigned char* disparity_r2l;
    unsigned char* final_result;

    /* OpenCL related declarations */
    cl_device_id device = NULL;
    size_t global_work_size[2];
    size_t global_work_size_lpf;
    size_t image_buffer_size;
    cl_context context = NULL;
    cl_kernel kernel, kernel_lpf;
    cl_int cl_error = CL_SUCCESS;
    cl_command_queue cmd_queue = NULL;
    cl_mem input_buffer_left_cl, input_buffer_right_cl, input_buffer_lpf_cl;
    cl_mem output_buffer_l2r_cl, output_buffer_r2l_cl, output_buffer_lpf_cl;
    cl_event event_1;
    cl_event event_2;
    cl_ulong start;
    cl_ulong end;
    double execution_time_ms = 0;

    error = lodepng_decode_file(&il, &w, &h, input_file_left, LCT_GREY, 8);
    if (error) 
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        printf("usage: ./zncc_c input_filename1 input_filename2 [output_filename]\n");
        return 0;
    }
    error = lodepng_decode_file(&ir, &w, &h, input_file_right, LCT_GREY, 8);
    if (error) 
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        printf("usage: ./zncc_c input_filename1 input_filename2 [output_filename]\n");
        return 0;
    }

    image_buffer_size = sizeof(unsigned char) * w * h;

    disparity_l2r = calloc(w * h, sizeof(unsigned char));
    disparity_r2l = calloc(w * h, sizeof(unsigned char));
    final_result = calloc(w * h, sizeof(unsigned char));

    /*-----------OpenCL part---------------------------------------------*/
    /* Initialize device */
    device = init_opencl_device(CL_DEVICE_TYPE_DEFAULT);
    global_work_size[0] = h;
    global_work_size[1] = w;

    /* Create OpenCL context */
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &cl_error);
    CHECK_OUTPUT(cl_error);

    /* Create command queue with profiling enabled*/
    cmd_queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &cl_error);
    CHECK_OUTPUT(cl_error);

    /* Allocate and initialize device memory for input and output matrices */
    input_buffer_left_cl  = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image_buffer_size, il, &cl_error);
    CHECK_OUTPUT(cl_error);
    input_buffer_right_cl  = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image_buffer_size, ir, &cl_error);
    CHECK_OUTPUT(cl_error);
    output_buffer_l2r_cl = clCreateBuffer(context, CL_MEM_WRITE_ONLY, image_buffer_size, NULL, &cl_error);
    CHECK_OUTPUT(cl_error);
    output_buffer_r2l_cl = clCreateBuffer(context, CL_MEM_WRITE_ONLY, image_buffer_size, NULL, &cl_error);
    CHECK_OUTPUT(cl_error);

    /* Create kernel */
    kernel = create_kernel("zncc_opencl.cl", "zncc", context, device);

    /* Left to right */
    /* zncc(il, ir, w, h, MIN_DISP, MAX_DISP, disparity_l2r); */
    /* Set kernel arguments */
    d1 = MIN_DISP;
    d2 = MAX_DISP;
    cl_error = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&input_buffer_left_cl);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&input_buffer_right_cl);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 2, sizeof(unsigned int), (void*)&w);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 3, sizeof(unsigned int), (void*)&h);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 4, sizeof(int), (void*)&d1);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 5, sizeof(int), (void*)&d2);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void*)&output_buffer_l2r_cl);
    CHECK_OUTPUT(cl_error);

    /* Launch kernel and link to event*/
    cl_error = clEnqueueNDRangeKernel(cmd_queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, &event_1);
    CHECK_OUTPUT(cl_error);

    /* Right to left */
    /* zncc(ir, il, w, h, -MAX_DISP, MIN_DISP, disparity_r2l); */

    /* Set kernel arguments */
    d1 = -MAX_DISP;
    d2 = MIN_DISP;
    cl_error = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void*)&input_buffer_right_cl);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void*)&input_buffer_left_cl);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 2, sizeof(unsigned int), (void*)&w);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 3, sizeof(unsigned int), (void*)&h);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 4, sizeof(int), (void*)&d1);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 5, sizeof(int), (void*)&d2);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel, 6, sizeof(cl_mem), (void*)&output_buffer_r2l_cl);
    CHECK_OUTPUT(cl_error);

    /* Launch kernel and link to event*/
    cl_error = clEnqueueNDRangeKernel(cmd_queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, &event_2);
    CHECK_OUTPUT(cl_error);

    /* Wait for kernel to finish */
    clFinish(cmd_queue);

    /* Copy the output from device memory back to host memory */
    cl_error = clEnqueueReadBuffer(cmd_queue, output_buffer_l2r_cl, CL_TRUE, 0, image_buffer_size, disparity_l2r, 0, NULL, NULL);
    CHECK_OUTPUT(cl_error);
    cl_error = clEnqueueReadBuffer(cmd_queue, output_buffer_r2l_cl, CL_TRUE, 0, image_buffer_size, disparity_r2l, 0, NULL, NULL);
    CHECK_OUTPUT(cl_error);

    /* Execution time measurement */
    cl_error = clGetEventProfilingInfo(event_1, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
    CHECK_OUTPUT(cl_error);
    cl_error = clGetEventProfilingInfo(event_1, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);
    CHECK_OUTPUT(cl_error);
    execution_time_ms += end - start;
    cl_error = clGetEventProfilingInfo(event_2, CL_PROFILING_COMMAND_START, sizeof(start), &start, NULL);
    CHECK_OUTPUT(cl_error);
    cl_error = clGetEventProfilingInfo(event_2, CL_PROFILING_COMMAND_END, sizeof(end), &end, NULL);
    CHECK_OUTPUT(cl_error);
    execution_time_ms += end - start;
    printf("Total ZNCC kernel execution time is: %0.3f milliseconds \n", execution_time_ms / 1000000.0);

    lodepng_encode_file("l2r.png", disparity_l2r, w, h, LCT_GREY, 8);
    lodepng_encode_file("r2l.png", disparity_r2l, w, h, LCT_GREY, 8);

    /* Post-processing */
    post_processing(disparity_l2r, disparity_r2l, final_result, w*h);

    /* Encode result */
    lodepng_encode_file(output_file, final_result, w, h, LCT_GREY, 8);

    /*------------------------- LPF PART -------------------------*/
    /* Create kernel */
    kernel_lpf = create_kernel("lpf.cl", "lpf", context, device);

    /* Allocate and initialize device memory for input and output */
    input_buffer_lpf_cl  = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, image_buffer_size, final_result, &cl_error);
    CHECK_OUTPUT(cl_error);
    output_buffer_lpf_cl = clCreateBuffer(context, CL_MEM_WRITE_ONLY, image_buffer_size, NULL, &cl_error);
    CHECK_OUTPUT(cl_error);

    /* Set kernel arguments */
    /* __kernel void lpf(__global unsigned char* image_in, unsigned int w, __global unsigned char* image_out) */
    global_work_size_lpf = (size_t)h;
    cl_error = clSetKernelArg(kernel_lpf, 0, sizeof(cl_mem), (void*)&input_buffer_lpf_cl);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel_lpf, 1, sizeof(unsigned int), (void*)&w);
    CHECK_OUTPUT(cl_error);
    cl_error = clSetKernelArg(kernel_lpf, 2, sizeof(cl_mem), (void*)&output_buffer_lpf_cl);
    CHECK_OUTPUT(cl_error);

    /* Launch kernel and link to event*/
    cl_error = clEnqueueNDRangeKernel(cmd_queue, kernel_lpf, 1, NULL, &global_work_size_lpf, NULL, 0, NULL, NULL);
    CHECK_OUTPUT(cl_error);

    /* Wait for kernel to finish */
    clFinish(cmd_queue);

    /* Copy the output from device memory back to host memory */
    cl_error = clEnqueueReadBuffer(cmd_queue, output_buffer_lpf_cl, CL_TRUE, 0, image_buffer_size, final_result, 0, NULL, NULL);
    CHECK_OUTPUT(cl_error);

    /* Encode result */
    lodepng_encode_file("output_lpf.png", final_result, w, h, LCT_GREY, 8);

    /*------------------------------------------------------------*/

    /* Cleanup */
    cleanup(cmd_queue, kernel, context, input_buffer_left_cl, input_buffer_right_cl, output_buffer_l2r_cl, output_buffer_r2l_cl);
    free(disparity_l2r);
    free(disparity_r2l);
    free(final_result);

    return 0;
}

void post_processing(unsigned char* disparity_l2r, unsigned char* disparity_r2l, unsigned char* result, int img_size)
{
    unsigned char max = 0;
    unsigned char min = 255;
    unsigned char nn_color = 0; // color of nearest neighbour

    for (int idx = 0; idx < img_size; idx++)
    {
        if (abs(disparity_l2r[idx] - disparity_r2l[idx]) > THRESHOLD)
        {
            result[idx] = 0;
        }
        else{
            result[idx] = disparity_l2r[idx];
        }
    }

    //Simplest form of nearest neighbour
    for (int idx = 0; idx < img_size; idx++)
    {
        if (result[idx] == 0)
        {
            result[idx] = nn_color;
        }
        else{
            nn_color = result[idx];
        }
    }

    //Normalize
    for (int i = 0; i < img_size; i++)
    {
        if (result[i] > max)
        {
            max = result[i];
        }
        if (result[i] < min)
        {
            min = result[i];
        }
    }
    for (int i = 0; i < img_size; i++)
    {
        result[i] = (unsigned char) (255*(result[i]- min)/(max - min));
    }
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
    
    return kernel;
}

void cleanup(cl_command_queue cmd_queue, cl_kernel kernel,
             cl_context context, cl_mem buf1, cl_mem buf2,
             cl_mem buf3, cl_mem buf4)
{
    clFlush(cmd_queue);
    clFinish(cmd_queue);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(cmd_queue);
    clReleaseContext(context);
    clReleaseMemObject(buf1);
    clReleaseMemObject(buf2);
    clReleaseMemObject(buf3);
    clReleaseMemObject(buf4);
}