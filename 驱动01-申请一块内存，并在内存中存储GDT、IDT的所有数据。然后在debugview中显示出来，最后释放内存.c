#include <ntddk.h>
#include <ntdef.h>

#pragma warning(disable:28159)
#pragma warning(disable:28222)
#pragma warning(disable:28209)


// 卸载函数
VOID DriverUnload(PDRIVER_OBJECT driver)
{
	DbgPrint("驱动程序停止运行了.\r\n");
}

// 入口函数，相当于main
NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
	// 设置一个卸载函数，便于退出
	driver->DriverUnload = DriverUnload;

	UCHAR gdt[6];
	UCHAR idt[6];

	__asm
	{
		sgdt fword ptr gdt
		sidt fword ptr idt
	}

	ULONG gdtAddr, idtAddr;
	USHORT gdtSize, idtSize;

	gdtAddr = *(PULONG)(gdt + 2);
	idtAddr = *(PULONG)(idt + 2);
	gdtSize = *(PUSHORT)gdt;
	idtSize = *(PUSHORT)idt;

	DbgPrint("gdtAddr = %x, idtaddr = %x, gdtsize = %x, idtsize = %x\n", gdtAddr, idtAddr, gdtSize, idtSize);

	PVOID gdtBuffer = ExAllocatePool(PagedPool, gdtSize);
	PVOID idtBuffer = ExAllocatePool(PagedPool, idtSize);

	RtlMoveMemory(gdtBuffer, (PVOID)gdtAddr, gdtSize);
	RtlMoveMemory(idtBuffer, (PVOID)idtAddr, idtSize);

	ULONG i;
	DbgPrint("打印GDT表如下:\n");
	for (i = 0; i < gdtSize; i += 16)
		DbgPrint("%08x  %08x`%08x %08x`%08x\r\n", gdtAddr + i,
			((PULONG)(gdtAddr + i))[1], ((PULONG)(gdtAddr + i))[0],
			((PULONG)(gdtAddr + i))[3], ((PULONG)(gdtAddr + i))[2]);

	DbgPrint("\n打印IDT表如下:\n");
	for (i = 0; i < idtSize; i += 16)
		DbgPrint("%08x  %08x`%08x %08x`%08x\r\n", idtAddr + i,
			((PULONG)(idtAddr + i))[1], ((PULONG)(idtAddr + i))[0],
			((PULONG)(idtAddr + i))[3], ((PULONG)(idtAddr + i))[2]);

	ExFreePool(gdtBuffer);
	ExFreePool(idtBuffer);

	return STATUS_SUCCESS;
}
