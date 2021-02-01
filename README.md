# windows-kernel-learning

## 逆向

+ 这个文件夹里面放了 `kernel32` 、 `ntoskrnl`  、 `ntdll` 的 `idb` ，里面写有一些注释， `PDBDownloader.exe` 是用来下载对应 `pdb` 文件的。如果用ida打开的时候提示需要导入可以使用这个程序来下载相应的 `pdb` 文件然后导入ida，具体做法可以参考这个链接https://www.cnblogs.com/onetrainee/p/11767895.html

## 驱动

### 驱动01

> 申请一块内存，并在内存中存储GDT、IDT的所有数据。然后在debugview中显示出来，最后释放内存

### 驱动02

>**编写代码，实现如下功能：**
>
><1> 初始化一个字符串
>
><2> 拷贝一个字符串
>
><3> 比较两个字符串是否相等
>
><4> ANSI_STRING与UNICODE_STRING字符串相互转换

### 驱动03

>遍历内核模块链表的代码

### 驱动04

>驱动在内存中通过特征码找未导出函数 `PspTerminateProcess` 的地址，0环和3环通信发送需要删除的进程PID，然后在驱动层调用 `PspTerminateProcess` 强杀进程。

### 驱动05（shellcode）

> 通过 `TEB` 和 `PEB` 找到 `LDR` 然后找到 `kernel32.dll` 的地址然后通过导出表找到 `GetProcAddress` 的地址。
>
> 这道题目要注意思路是找到 `kernel32.dll` 之后找其 **导出表** 中的内容找到 `GetProcAddress` 然后找到 `Loadlibrary` 的地址，这样就可以手到擒来使用各种系统函数了。注意写 `shellcode` 的时候不要调用任何函数，字符串要写成局部变量，不要使用任何的全局变量。
>
> 注：代码是能跑起来的，能弹出messagebox，但是我太懒了没有删掉里面的各种 `printf` `getchar` ，因此抠出来之后并不能用。

另附用来测试 `shellcode` 能不能跑的代码，使用的时候自行设置 `szFilePath` 然后把抠出来的 `shellcode` 写道相应的文件中去。

```cpp
#include <windows.h>
size_t GetSize(char * szFilePath)
{
	size_t size;
	FILE* f = fopen(szFilePath, "rb");
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	rewind(f);
	fclose(f);
	return size;
}
unsigned char* ReadBinaryFile(char *szFilePath, size_t *size)
{
	unsigned char *p = NULL;
	FILE* f = NULL;
	size_t res = 0;
	*size = GetSize(szFilePath);
	if (*size == 0) return NULL;		
	f = fopen(szFilePath, "rb");
	if (f == NULL)
	{
		printf("Binary file does not exists!\n");
		return 0;
	}
	p = new unsigned char[*size];
	rewind(f);
	res = fread(p, sizeof(unsigned char), *size, f);
	fclose(f);
	if (res == 0)
	{
		delete[] p;
		return NULL;
	}
	return p;
}
int main(int argc, char* argv[])
{
	char *szFilePath="c:\\test\\shellcode.bin";
	unsigned char *BinData = NULL;
	size_t size = 0;	
	BinData = ReadBinaryFile(szFilePath, &size);
	void *sc = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (sc == NULL)	
		return 0;	
	memcpy(sc, BinData, size);
	(*(int(*)()) sc)();	
	return 0;
}
```

### 驱动06

+ 通过3环的API加载驱动，注意如果用这种方法加载驱动，驱动必须设置

  ```c
  pDriver->MajorFunction[IRP_MJ_CREATE] = IrpCreateProc;
  ```

  否则Ring3调用CreateFile会返回1。

+ 都是API用法，貌似没啥好学的，直接抄了一份代码。



## 系统调用

### 重写ReadProcessMemory（sysenter）

+ 这里VC6有个bug，内联汇编的标记在使用的时候（比如push 一个标记地址）这样会把这个地址指向的值压进去。。mov也一样。没想到好办法（可以生成二进制之后再patch一下？），这里直接硬编码写进去了，VS没有这个问题。

### 重写ReadProcessMemory（中断门）

+ 比 `sysenter` 简单很多，直接复制系统的函数即可。

### SSDThook保护notepad

+ 通过 `ObReferenceObjectByHandle` 函数使用三环传过来的句柄获得目标进程的 `EPROCESS` ，然后比较查看目标进程的进程名是不是 `notepad.exe` ，如果是的话查看传进来的句柄是不是 `currentprocess` (-1) 

  ```c
  if (hProcess == (HANDLE)0xFFFFFFFF)
  ```

  如果是的话说明是自身通过关闭按钮关闭，正常执行。否则返回 `STATUS_ACCESS_DENIED` 。

  

### 系统调用阶段测试——基于 SSDT HOOK 的 FindWindowA 监视器

+ `KeServiceDescriptorTableShadow` 未导出，可以通过特征码搜索（？交叉引用），或者更简单的方法是 `KeServiceDescriptorTableShadow - 0x40` 
+ 要通过devicecontrol来调用，因为调用的进程必须使用过gdi，否则没有挂页会导致蓝屏
+ 懒了，和之前的好像区别不大，不用通过特征码搜索就没多难，咕了咕了

## 进程与线程

### EPROCESS断链隐藏进程

+ 当线程进入**0环**时，**FS:[0]指向`KPCR`**(**3环**时**FS:[0] -> TEB**)

+ 在零环获取当前进程的 `EPROCESS` 的方法如下

  ```c
  __asm
  {
      mov eax, fs:[0x124];
      mov eax, [eax + 0x220];
      mov pEprocess, eax;
  }
  ```

  首先通过 `fs:[0x124]` 找到指向 `KTHREAD` 的指针

  ![image-20210201174540485](https://cdn.jsdelivr.net/gh/smallzhong/picgo-pic-bed/image-20210201174700221.png)

  因为 `KTHREAD` 是 `ETHERAD`的第一个元素，而 `ETHREAD` 第 `0x220` 位是指向其所属进程的 `EPROCESS` 结构体的指针

  ![image-20210201174700221](https://cdn.jsdelivr.net/gh/smallzhong/picgo-pic-bed/image-20210201174700221.png)

  因此在获取指向 `KTHREAD` 的指针后 `[eax+0x220]` 即为当前进程 `EPROCESS` 的指针。