// IRPTest_R3.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <winioctl.h>


#define SYMBOLICLINK_NAME L"\\\\.\\HbgDevLnk"
#define OPER1 CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define OPER2 CTL_CODE(FILE_DEVICE_UNKNOWN,0x900,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IN_BUFFER_MAXLENGTH 4
#define OUT_BUFFER_MAXLENGTH 4

int main()
{
    // 获取设备句柄
    HANDLE hDevice =
        CreateFileW(SYMBOLICLINK_NAME, GENERIC_READ | GENERIC_WRITE, 0, 0,
                    OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    DWORD dwError = GetLastError();
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("获取设备句柄失败 %d.\n",
               dwError);  // 如果返回1，请在驱动中指定 IRP_MJ_CREATE 处理函数
        getchar();
        return 1;
    }
    else
    {
        printf("获取设备句柄成功.\n");
    }
    // 测试通信
    DWORD dwInBuffer = 0x510;
    DWORD dwOutBuffer = 0xFFFFFFFF;
    DWORD dwOut;
    DeviceIoControl(hDevice, OPER2, &dwInBuffer, 
					IN_BUFFER_MAXLENGTH,
                    &dwOutBuffer, 
					OUT_BUFFER_MAXLENGTH, &dwOut, NULL);
    printf("dwOutBuffer: %08X dwOut: %08X\n", dwOutBuffer, dwOut);
    // 关闭设备
    CloseHandle(hDevice);
    getchar();
    return 0;
}
