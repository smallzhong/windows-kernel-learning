#include "StdAfx.h"
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>

using namespace std;

BOOL __stdcall My_ReadProcessMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead);
BOOL __stdcall My_ReadProcessMemory_INT(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead);

#define EXIT_ERROR(x)                                 \
    do                                                \
{                                                 \
	cout << "error in line " << __LINE__ << endl; \
	printf("errcode = %d\n", GetLastError());     \
	cout << x;                                    \
	system("pause");                              \
	exit(EXIT_FAILURE);                           \
    } while (0)

int main()
{
	int pid = 0;
    cout << "请输入要读取的进程的PID：";
    cin >> pid;
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (hProcess == NULL)
		EXIT_ERROR("hProcess == NULL!");

	WORD t;
	DWORD dwSizeRead;

	ReadProcessMemory(hProcess, (LPCVOID)0x00400000,
		&t, sizeof WORD, &dwSizeRead);
	cout << hex << t << " " << dwSizeRead << endl;
	getchar();
	system("pause");

	My_ReadProcessMemory_INT(hProcess, (LPCVOID)0x00400000,
		&t, sizeof WORD, &dwSizeRead);
	
	cout << hex << t << " " << dwSizeRead;
	
	getchar();
	system("pause");

	return 0;
}

BOOL WINAPI My_ReadProcessMemory_INT(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead)
{
	DWORD NtStatus;
	__asm
	{
		mov eax, 0xBA;
		lea     edx, hProcess // edx里面存储最后入栈的参数	
		int     2Eh    
		mov NtStatus, eax
	}
	*lpNumberOfBytesRead = nSize;

	if (NtStatus) return NtStatus;
	else return 0;
}

BOOL __stdcall My_ReadProcessMemory(HANDLE hProcess, LPCVOID lpBaseAddress, LPVOID lpBuffer, DWORD nSize, LPDWORD lpNumberOfBytesRead)
{
	DWORD NtStatus;
	
	__asm
	{
		lea eax, [nSize]
		push eax // 将nsize的地址入栈，这里可以再用一个变量，不过直接自产自销传nsize进去然后用nsize接收OUT会方便一些 
		// 这里不直接push [nSize]是为了之后eax里面直接留着nsize的地址，更容易判断nsize的值，省两行代码
		push nSize // nsize值入栈
		push lpBuffer // buffer入栈
		push lpBaseAddress // lpBaseAddress入栈
		push hProcess // hProcess入栈
		
		// NT
		sub esp, 0x04; // 模拟 ReadProcessMemory 里的 CALL NtReadVirtualMemory
		mov     eax, 0BAh       // NtReadVirtualMemory

		// push NtReadRet // 模拟 NtReadVirtualMemory 函数里的 CALL [0x7FFE0300]
		// 傻逼VC6这里不能压标记的地址进栈，只好硬编码了（不要打我（（（VS没有这个问题
		push 0x00401EBC
		// kifastcall
		mov		edx, esp
		_emit 0x0f
		_emit 0x34 // 没有sysenter，要硬编码0x0f34
		//

NtReadRet:
		add esp, 0x18; // 模拟 NtReadVirtualMemory 返回到 ReadProcessMemory 时的 RETN 0x14
		//

		mov NtStatus, eax;
	}

	printf("nsize = %d, status = %d\n", nSize, NtStatus);

	return 0;
}