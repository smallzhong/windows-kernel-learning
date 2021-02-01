#include <ntddk.h>

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path);
VOID DriverUnload(PDRIVER_OBJECT driver);

NTSTATUS DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING reg_path)
{
    PEPROCESS pEprocess, pCurProcess;
    PCHAR ImageFileName;

    __asm {
		mov eax, fs: [0x124]
		mov eax, [eax + 0x220]
		mov pEprocess, eax
    }

    pCurProcess = pEprocess;

    do
    {
        ImageFileName = (PCHAR)((ULONG)pCurProcess + 0x174);
        DbgPrint("%s，strcmp(ImageFileName, \"notepad.exe\") == %d\r\n", ImageFileName, strcmp(ImageFileName, "notepad.exe"));
        if (strcmp(ImageFileName, "notepad.exe") == 0)
        {
            PLIST_ENTRY pcur, pf, pb;
            pcur = (PLIST_ENTRY)((ULONG)pCurProcess + 0x88);
            pf = (PLIST_ENTRY)((ULONG)(*(PULONG)((ULONG)pCurProcess + 0x88)));
            pb = (PLIST_ENTRY)((ULONG)(*(PULONG)((ULONG)pCurProcess + 0x88 + 0x4)));
            pb->Flink = pcur->Flink;
            pf->Blink = pcur->Blink;
        }

        pCurProcess = (PEPROCESS)(*((PULONG)((ULONG)pCurProcess + 0x88)) - 0x88);
    } while (pCurProcess != pEprocess); // 循环链表

    driver->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}

VOID DriverUnload(PDRIVER_OBJECT driver)
{
    DbgPrint("驱动卸载成功\n");
}
