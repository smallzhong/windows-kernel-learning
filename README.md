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