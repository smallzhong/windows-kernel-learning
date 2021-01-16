# 👴考完期末了哈哈哈哈哈哈哈哈

# 天空一声巨响，肝肝人闪亮登场！！！

# windows-kernel-learning

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

