// Single thread application
#include <Windows.h>
#include <stdio.h>		// Para poder usar el printf

// Include required header files
#define NTIMES            200   // Number of repetitions to get suitable times
#define SIZE      (1024*1024)   // Number of elements in the array
#define REPETICIONES       10	// Numero de veces que se tomaran tiempos para poder hacer la media

// Metodos
double medirTiempos();
void crearVectores();
void liberarVectores();
void hacerCalculos();

// Variables globales
float* vectorU;
float* vectorW;
float* vectorT;
float* vectorRes;
float* vectorAux1;
float* vectorAux2;

// Variables para medir tiempos
LARGE_INTEGER frequency;
LARGE_INTEGER tStart;
LARGE_INTEGER tEnd;
double dElapsedTimeS;

int main() {
	printf("Aplicacion singlethread.\r\n");
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
	vectorU = (float*)malloc(SIZE * sizeof(float));
	vectorW = (float*)malloc(SIZE * sizeof(float));
	vectorT = (float*)malloc(SIZE * sizeof(float));
	vectorRes = (float*)malloc(SIZE * sizeof(float));
	vectorAux1 = (float*)malloc(SIZE * sizeof(float));
	vectorAux2 = (float*)malloc(SIZE * sizeof(float));
	for (int i = 0; i < SIZE; i++) {
		vectorU[i] = (float)(-1.0 + (2.0 * rand()) / RAND_MAX);
		vectorW[i] = (float)(-1.0 + (2.0 * rand()) / RAND_MAX);
		vectorT[i] = (float)(-1.0 + (2.0 * rand()) / RAND_MAX);
	}
}

void liberarVectores() {
	free(vectorU);
	free(vectorW);
	free(vectorT);
	free(vectorRes);
	free(vectorAux1);
	free(vectorAux2);
}

void hacerCalculos() {
	// Antiguo: [countpos(W) * blurr4(U)] AND T
	// Nuevo: [countpos(W) * sub2(U)] AND T
	//
	// counpos(X): cantidad de numeros positivos en X
	// sub2(X): R(i)=(U(i+1)-U(i))/2
	// X AND Y.

	// countpos == K
	int resOp1 = 0;
	for (int i = 0; i < SIZE; i++) {
		if (vectorW[i] > 0) {
			resOp1++;
		}
	}


	// sub2 == G
	for (int i = 0; i < SIZE; i++) {
		if (i == SIZE - 1) {
			vectorAux1[i] = -vectorU[i] / 2;
		}
		else {
			vectorAux1[i] = (vectorU[i + 1] - vectorU[i]) / 2;
		}
	}

	// [K * G] == M
	for (int i = 0; i < SIZE; i++) {
		vectorAux2[i] = resOp1*vectorAux1[i];
	}

	// M AND T == res
	int *tmp1 = (int*)vectorAux1;
	int *tmp2 = (int*)vectorT;
	int res;
	for (int i = 0; i < SIZE; i++) {
		res = tmp1[i] & tmp2[i];
		vectorRes[i] = *(float*)&res;
	}
}