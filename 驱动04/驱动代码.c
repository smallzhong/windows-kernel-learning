#include <ntddk.h>

#define DEVICE_NAME L"\\Device\\HbgDev"
#define SYMBOLICLINK_NAME L"\\??\\HbgDevLnk"

#define OPER1 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define OPER2 CTL_CODE(FILE_DEVICE_UNKNOWN, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)

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
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY;

// 函数声明
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegPath);
VOID DriverUnload(PDRIVER_OBJECT pDriver);
NTSTATUS IrpCreateProc(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS IrpCloseProc(PDEVICE_OBJECT pDevObj, PIRP pIrp);
NTSTATUS IrpDeviceControlProc(PDEVICE_OBJECT pDevObj, PIRP pIrp);
BOOLEAN GetKernelBase(IN PDRIVER_OBJECT driver, OUT PVOID *pkrnlbase, OUT PUINT32 pkrnlsize);
PVOID MemorySearch(IN PVOID bytecode, IN PVOID beginAddr, IN UINT32 length, IN PVOID endAddr);

// PspTerminateProcess函数指针
typedef NTSTATUS (*_PspTerminateProcess)(PEPROCESS pEprocess,
                                         NTSTATUS ExitCode);
_PspTerminateProcess PspTerminateProcess;

// 全局变量
PDRIVER_OBJECT g_driver;

// 入口函数
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriver, PUNICODE_STRING RegPath)
{
    g_driver = pDriver;

    NTSTATUS status;
    ULONG uIndex = 0;
    PDEVICE_OBJECT pDeviceObj = NULL; // 设备对象指针
    UNICODE_STRING DeviceName;        // 设备名，0环用
    UNICODE_STRING SymbolicLinkName;  // 符号链接名，3环用

    // 创建设备名称
    RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
    // 创建设备
    status = IoCreateDevice(pDriver, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObj);
    if (status != STATUS_SUCCESS)
    {
        IoDeleteDevice(pDeviceObj);
        DbgPrint("创建设备失败.\n");
        return status;
    }
    DbgPrint("创建设备成功.\n");
    // 设置交互数据的方式
    pDeviceObj->Flags |= DO_BUFFERED_IO;
    // 创建符号链接
    RtlInitUnicodeString(&SymbolicLinkName, SYMBOLICLINK_NAME);
    IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName);
    // 设置分发函数
    pDriver->MajorFunction[IRP_MJ_CREATE] = IrpCreateProc;
    pDriver->MajorFunction[IRP_MJ_CLOSE] = IrpCloseProc;
    pDriver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IrpDeviceControlProc;
    // 设置卸载函数
    pDriver->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}

// 封装杀进程API
BOOLEAN KillProcessByPid(PDRIVER_OBJECT driver, UINT32 pid)
{
    // 内存特征码
    UINT32 bytecode[] = {
        0x8b55ff8b,
        0xa16456ec,
        0x00000124,
        0x3b08758b};
    PVOID pKrnlBase;       // 内核基址
    UINT32 uKrnlImageSize; // 内核大小
    PEPROCESS pEprocess;   // 要关闭的进程的EPROCESS

    // 获取内核模块基址和大小
    if (GetKernelBase(driver, &pKrnlBase, &uKrnlImageSize) == FALSE)
    {
        DbgPrint("获取内核模块基址和大小失败\r\n");
        return FALSE;
    }
    else
        DbgPrint("内核基址: %p，大小: %X\n", pKrnlBase, uKrnlImageSize);

    PspTerminateProcess =
        MemorySearch(bytecode, pKrnlBase, sizeof(bytecode),
                     (PVOID)((PCHAR)pKrnlBase + uKrnlImageSize));

    if (PspTerminateProcess != NULL)
    {
        DbgPrint("找到特征码，函数开头：%p, 开头4字节：0x%x\r\n",
                 PspTerminateProcess, *((PUINT32)PspTerminateProcess - 6));
    }

    else
    {
        DbgPrint("没有找到特征码！");
        return FALSE;
    }

    PsLookupProcessByProcessId((HANDLE)pid, &pEprocess); // 记事本PID是1796
    // 调用 PspTerminateProcess 关闭进程
    PspTerminateProcess(pEprocess, 0);
    DbgPrint("0x%x进程被 PspTerminateProcess 函数关闭了.\n", pid);

    return TRUE;
}

BOOLEAN GetKernelBase(IN PDRIVER_OBJECT driver, OUT PVOID *pkrnlbase, OUT PUINT32 pkrnlsize)
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

// 用来查找特征码的起始位置
PVOID MemorySearch(IN PVOID bytecode, IN PVOID beginAddr, IN UINT32 length, IN PVOID endAddr)
{
    DbgPrint("inside MemorySearch, length = %d\r\n", length);
    PVOID pcur = beginAddr;

    while (pcur != endAddr)
    {
        UINT32 tlen = RtlCompareMemory(pcur, bytecode, length);
        //if (RtlCompareMemory(pcur, bytecode, length) == length)

        if (tlen == length)
        {
            DbgPrint("找到特征码，内存地址%p\r\n", pcur);
            return pcur;
        }
        ((UINT32)pcur)++;
    }

    // 没有找到则返回空指针
    DbgPrint("memorysearch失败\r\n");
    return NULL;
}

// 卸载驱动
VOID DriverUnload(PDRIVER_OBJECT pDriver)
{
    UNICODE_STRING SymbolicLinkName;
    // 删除符号链接，删除设备
    RtlInitUnicodeString(&SymbolicLinkName, SYMBOLICLINK_NAME);
    IoDeleteSymbolicLink(&SymbolicLinkName);
    IoDeleteDevice(pDriver->DeviceObject);
    DbgPrint("驱动卸载成功\n");
}

// 不设置这个函数，则Ring3调用CreateFile会返回1
// IRP_MJ_CREATE 处理函数
NTSTATUS IrpCreateProc(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    DbgPrint("应用层连接设备.\n");
    // 返回状态如果不设置，Ring3返回值是失败
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// IRP_MJ_CLOSE 处理函数
NTSTATUS IrpCloseProc(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    DbgPrint("应用层断开连接设备.\n");
    // 返回状态如果不设置，Ring3返回值是失败
    pIrp->IoStatus.Status = STATUS_SUCCESS;
    pIrp->IoStatus.Information = 0;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// IRP_MJ_DEVICE_CONTROL 处理函数
NTSTATUS IrpDeviceControlProc(PDEVICE_OBJECT pDevObj, PIRP pIrp)
{
    // DbgPrint("IrpDeviceControlProc.\n");
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    PIO_STACK_LOCATION pIrpStack;
    ULONG uIoControlCode;
    PVOID pIoBuffer;
    ULONG uInLength;
    ULONG uOutLength;
    ULONG uRead;
    ULONG uWrite;

    // 设置临时变量的值
    uRead = 0;
    uWrite = 0x12345678;
    // 获取IRP数据
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    // 获取控制码
    uIoControlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
    // 获取缓冲区地址（输入输出是同一个）
    pIoBuffer = pIrp->AssociatedIrp.SystemBuffer;
    // Ring3 发送数据的长度
    uInLength = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    // Ring0 发送数据的长度
    uOutLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;

    switch (uIoControlCode)
    {
    case OPER1:
    {
        DbgPrint("IrpDeviceControlProc -> OPER1...\n");
        pIrp->IoStatus.Information = 0;
        status = STATUS_SUCCESS;
        break;
    }
    case OPER2:
    {
        DbgPrint("IrpDeviceControlProc -> OPER2 输入字节数: %d\n", uInLength);
        DbgPrint("IrpDeviceControlProc -> OPER2 输出字节数: %d\n", uOutLength);
        // 读取缓冲区
        memcpy(&uRead, pIoBuffer, 4);

        // 杀进程
        KillProcessByPid(g_driver, uRead);

        DbgPrint("IrpDeviceControlProc -> OPER2 uRead: %x\n", uRead);
        // 写入缓冲区
        memcpy(pIoBuffer, &uWrite, 4);
        // 设置状态
        pIrp->IoStatus.Information = 2; // 返回两字节
        status = STATUS_SUCCESS;
        break;
    }
    }

    // 返回状态如果不设置，Ring3返回值是失败
    pIrp->IoStatus.Status = status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
