// Single thread application
#include <Windows.h>
#include <stdio.h>		// Para poder usar el printf
#include <thread>
#include <inttypes.h>
#include <intrin.h>
#include <immintrin.h>

// Include required header files
#define NTIMES            200   // Number of repetitions to get suitable times
#define SIZE      (1024*1024)   // Number of elements in the array
#define REPETICIONES       10	// Numero de veces que se tomaran tiempos para poder hacer la media

// Estructura
typedef struct {
	unsigned int size;	// Numero de posiciones con las que tendra que calcular
	float* vector1;
	float* vector2;
	float* vector3;
	int entero;			// En funcion sub2: 0=normal, 1=ultimo. En funcion mult: valor del countpos
	int* resultados;	// Array para guardar los resultados de cada hilo en la funcion countpos
	int posResultados;	// Posicion que le corresponde del array resultados
} datosStruct;

// Metodos main
int calcularNumeroThreads();
double medirTiempos(int);
void crearVectores();
void liberarVectores();
void hacerCalculos(int, datosStruct*, HANDLE*);

// Metodos para hacer hilos
int hilosCountPos(int, datosStruct*, HANDLE*);
void hilosSub2(int, datosStruct*, HANDLE*);
void hilosMultiplicacion(int, datosStruct*, HANDLE*, int);
void hilosAnd(int, datosStruct*, HANDLE*);

// Metodos para hacer las funciones
DWORD WINAPI countPos(LPVOID datos);
DWORD WINAPI sub2(LPVOID datos);
DWORD WINAPI multiplicacion(LPVOID datos);
DWORD WINAPI and (LPVOID datos);

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

// ---------------------------------------------------------------- MAIN ---------------------------------------------------------------- \\

int main() {
	printf("Aplicacion multithread con SIMD.\r\n");
	// Calcular numero de hilos
	int numThreads = calcularNumeroThreads();
	printf("Numero de hilos %i\r\n", numThreads);
	// Reserva el espacio de memoria
	crearVectores();
	for (int i = 0; i < REPETICIONES; i++) {
		printf("El tiempo en la iteracion %i es: %f segundos\r\n", i + 1, medirTiempos(numThreads));
	}
	// Liberar el espacio de memoria
	liberarVectores();
	printf("Enter para finalizar.\r\n");
	getchar();
}

int calcularNumeroThreads() {
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
}

double medirTiempos(int numThreads) {
	// Se reserva memoria para guardar un array de datosStruct
	datosStruct* arrayStructs = (datosStruct*)malloc(sizeof(datosStruct)*numThreads);
	// Se reserva memoria para guardar un array de HANDLE
	HANDLE* arrayHilos = (HANDLE*)malloc(sizeof(HANDLE)*numThreads);
	// Get clock frequency in Hz
	QueryPerformanceFrequency(&frequency);
	// Get initial clock count
	QueryPerformanceCounter(&tStart);
	// Calculos
	for (int i = 0; i < NTIMES; i++) {
		hacerCalculos(numThreads, arrayStructs, arrayHilos);
	}
	// Get final clock count
	QueryPerformanceCounter(&tEnd);
	// Compute the elapsed time in seconds
	dElapsedTimeS = (tEnd.QuadPart - tStart.QuadPart) / (double)frequency.QuadPart;
	// Print the elapsed time
	/*printf("Elapsed time in seconds: %f\n", dElapsedTimeS);*/
	// Se libera la memoria reservada
	free(arrayStructs);
	free(arrayHilos);
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

// ---------------------------------------------------------------- CALCULOS ---------------------------------------------------------------- \\

void hacerCalculos(int numThreads, datosStruct* arrayStructs, HANDLE* arrayHilos) {
	// Como las operaciones se realizaran una detras de otra podemos reutilizar el array de structs y el de hilos
	int countPos = hilosCountPos(numThreads, arrayStructs, arrayHilos);
	hilosSub2(numThreads, arrayStructs, arrayHilos);
	hilosMultiplicacion(numThreads, arrayStructs, arrayHilos, countPos);
	hilosAnd(numThreads, arrayStructs, arrayHilos);
}

// ---------------------------------------------------------------- COUNTPOS ---------------------------------------------------------------- \\

int hilosCountPos(int numThreads, datosStruct* arrayStructs, HANDLE* arrayHilos) {
	// Se reserva memoria para un array donde se alojaran los resultados del countpos
	int* arrayResultadosCountPos = (int*)malloc(sizeof(int)*numThreads);
	// Aqui se crean los structs y se llama a la operacion countPos con cada hilo
	for (int i = 0; i < numThreads; i++) {
		arrayStructs[i].size = SIZE / numThreads;
		arrayStructs[i].vector1 = &vectorW[(SIZE / numThreads)*i];
		arrayStructs[i].vector2 = &vectorS[(SIZE / numThreads)*i];
		arrayStructs[i].resultados = arrayResultadosCountPos;
		arrayStructs[i].posResultados = i;
		// Se crea el hilo
		arrayHilos[i] = CreateThread(NULL, 0, countPos, &arrayStructs[i], 0, NULL);
	}

	// Aqui se espera a que todos los hilos acaben
	for (int i = 0; i < numThreads; i++) {
		WaitForSingleObject(arrayHilos[i], INFINITE);
	}

	// Aqui se suman todos los resultados recogidos por cada hilo.
	int resultado = 0;
	for (int i = 0; i < numThreads; i++) {
		resultado += arrayResultadosCountPos[i];
	}
	free(arrayResultadosCountPos);
	return resultado;
}
DWORD WINAPI countPos(LPVOID datos) {
	// Countpos
	datosStruct *structDatos = (datosStruct*)datos;
	int sizeDatos = structDatos->size;
	float* vectorWLocal = structDatos->vector1;
	float* vectorSLocal = structDatos->vector2;
	
	float mask = 0.0;
	__m256 mask2 = _mm256_set1_ps(mask);

	int *tmp = (int*)vectorSLocal;
	__m256 *vectorW256 = (__m256*)vectorWLocal;
	__m256 *vectorS256 = (__m256*)vectorSLocal;
	for (int i = 0; i < sizeDatos / 8; i++) {
		vectorS256[i] = _mm256_cmp_ps(vectorW256[i], mask2, _CMP_GT_OQ);
	}
	int resOp1 = 0;
	for (int i = 0; i < sizeDatos; i++) {
		if (tmp[i] != 0)
			resOp1++;
	}
	structDatos->resultados[structDatos->posResultados] = resOp1;
	return 0;
}

// ---------------------------------------------------------------- SUB2 ---------------------------------------------------------------- \\

void hilosSub2(int numThreads, datosStruct* arrayStructs, HANDLE* arrayHilos) {
	// Aqui se crean los structs y se llama a la operacion countPos con cada hilo
	for (int i = 0; i < numThreads; i++) {
		arrayStructs[i].size = SIZE / numThreads;
		arrayStructs[i].vector1 = &vectorT[(SIZE / numThreads)*i];
		arrayStructs[i].vector2 = &vectorS[(SIZE / numThreads)*i];
		if (i == numThreads - 1) {
			arrayStructs[i].entero = 1;
		}
		else {
			arrayStructs[i].entero = 0;
		}
		// Se crea el hilo
		arrayHilos[i] = CreateThread(NULL, 0, sub2, &arrayStructs[i], 0, NULL);
	}

	// Aqui se espera a que todos los hilos acaben
	for (int i = 0; i < numThreads; i++) {
		WaitForSingleObject(arrayHilos[i], INFINITE);
	}
}
DWORD WINAPI sub2(LPVOID datos) {
	// Sub2
	datosStruct *structDatos = (datosStruct*)datos;
	int sizeDatos = structDatos->size;
	int ultimo = structDatos->entero;
	float* vectorTLocal = structDatos->vector1;
	float* vectorSLocal = structDatos->vector2;

	__m256 *vectorT256 = (__m256*)(vectorTLocal);
	__m256 *vectorTAux256 = (__m256*)(vectorTLocal + 1);
	__m256 *vectorS256 = (__m256*)vectorSLocal;
	__m256 k256 = _mm256_set1_ps(0.5);

	for (int i = 0; i < sizeDatos / 8 - 1; i++) {
		if (ultimo == 1 && i == sizeDatos - 1) {
			vectorSLocal[sizeDatos - 1] = 0.0;
			vectorSLocal[sizeDatos - 2] = (float)((vectorSLocal[sizeDatos - 1] - vectorSLocal[sizeDatos - 2])*0.5);
			vectorSLocal[sizeDatos - 3] = (float)((vectorSLocal[sizeDatos - 2] - vectorSLocal[sizeDatos - 3])*0.5);
			vectorSLocal[sizeDatos - 4] = (float)((vectorSLocal[sizeDatos - 3] - vectorSLocal[sizeDatos - 4])*0.5);
			vectorSLocal[sizeDatos - 5] = (float)((vectorSLocal[sizeDatos - 4] - vectorSLocal[sizeDatos - 5])*0.5);
			vectorSLocal[sizeDatos - 6] = (float)((vectorSLocal[sizeDatos - 5] - vectorSLocal[sizeDatos - 6])*0.5);
			vectorSLocal[sizeDatos - 7] = (float)((vectorSLocal[sizeDatos - 6] - vectorSLocal[sizeDatos - 7])*0.5);
			vectorSLocal[sizeDatos - 8] = (float)((vectorSLocal[sizeDatos - 7] - vectorSLocal[sizeDatos - 8])*0.5);
		}
		else {
			vectorS256[i] = _mm256_sub_ps(vectorT256[i], vectorTAux256[i]);
			vectorS256[i] = _mm256_mul_ps(vectorS256[i], k256);
		}
	}

	return 0;
}

// ---------------------------------------------------------------- MULTIPLICACION ---------------------------------------------------------------- \\

void hilosMultiplicacion(int numThreads, datosStruct* arrayStructs, HANDLE* arrayHilos, int countPos) {
	// Aqui se crean los structs y se llama a la operacion countPos con cada hilo
	for (int i = 0; i < numThreads; i++) {
		arrayStructs[i].size = SIZE / numThreads;
		arrayStructs[i].vector1 = &vectorS[(SIZE / numThreads)*i];
		arrayStructs[i].entero = countPos;
		// Se crea el hilo
		arrayHilos[i] = CreateThread(NULL, 0, multiplicacion, &arrayStructs[i], 0, NULL);
	}

	// Aqui se espera a que todos los hilos acaben
	for (int i = 0; i < numThreads; i++) {
		WaitForSingleObject(arrayHilos[i], INFINITE);
	}
}
DWORD WINAPI multiplicacion(LPVOID datos) {
	// [K * G] == M
	datosStruct *structDatos = (datosStruct*)datos;
	int sizeDatos = structDatos->size;
	int countPos = structDatos->entero;
	float* vectorSLocal = structDatos->vector1;

	__m256 count256 = _mm256_set1_ps(countPos);
	__m256 *mul256 = (__m256*)vectorSLocal;

	for (int i = 0; i < sizeDatos / 8; i++)
		mul256[i] = _mm256_mul_ps(mul256[i], count256);
	return 0;
}

// ---------------------------------------------------------------- AND ---------------------------------------------------------------- \\

void hilosAnd(int numThreads, datosStruct* arrayStructs, HANDLE* arrayHilos) {
	// Aqui se crean los structs y se llama a la operacion countPos con cada hilo
	for (int i = 0; i < numThreads; i++) {
		arrayStructs[i].size = SIZE / numThreads;
		arrayStructs[i].vector1 = &vectorU[(SIZE / numThreads)*i];
		arrayStructs[i].vector2 = &vectorS[(SIZE / numThreads)*i];
		// Se crea el hilo
		arrayHilos[i] = CreateThread(NULL, 0, and, &arrayStructs[i], 0, NULL);
	}
	// Aqui se espera a que todos los hilos acaben
	for (int i = 0; i < numThreads; i++) {
		WaitForSingleObject(arrayHilos[i], INFINITE);
	}
}
DWORD WINAPI and (LPVOID datos) {
	// M AND T
	datosStruct *structDatos = (datosStruct*)datos;
	int sizeDatos = structDatos->size;

	float* vectorULocal = structDatos->vector1;
	float* vectorSLocal = structDatos->vector2;

	__m256 *vectorU256 = (__m256*)vectorULocal;
	__m256 *vectorS256 = (__m256*)vectorSLocal;

	for (int i = 0; i < sizeDatos/8; i++) {
		vectorS256[i] = _mm256_and_ps(vectorU256[i], vectorS256[i]);
	}
	return 0;
}