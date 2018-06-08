// Single thread application
#include <Windows.h>
#include <stdio.h>		// Para poder usar el printf
#include <cstdlib>		// Para los random
#include <thread>

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

// ---------------------------------------------------------------- MAIN ---------------------------------------------------------------- \\

int main() {
	printf("Aplicacion multithread.\r\n");
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
	int resOp1 = 0;
	datosStruct *structDatos = (datosStruct*)datos;
	int sizeDatos = structDatos->size;
	float* vectorLocal = structDatos->vector1;
	for (int i = 0; i < sizeDatos; i++) {
		if (vectorLocal[i] > 0) {
			resOp1++;
		}
	}
	structDatos->resultados[structDatos->posResultados] = resOp1;
	return 0;
}

// ---------------------------------------------------------------- SUB2 ---------------------------------------------------------------- \\

void hilosSub2(int numThreads, datosStruct* arrayStructs, HANDLE* arrayHilos) {
	// Aqui se crean los structs y se llama a la operacion countPos con cada hilo
	for (int i = 0; i < numThreads; i++) {
		arrayStructs[i].size = SIZE / numThreads;
		arrayStructs[i].vector1 = &vectorU[(SIZE / numThreads)*i];
		arrayStructs[i].vector2 = &vectorAux1[(SIZE / numThreads)*i];
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
	float* vectorULocal = structDatos->vector1;
	float* vectorAuxLocal = structDatos->vector2;
	for (int i = 0; i < sizeDatos; i++) {
		if (ultimo == 1 && i == SIZE - 1) {
			vectorAuxLocal[i] = -vectorULocal[i] / 2;
		}
		else {
			vectorAuxLocal[i] = (vectorULocal[i + 1] - vectorULocal[i]) / 2;
		}
	}
	return 0;
}

// ---------------------------------------------------------------- MULTIPLICACION ---------------------------------------------------------------- \\

void hilosMultiplicacion(int numThreads, datosStruct* arrayStructs, HANDLE* arrayHilos, int countPos) {
	// Aqui se crean los structs y se llama a la operacion countPos con cada hilo
	for (int i = 0; i < numThreads; i++) {
		arrayStructs[i].size = SIZE / numThreads;
		arrayStructs[i].vector1 = &vectorAux1[(SIZE / numThreads)*i];
		arrayStructs[i].entero = countPos;
		arrayStructs[i].vector2 = &vectorAux2[(SIZE / numThreads)*i];
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
	float* vectorAuxLocal = structDatos->vector1;
	float* vectorResLocal = structDatos->vector2;
	for (int i = 0; i < sizeDatos; i++) {
		vectorResLocal[i] = vectorAuxLocal[i] * countPos;
	}
	return 0;
}

// ---------------------------------------------------------------- AND ---------------------------------------------------------------- \\

void hilosAnd(int numThreads, datosStruct* arrayStructs, HANDLE* arrayHilos) {
	// Aqui se crean los structs y se llama a la operacion countPos con cada hilo
	for (int i = 0; i < numThreads; i++) {
		arrayStructs[i].size = SIZE / numThreads;
		arrayStructs[i].vector1 = &vectorAux2[(SIZE / numThreads)*i];
		arrayStructs[i].vector2 = &vectorT[(SIZE / numThreads)*i];
		arrayStructs[i].vector3 = &vectorRes[(SIZE / numThreads)*i];
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

	float* vectorAux2Local = structDatos->vector1;
	int* tmp1 = (int*)vectorAux2Local;

	float* vectorTLocal = structDatos->vector2;
	int* tmp2 = (int*)vectorTLocal;

	float* vectorResLocal = structDatos->vector3;
	int res = 0;
	for (int i = 0; i < sizeDatos; i++) {
		res = tmp1[i] & tmp2[i];
		vectorResLocal[i] = *(float*)&res;
	}
	return 0;
}