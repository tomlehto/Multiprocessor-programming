/*Tomi Lehto 2508005, Julius Rintamäki 2507255
 *Plain C implementation for addition of two 100x100 matrices */

#define M_ROWS 100
#define M_COLS 100

#include <stdio.h>
#include <stdlib.h>

void print_matrix(int* m);
void add_matrices(int* m1, int* m2, int* result);
void generate_test_matrix(int* result, int a);

int main()
{
	int* m1 = malloc(M_ROWS * M_COLS * sizeof(*m1));
	int* m2 = malloc(M_ROWS * M_COLS * sizeof(*m2));
	int* m3 = malloc(M_ROWS * M_COLS * sizeof(*m3));
	
	generate_test_matrix(m1, 1);
	generate_test_matrix(m2, 2);
	
	add_matrices(m1,m2,m3);
	print_matrix(m3);
	
	free(m1);
	free(m2);
	free(m3);
	
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

void generate_test_matrix(int* result, int a)
{
	for (int i = 0; i < M_ROWS; i++)
	{
		for (int j = 0; j < M_COLS; j++)
		{
			result[i*M_COLS + j] = a;
		}
	}
}
