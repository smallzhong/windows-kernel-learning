#include <ntddk.h>

typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;
	LIST_ENTRY InMemoryOrderLinks;
	LIST_ENTRY InInitializationOrderLinks;
	PVOID DllBase;
	PVOID EntryPoint;
	ULONG SizeOfImage;
	UNICODE_STRING FullDllName;
	UNICODE_STRING BaseDllName;
	ULONG Flags;
	UINT16 LoadCount;
	UINT16 TlsIndex;
	LIST_ENTRY HashLinks;
	PVOID SectionPointer;
	ULONG CheckSum;
	ULONG TimeDateStamp;
	PVOID LoadedImports;
	PVOID EntryPointActivationContext;
	PVOID PatchInformation;
} LDR_DATA_TABLE_ENTRY, * PLDR_DATA_TABLE_ENTRY;

PDRIVER_OBJECT g_Driver;

typedef BOOLEAN(__stdcall* HOOKFUNCTION)(PVOID in_data,              // 传进去的数据
	PVOID out_data,        // 传出来的数据缓冲区
	ULONG in_size,              // 穿进去的数据的大小 
	SIZE_T nSize,                 // 操作码，正常调用readprocessmemory的数字一定是整数，因此这里要设置为负数
	SIZE_T* lpNumberOfBytesRead  // number of bytes read
	);

HOOKFUNCTION oldfunction = NULL;
PVOID ret_addr = NULL; // 裸函数返回地址


NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path);
VOID DriverUnload(PDRIVER_OBJECT driver);
PVOID MemorySearch(PVOID bytecode, ULONG bytecodeLen, PVOID pBeginAddress, PVOID pEndAddress);
VOID HookFun(PVOID in_data, PVOID out_data, ULONG in_size, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead);
BOOLEAN solve();
VOID UnsetInlineHookNtQuerySystemInformation();


#define OP_TEST 0x114514

// 关闭页保护
VOID PageProtectOff()
{
	__asm
	{
		cli; // 关闭中断
		mov eax, cr0;
		and eax, not 0x10000; // WP位置0
		mov cr0, eax;
	}
}

// 开启页保护
VOID PageProtectOn()
{
	__asm
	{
		mov eax, cr0;
		or eax, 0x10000; // WP位置1
		mov cr0, eax;
		sti; // 恢复中断
	}
}

BOOLEAN GetKernelBase(IN PDRIVER_OBJECT driver, OUT PVOID* pkrnlbase, OUT PUINT32 pkrnlsize)
{
	// 设定头和遍历指针
	PLDR_DATA_TABLE_ENTRY pldrHead = (PLDR_DATA_TABLE_ENTRY)(driver->DriverSection);
	PLDR_DATA_TABLE_ENTRY pldrcur = pldrHead;
	UNICODE_STRING usKrnlBaseDllName; // 内核模块名

	RtlInitUnicodeString(&usKrnlBaseDllName, L"ntoskrnl.exe");

	do
	{
		// 如果找到了
		if (RtlCompareUnicodeString(&usKrnlBaseDllName, &pldrcur->BaseDllName, TRUE) == 0)
		{
			*pkrnlbase = pldrcur->DllBase;
			*pkrnlsize = pldrcur->SizeOfImage;
			return TRUE;
		}
		// 指向下一个节点
		pldrcur = (PLDR_DATA_TABLE_ENTRY)pldrcur->InLoadOrderLinks.Flink;

	} while (pldrcur != pldrHead);

	return FALSE;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
	g_Driver = driver;
	driver->DriverUnload = DriverUnload;

	return (solve());

	return STATUS_SUCCESS;
}

BOOLEAN solve()
{
	PVOID pKrnlBase = NULL;
	ULONG uKrnlImageSize = 0;

	// 获取内核模块基址和大小
	if (GetKernelBase(g_Driver, &pKrnlBase, &uKrnlImageSize) == FALSE)
	{
		DbgPrint("获取内核模块基址和大小失败\r\n");
		return FALSE;
	}
	else
	{
		DbgPrint("krnlbase = 0x%x, imagesize = 0x%x\r\n", (ULONG)pKrnlBase, uKrnlImageSize);
	}

	// NTREADVIRTUALMEMORY特征码
	// 64 A1 24 01     00 00 8B F8
	// 8A 87 40 01     00 00 88 45 
	// E0 8B 75 14
	ULONG bytecode[] = {
		0x0124a164, 0xf88b0000,
		0x0140878a, 0x45880000,
		0x14758be0
	};
	oldfunction = (HOOKFUNCTION)((ULONG)MemorySearch(
		bytecode, sizeof(bytecode), pKrnlBase, (PVOID)((ULONG)pKrnlBase + uKrnlImageSize)) - 12);

	DbgPrint("NTREADVIRTUALMEMORY = 0x%x\r\n", oldfunction);

	// 设置裸函数的返回地址
	// 返回到  PAGE:004B14E4 88 45 E0 mov     [ebp+AccessMode], al
	ret_addr = (ULONG)oldfunction + 20 + 5;

	UCHAR patchbyte[6];
	RtlFillMemory(patchbyte, sizeof patchbyte, 0x90);
	*patchbyte = 0xe9; // JMP
	*(PULONG)(patchbyte + 1) = (ULONG)((ULONG)HookFun - (ULONG)ret_addr);
	DbgPrint("hookfun = 0x%x\r\n"
		"retaddr = 0x%x\r\n(ULONG)((ULONG)HookFun - (ULONG)ret_addr) = 0x%x\r\n",
		HookFun,
		ret_addr,
		(ULONG)((ULONG)HookFun - (ULONG)ret_addr));

	PageProtectOff();
	memcpy((PVOID)((ULONG)oldfunction + 20), patchbyte, 6);
	PageProtectOn();
}

__declspec(naked) VOID HookFun(PVOID in_data, PVOID out_data, ULONG in_size, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead)
{
	__asm
	{
		push ebp
		mov ebp, esp
		sub esp, 0x50
		push fs
		pushad
		pushfd
	}

	//DbgPrint("%x %x %x %x %x\r\n", in_data, out_data, in_size, nSize, lpNumberOfBytesRead);

	//switch ((int)in_data)
	//{
	//case  0x12345:
	//	DbgPrint("1231312\r\n");
	//	UnsetInlineHookNtQuerySystemInformation();

	//	__asm
	//	{
	//		popfd
	//		popad
	//		pop fs

	//		add esp, 0x50
	//		pop ebp
	//		ret 0x14 // 内平栈，正常返回
	//	}
	//	break;
	//}

	//if (nSize == OP_TEST)
	//{
	//	__asm
	//	{
	//		popfd
	//		popad
	//		pop fs

	//		add esp, 0x50
	//		pop ebp
	//		ret 0x14 // 内平栈，正常返回
	//	}
	//}

	//// 如果不进switch
	//DbgPrint("没进switch\r\n");

	DbgPrint("1231312\r\n");

	__asm
	{
		popfd
		popad
		pop fs
		add esp, 0x50
		pop ebp
		mov     al, [edi + 140h]
		jmp ret_addr
	}





}

// 特征码搜索
PVOID MemorySearch(PVOID bytecode, ULONG bytecodeLen, PVOID pBeginAddress, PVOID pEndAddress)
{
	PVOID pCur = pBeginAddress;
	while (pCur != pEndAddress)
	{
		if (RtlCompareMemory(bytecode, pCur, bytecodeLen) == bytecodeLen)
		{
			return pCur;
		}
		((ULONG)pCur)++;
	}
	return 0;
}

VOID UnsetInlineHookNtQuerySystemInformation()
{
	UCHAR ReplaceByte[6] = { 0x8A, 0x87, 0x40 ,0x01, 0x00, 0x00 };
	PageProtectOff();
	memcpy((PVOID)((ULONG)oldfunction + 20), ReplaceByte, 6);
	PageProtectOn();
}

VOID DriverUnload(PDRIVER_OBJECT driver)
{
	UnsetInlineHookNtQuerySystemInformation();
	DbgPrint("驱动卸载成功\n");
}
