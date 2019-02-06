#include "pch.h"
#include <Windows.h>
#include <stdio.h>
#define BLOCK_SIZE   1024
// struttura di definizione dell'area di memoria condivisa
#pragma warning ( disable: 4996 )

struct SHARED
{
	unsigned char buffer[BLOCK_SIZE]; 
	// buffer blocco dati
	unsigned int count;
	// dimensione in byte del blocco dati
	int end; // flag di segnalazione della fine dei dati
};

int main(int argc, char* argv[])
{
	struct SHARED *shared_data;
	HANDLE shared_map, empty_semaphore, full_semaphore;
	FILE* output_file;
	long count;
	if (argc != 2)
	{
		printf("Uso: %s output-file\r\n", argv[0]);
		return -1;
	}// apertura area di memoria condivisa denominata SHARED
	shared_map = OpenFileMapping(FILE_MAP_READ, FALSE, (LPCWSTR)"SHARED");
	if (shared_map == NULL)
	{
		printf("Errore apertura memory-map\r\n");
		return -1;
	}// associazione al processo dell'area di memoria condivisa
	shared_data = (struct SHARED*)MapViewOfFile( shared_map, FILE_MAP_READ, 0,0, sizeof(struct SHARED));
	if(shared_data == NULL)
	{
		printf("Errore associazione memory-map\r\n");
		CloseHandle(shared_map);
		return -1;
	}// apertura semaforo denominato EMPTY
	empty_semaphore = OpenSemaphore( SEMAPHORE_ALL_ACCESS, FALSE, (LPCWSTR)"EMPTY");
	if (empty_semaphore == NULL)
	{
		printf("Errore apertura semaforo EMPTY\r\n");
		UnmapViewOfFile(shared_data);
		CloseHandle(shared_map);
		return -1;
	}
	// apertura semaforo denominato FULL
	full_semaphore = OpenSemaphore( SEMAPHORE_ALL_ACCESS, FALSE, (LPCWSTR)"FULL");
	if (full_semaphore == NULL)
	{
		printf("Errore apertura semaforo FULL\r\n");
		UnmapViewOfFile(shared_data);
		CloseHandle(shared_map);
		CloseHandle(empty_semaphore);
		return -1;
	}// apertura file output in scrittura
	output_file = fopen(argv[1], "wb");
	if (output_file == NULL)
	{
		printf("Errore apertura file %s\r\n", argv[1]);
		UnmapViewOfFile(shared_data);
		CloseHandle(shared_map);
		CloseHandle(empty_semaphore);
		CloseHandle(full_semaphore);
		return -1;
	}// ciclo di consumazione sincronizzata dei dati
	do
	{
		WaitForSingleObject(full_semaphore, INFINITE);
		// scrittura blocco-dati del buffer condiviso nel file
		fwrite(shared_data->buffer, 1, shared_data->count, output_file);
		ReleaseSemaphore(empty_semaphore, 1, &count);
	}  
	while (!(shared_data->end));
	
	fclose(output_file);
	UnmapViewOfFile(shared_data);
	CloseHandle(shared_map);
	CloseHandle(empty_semaphore);
	CloseHandle(full_semaphore);

	return 0;
}