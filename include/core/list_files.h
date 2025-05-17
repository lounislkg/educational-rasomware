#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    char *cFileName; // nom du fichier + long que MAX_PATH
    FILETIME ftLastAccessTime; // date de dernier accès
    DWORD    nFileSizeHigh; // taille du fichier (32 bits)
} FileData;

typedef struct {
    FileData* fd;    // tableau de HANDLEs
    int length;        // nombre d'éléments utilisés
    int capacity;    // capacité actuelle allouée
} FileDataArray;

void list_files(char *dir, FileDataArray *arr, DWORD* file_size);
void initHandleArray(FileDataArray* arr);
void freeHandleArray(FileDataArray* arr);
