// Single thread application
#include <Windows.h>
#include <stdio.h>		// Para poder usar el printf
#include <stdint.h>		// Enteros de 32 bits
#include <inttypes.h>
#include <intrin.h>
#include <immintrin.h>
#include <cstdlib>		// Para los random

// Include required header files
#define NTIMES            200   // Number of repetitions to get suitable times
#define SIZE      (1024*1024)   // Number of elements in the array
#define REPETICIONES       10	// Numero de veces que se tomaran tiempos para poder hacer la media



// Metodos
double medirTiempos();
void crearVectores();
void liberarVectores();
void hacerCalculos();
int funcionCountPos();
void funcionSub2();
void funcionMult(int);
void funcionAnd();

// Variables globales
float *vectorU;
float *vectorW;
float *vectorT;
float *vectorAux;
float *vectorS;

// Variables para medir tiempos
LARGE_INTEGER frequency;
LARGE_INTEGER tStart;
LARGE_INTEGER tEnd;
double dElapsedTimeS;

int main() {
	printf("Aplicacion singlethread con SIMD.\r\n");
	// Reserva el espacio de memoria
	crearVectores();
	for (int i = 0; i < REPETICIONES; i++) {
		printf("El tiempo en la iteracion %i es: %f segundos.\r\n", i + 1, medirTiempos());
	}
	// Liberar el espacio de memoria
	liberarVectores();
	printf("Enter para finalizar.\r\n");
	getchar();
}

double medirTiempos() {

	// Get clock frequency in Hz
	QueryPerformanceFrequency(&frequency);
	// Get initial clock count
	QueryPerformanceCounter(&tStart);
	// Calculos
	for (int i = 0; i < NTIMES; i++) {
		hacerCalculos();
	}
	// Get final clock count
	QueryPerformanceCounter(&tEnd);
	// Compute the elapsed time in seconds
	dElapsedTimeS = (tEnd.QuadPart - tStart.QuadPart) / (double)frequency.QuadPart;
	// Return the elapsed time if it'll util
	return dElapsedTimeS;
}

void crearVectores() {
	vectorU = (float *)_aligned_malloc(SIZE * sizeof(float), sizeof(__m256));
	vectorW = (float *)_aligned_malloc(SIZE * sizeof(float), sizeof(__m128));
	vectorT = (float *)_aligned_malloc(SIZE * sizeof(float), sizeof(__m256));
	vectorS = (float *)_aligned_malloc(SIZE * sizeof(float), sizeof(__m256));

	for (int i = 0; i < SIZE; i++) {
		vectorU[i] = (float)(-1.0 + (2.0 * rand()) / RAND_MAX);
		vectorW[i] = (float)(-1.0 + (2.0 * rand()) / RAND_MAX);
		vectorT[i] = (float)(-1.0 + (2.0 * rand()) / RAND_MAX);
	}
}

void liberarVectores() {
	_aligned_free(vectorU);
	_aligned_free(vectorW);
	_aligned_free(vectorT);
	_aligned_free(vectorAux);
	_aligned_free(vectorS);
}

void hacerCalculos() {
	int contador = funcionCountPos();
	funcionSub2();
	funcionMult(contador);
	funcionAnd();
}

int funcionCountPos() {

	float mask = 0.0;
	__m256 mask2 = _mm256_set1_ps(mask);

	int *tmp = (int*)vectorS;
	// Creas 2 punteros que apuntan a los vectores W y S
	__m256 *vectorW256 = (__m256*)vectorW;
	__m256 *vectorS256 = (__m256*)vectorS;
	// Mete en el vector S256 si los enteros del vectorW son positivos o negativos (mete 0 ó 1)
	for (int i = 0; i < SIZE / 8; i++) {
		//elegimos la función cmp con la extension _CMP_EQ_OQ para que busque reales mayores o iguales a cero
		vectorS256[i] = _mm256_cmp_ps(vectorW256[i], mask2, _CMP_GT_OQ);
	}
	//la comparación en caso de hacerse bien devuelve nan o si esta mal devuelve 0, esto se ve al depurar
	//dst[i+31:i] := ( a[i+31:i] OP b[i+31:i] ) ? 0xFFFFFFFF : 0
	int contador = 0;
	for (int i = 0; i < SIZE; i++) {
		if (tmp[i] != 0)//buscamos todos lo números distintos a 0
			contador++;
	}
	return contador;
}

void funcionSub2() {

	// Punteros que apuntan al inicio de los vectores T y S
	__m256 *vectorT256 = (__m256*)(vectorT);
	__m256 *vectorTAux256 = (__m256*)(vectorT + 1);
	__m256 *vectorS256 = (__m256*)vectorS;

	// Para dividir entre 2 todos los paquetes
	// sub2(X): R(i)=(U(i+1)-U(i))/2
	__m256 k256 = _mm256_set1_ps(0.5);

	for (int i = 0; i < SIZE / 8 - 1; i++) {
		// Hace la resta
		vectorS256[i] = _mm256_sub_ps(vectorT256[i], vectorTAux256[i]);
		// Hace la division
		vectorS256[i] = _mm256_mul_ps(vectorS256[i], k256);
	}

	// Como el ultimo paquete no lo tratas, hay que hacerlo a mano
	vectorS[SIZE - 1] = 0.0;
	vectorS[SIZE - 2] = (float)((vectorS[SIZE - 1] - vectorS[SIZE - 2])*0.5);
	vectorS[SIZE - 3] = (float)((vectorS[SIZE - 2] - vectorS[SIZE - 3])*0.5);
	vectorS[SIZE - 4] = (float)((vectorS[SIZE - 3] - vectorS[SIZE - 4])*0.5);
	vectorS[SIZE - 5] = (float)((vectorS[SIZE - 4] - vectorS[SIZE - 5])*0.5);
	vectorS[SIZE - 6] = (float)((vectorS[SIZE - 5] - vectorS[SIZE - 6])*0.5);
	vectorS[SIZE - 7] = (float)((vectorS[SIZE - 6] - vectorS[SIZE - 7])*0.5);
	vectorS[SIZE - 8] = (float)((vectorS[SIZE - 7] - vectorS[SIZE - 8])*0.5);
}

//multiplicamos el contador de positivos con el vector resultante de sub2
void funcionMult(int contador) {

	__m256 count256 = _mm256_set1_ps(contador);//metemos el contador como un paquete de 256 bits
	__m256 *mul256 = (__m256*)vectorS;

	for (int i = 0; i < SIZE / 8; i++)
		mul256[i] = _mm256_mul_ps(mul256[i], count256);//multiplicamos el vector por el contador
}

//del resultado de la multiplicación usamos la función and con otro vector con números aleatorios.
void funcionAnd() {
	__m256 *vectorU256 = (__m256*)vectorU;
	__m256 *vectorS256 = (__m256*)vectorS;

	for (int i = 0; i < SIZE / 8; i++)
		vectorS256[i] = _mm256_and_ps(vectorU256[i], vectorS256[i]);

}