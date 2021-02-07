// 03通信非常规方式.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------



BOOL LoadDriver(PCWSTR lpszDriverName, PCWSTR lpszDriverPath);
void UnLoadDriver(PCWSTR lpszDriverName);

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

// typedef BOOL (__stdcall *ReadProcessMemory)(  LPVOID in_data,              // 传进去的数据
// 					   LPVOID out_data,        // 传出来的数据缓冲区
// 					   DWORD in_size,              // 穿进去的数据的大小 
// 					   SIZE_T nSize,                 // 操作码，正常调用readprocessmemory的数字一定是整数，因此这里要设置为负数
//   SIZE_T * lpNumberOfBytesRead  // number of bytes read
//  );
//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

#define DRIVER_NAME L"Project1"
#define DRIVER_PATH L"Project1.sys"

#define OP_TEST -50

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

int main()
{
	// HbgCommunication = (HBGCOMMUNICATION)GetProcAddress(LoadLibraryA("ntdll.dll"),"ZwQuerySystemInformation");
// 	if (HbgCommunication == NULL) 
// 	{
// 		printf("获取函数地址失败. %d\n",GetLastError());
// 		getchar();
// 		return 1;
// 	}
/*
	LoadDriver(DRIVER_NAME,DRIVER_PATH);
	char indata[10] = {0};
	char OutData[10] = {0};
	memset(OutData, 0, sizeof OutData);
	memset(indata, 0, sizeof indata);
	strcpy(indata, "Hello w");
	
	ULONG InOutDataLength;
	ReadProcessMemory("Hello", OutData , (LPVOID)(sizeof indata), OP_TEST, NULL); // 最后一个参数不传了
	printf("%d %s\n", InOutDataLength, OutData);
	UnLoadDriver(DRIVER_NAME);
*/
	ReadProcessMemory((PVOID)0x12345, (PVOID)0x12345, (PVOID)0x12345, (PVOID)0x12345, 0x114514, 0);
	getchar();
	return 0;
}

BOOL LoadDriver(PCWSTR lpszDriverName, PCWSTR lpszDriverPath)
{
	// 获取驱动完整路径
	WCHAR szDriverFullPath[MAX_PATH] = { 0 };
	GetFullPathNameW(lpszDriverPath,MAX_PATH,szDriverFullPath,NULL);
	//printf("%s\n", szDriverFullPath);
	// 打开服务控制管理器
	SC_HANDLE hServiceMgr = NULL; // SCM管理器句柄	
	hServiceMgr = OpenSCManagerW(NULL,NULL,SC_MANAGER_ALL_ACCESS);
	if (NULL == hServiceMgr)
	{
		printf("OpenSCManagerW 失败, %d\n", GetLastError());
		return FALSE;
	}
	printf("打开服务控制管理器成功.\n");
	// 创建驱动服务
	SC_HANDLE hServiceDDK = NULL; // NT驱动程序服务句柄
	hServiceDDK = CreateServiceW(
		hServiceMgr,
		lpszDriverName,
		lpszDriverName,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,
		szDriverFullPath,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL);
	if (NULL == hServiceDDK)
	{
		DWORD dwErr = GetLastError();
		if (dwErr != ERROR_IO_PENDING && dwErr != ERROR_SERVICE_EXISTS)
		{
			printf("创建驱动服务失败, %d\n", dwErr);
			return FALSE;
		}
	}
	printf("创建驱动服务成功.\n");
	// 驱动服务已经创建，打开服务
	hServiceDDK = OpenServiceW(hServiceMgr,lpszDriverName,SERVICE_ALL_ACCESS);
	if (!StartService(hServiceDDK, NULL, NULL))
	{
		DWORD dwErr = GetLastError();
		if (dwErr != ERROR_SERVICE_ALREADY_RUNNING)
		{
			printf("运行驱动服务失败, %d\n", dwErr);
			return FALSE;
		}
	}
	printf("运行驱动服务成功.\n");
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
	return TRUE;
}

void UnLoadDriver(PCWSTR lpszDriverName)
{
	SC_HANDLE hServiceMgr = OpenSCManagerW(0,0,SC_MANAGER_ALL_ACCESS);
	SC_HANDLE hServiceDDK = OpenServiceW(hServiceMgr,lpszDriverName,SERVICE_ALL_ACCESS);
	SERVICE_STATUS SvrStatus;
	ControlService(hServiceDDK,SERVICE_CONTROL_STOP,&SvrStatus);
	DeleteService(hServiceDDK);
	if (hServiceDDK)
	{
		CloseServiceHandle(hServiceDDK);
	}
	if (hServiceMgr)
	{
		CloseServiceHandle(hServiceMgr);
	}
}
