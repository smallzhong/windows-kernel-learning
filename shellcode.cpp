#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winioctl.h>
#include <iostream>

using namespace std;

// 不能用库函数
#define TOUPPER(x) ((((x) >= 'a') && ((x) <= 'z')) ? ((x)-32) : (x))
#define TOLOWER(x) ((((x) >= 'A') && ((x) <= 'Z')) ? ((x) + 32) : (x))
#define EXIT_ERROR(x)                                 \
    do                                                \
    {                                                 \
	cout << "error in line " << __LINE__ << endl; \
	cout << x;                                    \
	getchar();                                    \
	exit(EXIT_FAILURE);                           \
    } while (0)
#define MY_ASSERT(x)                         \
    do                                       \
{                                        \
	if (!x)                              \
	EXIT_ERROR("ASSERTION failed!"); \
    } while (0)

typedef struct _UNICODE_STRING
{
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

typedef struct _PEB_LDR_DATA
{
    DWORD Length;
    bool Initialized;
    PVOID SsHandle;
    LIST_ENTRY InLoadOrderModuleList;
    LIST_ENTRY InMemoryOrderModuleList;
    LIST_ENTRY InInitializationOrderModuleList;
} PEB_LDR_DATA, *PPEB_LDR_DATA;

typedef struct _LDR_DATA_TABLE_ENTRY
{
    LIST_ENTRY InLoadOrderLinks;
    LIST_ENTRY InMemoryOrderLinks;
    LIST_ENTRY InInitializationOrderLinks;
    PVOID DllBase;
    PVOID EntryPoint;
    UINT32 SizeOfImage;
    UNICODE_STRING FullDllName;
    UNICODE_STRING BaseDllName;
    UINT32 Flags;
    USHORT LoadCount;
    USHORT TlsIndex;
    LIST_ENTRY HashLinks;
    PVOID SectionPointer;
    UINT32 CheckSum;
    UINT32 TimeDateStamp;
    PVOID LoadedImports;
    PVOID EntryPointActivationContext;
    PVOID PatchInformation;
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

// DWORD RVA_TO_FOA(LPVOID hKernel32, PIMAGE_DOS_HEADER pDosHeader,
//                  PIMAGE_NT_HEADERS32 pNTHeader,
//                  PIMAGE_SECTION_HEADER pSectionHeader, IN DWORD RVA);

typedef HMODULE(WINAPI *PLOADLIBRARYA)(LPCSTR);
typedef DWORD(WINAPI *PGETPROCADDRESS)(HMODULE, LPCSTR);
typedef DWORD(WINAPI *PMESSAGEBOXA)(HWND, LPCSTR, LPCSTR, UINT);

void init()
{
//	freopen("c:\\123.txt", "w", stdout);
}

int main()
{
	init();
	PPEB_LDR_DATA pLDR = NULL;
	HMODULE hKernel32 = NULL;
    HMODULE hUser32 = NULL;
	PGETPROCADDRESS pGetProcAddress = NULL;
	PLOADLIBRARYA pLoadLibraryA = NULL;
    PMESSAGEBOXA pMessageBoxA = NULL;

	char szKernel32[] = {
        'k', 0, 'e', 0, 'r', 0, 'n', 0, 'e', 0, 'l', 0, '3', 0,
			'2', 0, '.', 0, 'd', 0, 'l', 0, 'l', 0, 0,   0};  // Unicode
		char szUser32[] = {'u', 's', 'e', 'r', '3', '2', '.', 'd', 'l', 'l', 0};
		char szGetProcAddress[] = {'G', 'e', 't', 'P', 'r', 'o', 'c', 'A',
			'd', 'd', 'r', 'e', 's', 's', 0};
		char szLoadLibrary[] = {'L', 'o', 'a', 'd', 'L', 'i', 'b',
			'r', 'a', 'r', 'y', 'A', 0};
		char szMessageBoxA[] = {'M', 'e', 's', 's', 'a', 'g',
			'e', 'B', 'o', 'x', 'A', 0};
		char szHelloShellCode[] = {'H', 'e', 'l', 'l', 'o', 'S', 'h', 'e',
                               'l', 'l', 'C', 'o', 'd', 'e', 0};

	PPEB_LDR_DATA pltr = NULL;

	__asm {
		mov eax,fs:[0x30]  // PEB
		mov eax,[eax+0x0C]  // PEB->LDR
		mov pLDR,eax
    }
	
	// 这里不是一个指针，要&
	PLDR_DATA_TABLE_ENTRY pldrHead = (PLDR_DATA_TABLE_ENTRY)(&(pLDR->InLoadOrderModuleList));
	PLDR_DATA_TABLE_ENTRY pldrcur = (PLDR_DATA_TABLE_ENTRY)(pldrHead->InLoadOrderLinks.Flink);

	

	while (pldrcur != pldrHead)
	{
		PCHAR pchar1 = szKernel32;
		printf("%ws\n", pldrcur->BaseDllName.Buffer);
		PCHAR pchar2 = (PCHAR)(pldrcur->BaseDllName.Buffer);
		
		BOOL equalFlag = FALSE;

		while (1)
		{
			if ((*((PWORD)pchar1) == 0) && (*((PWORD)pchar2) == 0))
			{
				equalFlag = TRUE;
				break;
			}

			if (TOUPPER(*pchar1) != TOUPPER(*pchar2))
            {
                break;
            }
            pchar1 += 2;
            pchar2 += 2;
		}

		if (equalFlag)
		{
			hKernel32 = (HMODULE)(pldrcur->DllBase);
		}
		
		// 往前指
		pldrcur = (PLDR_DATA_TABLE_ENTRY)(pldrcur->InLoadOrderLinks.Flink);
	}

	if (hKernel32 == NULL)
	{
		printf("搜索kernel32失败！");
		getchar();
		exit(EXIT_FAILURE);
	}
	
	PIMAGE_DOS_HEADER pDosHeader = NULL;
	PIMAGE_NT_HEADERS32 pNTHeader = NULL;
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;

	pDosHeader = (PIMAGE_DOS_HEADER)(hKernel32);
    MY_ASSERT(pDosHeader);
    MY_ASSERT((pDosHeader->e_magic == IMAGE_DOS_SIGNATURE));
	
	pNTHeader =
        (PIMAGE_NT_HEADERS32)((PBYTE)hKernel32 + pDosHeader->e_lfanew);
	MY_ASSERT(pNTHeader);
    if (pNTHeader->FileHeader.SizeOfOptionalHeader != 0xe0)
        EXIT_ERROR("this is not a 32-bit executable file.");

	pSectionHeader = (PIMAGE_SECTION_HEADER)(
        (PBYTE)pNTHeader + sizeof(IMAGE_NT_SIGNATURE) +
        sizeof(IMAGE_FILE_HEADER) + pNTHeader->FileHeader.SizeOfOptionalHeader);
	MY_ASSERT(pSectionHeader);

	PIMAGE_EXPORT_DIRECTORY pExportDirectory = (PIMAGE_EXPORT_DIRECTORY)(
            (DWORD)hKernel32 + pNTHeader->OptionalHeader.DataDirectory[0].VirtualAddress);

	PDWORD AddressOfFunctions =
		(PDWORD)((DWORD)hKernel32 + pExportDirectory->AddressOfFunctions);
	PDWORD AddressOfNames =
		(PDWORD)((DWORD)hKernel32 + pExportDirectory->AddressOfNames);
	PWORD AddressOfNameOridinals =
            (PWORD)((DWORD)hKernel32 + pExportDirectory->AddressOfNameOrdinals);

	int i;
	for (i = 0; i < pExportDirectory->NumberOfFunctions; i ++ )
	{
		PCHAR pchar1 = szGetProcAddress;
		PCHAR pchar2 = (PCHAR)((DWORD)hKernel32 + AddressOfNames[i]);

		BOOL flag = FALSE;

		while (1)
		{
			if (*pchar1 == 0 && *pchar2 == 0)
			{
				flag = TRUE;
				break;
			}

			if (*pchar1 != *pchar2) break;
			
			pchar1 ++ , pchar2 ++ ;
		}
		
		if (flag == TRUE)
		{
			printf("找到了！,函数地址0x%x\n", (DWORD)((PCHAR)hKernel32 + AddressOfFunctions[i]));
			

			pGetProcAddress = (PGETPROCADDRESS)((PCHAR)hKernel32 + AddressOfFunctions[i]);

			if (pGetProcAddress != NULL)
			{
				// LoadLibrary
				pLoadLibraryA = (PLOADLIBRARYA)pGetProcAddress(hKernel32, szLoadLibrary);
				
				hUser32 = pLoadLibraryA(szUser32);
				
				pMessageBoxA = (PMESSAGEBOXA)pGetProcAddress(hUser32, szMessageBoxA);

				pMessageBoxA(0, szHelloShellCode, 0, MB_OK);
			}
			
			getchar();
		}
	}

	printf("无事发生");
	getchar();

	return 0;
}

// DWORD RVA_TO_FOA(LPVOID hKernel32, PIMAGE_DOS_HEADER pDosHeader,
//                  PIMAGE_NT_HEADERS32 pNTHeader,
//                  PIMAGE_SECTION_HEADER pSectionHeader, IN DWORD RVA)
// {
//     if (RVA < pNTHeader->OptionalHeader.SizeOfHeaders)
//         return RVA;
// 	
//     for (int i = 0; i < pNTHeader->FileHeader.NumberOfSections; i++)
//     {
//         if (RVA >= pSectionHeader[i].VirtualAddress &&
//             RVA < pSectionHeader[i].VirtualAddress +
// 			pSectionHeader[i].Misc.VirtualSize)
//         {
//             return (RVA - pSectionHeader[i].VirtualAddress +
// 				pSectionHeader[i].PointerToRawData);
//         }
//     }
// 	
//     EXIT_ERROR("rva to foa failure!");
// }