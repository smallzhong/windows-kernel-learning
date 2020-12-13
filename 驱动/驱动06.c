#include "StdAfx.h"
#include <stdio.h>
#include <windows.h>

#define DRIVER_NAME L"Project1"
#define DRIVER_PATH L"C:\\Documents and Settings\\Administrator\\桌面\\Project1.sys"
// #define DRIVER_LINK L"\\\\.\\MsgBoxAWatcherDriverLnk"

BOOL LoadDriver(PCWSTR lpszDriverName, PCWSTR lpszDriverPath);

int main()
{
	LoadDriver(DRIVER_NAME, DRIVER_PATH);
	getchar();

	return 0;
}

// 加载驱动
BOOL LoadDriver(PCWSTR lpszDriverName, PCWSTR lpszDriverPath)
{
	// 获取驱动完整路径
	WCHAR szDriverFullPath[MAX_PATH] = {0};
	GetFullPathNameW(lpszDriverPath, MAX_PATH, szDriverFullPath, NULL);
	//printf("%s\n", szDriverFullPath);
	// 打开服务控制管理器
	SC_HANDLE hServiceMgr = NULL; // SCM管理器句柄
	hServiceMgr = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (NULL == hServiceMgr)
	{
		printf("OpenSCManagerW 失败, %d\n", GetLastError());
		return FALSE;
	}
	//printf("打开服务控制管理器成功.\n");
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
	//printf("创建驱动服务成功.\n");
	// 驱动服务已经创建，打开服务
	hServiceDDK = OpenServiceW(hServiceMgr, lpszDriverName, SERVICE_ALL_ACCESS);
	if (!StartService(hServiceDDK, NULL, NULL))
	{
		DWORD dwErr = GetLastError();
		if (dwErr != ERROR_SERVICE_ALREADY_RUNNING)
		{
			printf("运行驱动服务失败, %d\n", dwErr);
			return FALSE;
		}
	}
	//printf("运行驱动服务成功.\n");
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
