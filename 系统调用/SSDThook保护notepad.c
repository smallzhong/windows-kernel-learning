#include <ntddk.h>
#include <ntstatus.h>

/************************************************************************/
/* 类型声明                                                             */
/************************************************************************/

// 系统服务表
typedef struct _KSYSTEM_SERVICE_TABLE
{
    PULONG ServiceTableBase;        // 函数地址表（SSDT）
    PULONG ServiceCounterTableBase; // SSDT 函数被调用的次数
    ULONG NumberOfService;          // 函数个数
    PULONG ParamTableBase;          // 函数参数表（SSPT）
} KSYSTEM_SERVICE_TABLE, *PKSYSTEM_SERVICE_TABLE;

typedef struct _KSERVICE_TABLE_DESCRIPTOR
{
    KSYSTEM_SERVICE_TABLE ntoskrnl; // 内核函数
    KSYSTEM_SERVICE_TABLE win32k;   // win32k.sys 函数
    KSYSTEM_SERVICE_TABLE unUsed1;
    KSYSTEM_SERVICE_TABLE unUsed2;
} KSERVICE_TABLE_DESCRIPTOR, *PKSERVICE_TABLE_DESCRIPTOR;

// NTOPENPROCESS
typedef NTSTATUS (*NTTERMINATEPROCESS)(HANDLE hProcess, ULONG uExitCode);

/************************************************************************/
/* 函数声明                                                             */
/************************************************************************/

VOID DriverUnload(PDRIVER_OBJECT pDriver);
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING reg_path);
VOID PageProtectOff();
VOID PageProtectOn();
VOID HookNtOpenProcess();
VOID UnHookNtOpenProcess();

/************************************************************************/
/* 全局变量                                                             */
/************************************************************************/

extern PKSERVICE_TABLE_DESCRIPTOR KeServiceDescriptorTable; // ntoskrnl.exe 导出的全局变量
ULONG uOldNtTerminateProcess;                               // 旧的函数地址

/************************************************************************/
/* 函数定义                                                             */
/************************************************************************/
// 被修改的 TerminateProcess 函数，简单打印参数
NTSTATUS TerminateProcess(HANDLE hProcess, // handle to the process
                          ULONG uExitCode  // exit code for the process
)
{
    PEPROCESS pEprocess;
    NTSTATUS status;
    PCHAR ImageFileName;
    DbgPrint("%x %x\r\n", hProcess, uExitCode);
    status = ObReferenceObjectByHandle(hProcess, FILE_ANY_ACCESS, *PsProcessType, KernelMode, &pEprocess, NULL);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    ImageFileName = (PCHAR)pEprocess + 0x174;
    if (strcmp(ImageFileName, "notepad.exe") == 0)
    {
        if (hProcess == (HANDLE)0xFFFFFFFF)
        {
            // 通过关闭按钮关闭
            return ((NTTERMINATEPROCESS)uOldNtTerminateProcess)(hProcess, uExitCode);
        }
        else
        {
            // 通过任务管理器关闭
            DbgPrint("Terminate denied. %s: NtTerminateProcess(%x, %x)\n", ImageFileName, hProcess, uExitCode);
            return STATUS_ACCESS_DENIED;
        }
    }
    return ((NTTERMINATEPROCESS)uOldNtTerminateProcess)(hProcess, uExitCode);
}

// 驱动入口
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING reg_path)
{
    // HOOK
    HookNtOpenProcess();

    pDriver->DriverUnload = DriverUnload;

    return STATUS_SUCCESS;
}

// 卸载驱动
VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
    UnHookNtOpenProcess();
    DbgPrint("Driver unloaded.\n");
}

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

// HOOK NtOpenProcess
VOID HookNtOpenProcess()
{
    PageProtectOff();
    uOldNtTerminateProcess = KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[0x101];
    KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[0x101] = (ULONG)TerminateProcess;
    PageProtectOn();
}

// UnHOOK NtOpenProcess
VOID UnHookNtOpenProcess()
{
    PageProtectOff();
    KeServiceDescriptorTable->ntoskrnl.ServiceTableBase[0x101] = uOldNtTerminateProcess;
    PageProtectOn();
}