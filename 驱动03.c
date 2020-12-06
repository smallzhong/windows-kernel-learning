#include <ntddk.h>

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
	UINT16 LoadCount;
	UINT16 TlsIndex;
	LIST_ENTRY HashLinks;
	PVOID SectionPointer;
	UINT32 CheckSum;
	UINT32 TimeDateStamp;
	PVOID LoadedImports;
	PVOID EntryPointActivationContext;
	PVOID PatchInformation;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path);
VOID DriverUnload(PDRIVER_OBJECT driver);

// 入口函数，相当于main
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
	driver->DriverUnload = DriverUnload;

	PLDR_DATA_TABLE_ENTRY phead;
	PLDR_DATA_TABLE_ENTRY pcur;

	// 设定phead
	phead = (PLDR_DATA_TABLE_ENTRY)(driver->DriverSection);
	pcur = phead;

	do
	{
		DbgPrint("DllBase: %p, SizeOfImage: %08X %wZ\n", pcur->DllBase, pcur->SizeOfImage, &(pcur->FullDllName));
		pcur = (PLDR_DATA_TABLE_ENTRY)(pcur->InLoadOrderLinks.Flink);
	} while (phead != pcur);
	
	return STATUS_SUCCESS;
}

// 卸载函数
VOID DriverUnload(PDRIVER_OBJECT driver)
{
	DbgPrint("驱动卸载成功\n");
}
