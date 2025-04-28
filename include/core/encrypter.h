#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "winternl.h"
#include <stdio.h>
#include <stdint.h>


#pragma comment(lib, "ntdll")

// To compile this code, you need to link against ntdll.lib. 
// The pragma comment is ignored by gcc with minGW-w64, so you need to link it manually.
// You can do this by adding -lntdll to your gcc command line or by using the -l option in your IDE.
// Example: gcc -o my_program encrypter.c NtCreateSection.o [...] -lntdll

typedef enum _SECTION_INHERIT {
    ViewShare=1,
    ViewUnmap=2
} SECTION_INHERIT, *PSECTION_INHERIT;

// Using the NtCreateFile prototype to define a prototype for SysNtCreateFile.
// The prorotype name needs to match the procedure name defined in the NtCreateFile.asm
// EXTERN_C tells the compiler to link this function as a C function and use stdcall
// calling convention - Important!
EXTERN_C NTSTATUS SysNtCreateFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	POBJECT_ATTRIBUTES ObjectAttributes,
	PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength);

EXTERN_C NTSTATUS SysNtCreateSection(
	OUT PHANDLE SectionHandle,
	IN ULONG DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN PLARGE_INTEGER MaximumSize OPTIONAL,
	IN ULONG PageAttributess,
	IN ULONG SectionAttributes,
	IN HANDLE FileHandle OPTIONAL);


EXTERN_C NTSTATUS SysNtMapViewOfSection(
	IN HANDLE SectionHandle,
	IN HANDLE ProcessHandle,
	IN OUT PVOID *BaseAddress OPTIONAL,
	IN ULONG ZeroBits OPTIONAL,
	IN ULONG CommitSize,
	IN OUT PLARGE_INTEGER SectionOffset OPTIONAL,
	IN OUT PULONG ViewSize,
	IN SECTION_INHERIT InheritDisposition,
	IN ULONG AllocationType OPTIONAL,
	IN ULONG Protect
);