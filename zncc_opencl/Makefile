CC = cc

zncc_opencl: zncc_opencl.c zncc_opencl.cl
	$(CC) zncc_opencl.c ../LodePNG/lodepng.c -lOpenCL -o zncc_opencl -Wall

clean:
	rm zncc_opencl
