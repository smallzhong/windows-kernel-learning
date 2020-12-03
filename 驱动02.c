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

	ANSI_STRING ansi_src, ansi_des;
	UNICODE_STRING uni_src, uni_des;

	RtlInitAnsiString(&ansi_src, "smallzhong");
	RtlInitUnicodeString(&uni_src, L"smallzhong");

	DbgPrint("%s %d %d\n", ansi_src.Buffer, ansi_src.Length, ansi_src.MaximumLength);
	// wchar类型要用%ls来输出
	DbgPrint("%ls %d %d\n", uni_src.Buffer, uni_src.Length, uni_src.MaximumLength);

	RtlCopyString(&ansi_des, &ansi_src);
	if (RtlCompareString(&ansi_des, &ansi_src, TRUE) == 0)
		DbgPrint("两个字符串相等！\n");
	else
		DbgPrint("两个字符串不相等\n");

	// 第三个参数？？？
	RtlAnsiStringToUnicodeString(&uni_des, &ansi_src, TRUE);

	// 第三个参数好像是忽略大小写的意思
	if (RtlCompareString(&uni_des, &uni_src, TRUE) == 0)
		DbgPrint("两个字符串相等！\n");
	else
		DbgPrint("两个字符串不相等\n");

	return STATUS_SUCCESS;
}