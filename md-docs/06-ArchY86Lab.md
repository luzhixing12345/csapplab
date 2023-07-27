
# ArchY86Lab

相关的前置知识点在 05-ArchLab 中已经介绍过了, 本节着重介绍实验解答部分

## 前言

在 `make` 构建的时候会出现一些问题 ,比如 flex bison 未找到 tcl 库没找到这种, `.usr/bin/ld: cannot find -lfl` 等,运行如下指令安装即可

```bash
sudo apt install tcl tcl-dev tk tk-dev
sudo apt install flex bison
```

编译还可能会遇到其他问题, 比如 ld 的时候multi define, 这个主要是因为弱符号强符号的问题, gcc-10 之后的默认选项变成了 -fcommon 所以如果你是 gcc-10 及更高版本会报错, 参考[fail to compile the y86 simulatur csapp](https://stackoverflow.com/questions/63152352/fail-to-compile-the-y86-simulatur-csapp) 要么将所有 Makefile 里面的 CFLAGS LCFLAGS 加上 -fcommon

不过我建议下一个低版本的 gcc-9 然后切为默认, 读者可参考[gcc版本切换](https://luzhixing12345.github.io/2023/03/14/环境配置/gcc版本切换/), 再编译就没问题了

## PartA

第一部分内容在 sim/misc 下完成

### sum.ys

第一个任务的要求是编写 Y86-64 的汇编完成 sum_list 函数, 该函数的作用是对链表的值求和, C 代码如下

```c
typedef struct ELE {
    long val;
    struct ELE *next;
} * list_ptr;

/* sum_list - Sum the elements of a linked list */
long sum_list(list_ptr ls) {
    long val = 0;
    while (ls) {
        val += ls->val;
        ls = ls->next;
    }
    return val;
}
```

我们新建一个文件: misc/sum.ys

```x86asm
# Execution begins at address 0
        .pos 0
        irmovq stack, %rsp      # Set up stack pointer
        call main               # Execute main program
        halt                    # Terminate program

# Sample linked list
        .align 8
ele1:
        .quad 0x00a
        .quad ele2
ele2:
        .quad 0x0b0
        .quad ele3
ele3:
        .quad 0xc00
        .quad 0

main:
        irmovq ele1,%rdi
        call sum_list
        ret

# long sum_list(list_ptr ls)
# start in %rdi
sum_list:
        irmovq $0, %rax
        jmp test

loop:
        mrmovq (%rdi), %rsi
        addq %rsi, %rax
        mrmovq 8(%rdi), %rdi

test:
        andq %rdi, %rdi
        jne loop
        ret

# Stack starts here and grows to lower addresses
        .pos 0x200
stack:

```

利用编译好的 yas 将 sum.ys 翻译为字节码 sum.yo, 再利用 yis 执行, 最后观察到结果 %rax = 0xcba = 0x00a + 0x0b0 + 0xc00, 刚好是三个结构体的value的和,说明正确

```bash
(base) kamilu@LZX:~/csapplab/05_arch_lab/sim/misc$ ./yas sum.ys
(base) kamilu@LZX:~/csapplab/05_arch_lab/sim/misc$ ./yis sum.yo
Stopped in 26 steps at PC = 0x13.  Status 'HLT', CC Z=1 S=0 O=0
Changes to registers:
%rax:   0x0000000000000000      0x0000000000000cba
%rsp:   0x0000000000000000      0x0000000000000200
%rsi:   0x0000000000000000      0x0000000000000c00

Changes to memory:
0x01f0: 0x0000000000000000      0x000000000000005b
0x01f8: 0x0000000000000000      0x0000000000000013
```

> 汇编代码结尾需要有一行换行, 不然会yas的时候产生警告: `Missing end-of-line on final line`

### rsum.ys

第二个任务是实现递归调用

```c
/* rsum_list - Recursive version of sum_list */
long rsum_list(list_ptr ls) {
    if (!ls)
        return 0;
    else {
        long val = ls->val;
        long rest = rsum_list(ls->next);
        return val + rest;
    }
}
```

```x86asm
# Execution begins at address 0
        .pos 0
        irmovq stack, %rsp      # Set up stack pointer
        call main               # Execute main program
        halt                    # Terminate program

# Sample linked list
        .align 8
ele1:
        .quad 0x00a
        .quad ele2
ele2:
        .quad 0x0b0
        .quad ele3
ele3:
        .quad 0xc00
        .quad 0

main:
        irmovq ele1,%rdi
        call rsum_list
        ret

# long rsum_list(list_ptr ls)
# start in %rdi
rsum_list:
        andq %rdi, %rdi
        je return               # if(!ls)
        mrmovq (%rdi), %rbx     # val = ls->val
        mrmovq 8(%rdi), %rdi    # ls = ls->next
        pushq %rbx
        call rsum_list          # rsum_list(ls->next)
        popq %rbx
        addq %rbx, %rax         # val + rest
        ret
return:
        irmovq $0, %rax
        ret


# Stack starts here and grows to lower addresses
        .pos 0x200
stack: 

```

完成递归的思路是先取出 ls->val 放入 %rbx, 然后利用栈保存值, 先push入栈然后在递归调用的 rsum_list 返回之后从栈中取出 val 求和再返回

```x86asm
        pushq %rbx
        call rsum_list          # rsum_list(ls->next)
        popq %rbx
        addq %rbx, %rax         # val + rest
```

```bash
(base) kamilu@LZX:~/csapplab/05_arch_lab/sim/misc$ ./yas rsum.ys
(base) kamilu@LZX:~/csapplab/05_arch_lab/sim/misc$ ./yis rsum.yo
Stopped in 37 steps at PC = 0x13.  Status 'HLT', CC Z=0 S=0 O=0
Changes to registers:
%rax:   0x0000000000000000      0x0000000000000cba
%rbx:   0x0000000000000000      0x000000000000000a
%rsp:   0x0000000000000000      0x0000000000000200

Changes to memory:
0x01c0: 0x0000000000000000      0x0000000000000086
0x01c8: 0x0000000000000000      0x0000000000000c00
0x01d0: 0x0000000000000000      0x0000000000000086
0x01d8: 0x0000000000000000      0x00000000000000b0
0x01e0: 0x0000000000000000      0x0000000000000086
0x01e8: 0x0000000000000000      0x000000000000000a
0x01f0: 0x0000000000000000      0x000000000000005b
0x01f8: 0x0000000000000000      0x0000000000000013
```

运行可以看到这次修改的内存范围变多了, 因为使用push和call都会入栈修改栈空间附近的内存值, 其中高地址的 `0x13` `0x5b` 分别是开始的两次call压栈的返回地址

![20230721101114](https://raw.githubusercontent.com/learner-lu/picbed/master/20230721101114.png)

然后的 0x00a 是当前结构体的 val 值, 0x86 是 rsum_list 的返回地址

![20230721100944](https://raw.githubusercontent.com/learner-lu/picbed/master/20230721100944.png)

最后结果 `0xcba` 也如预期所料

###

```c
/* copy_block - Copy src to dest and return xor checksum of src */
long copy_block(long *src, long *dest, long len) {
    long result = 0;
    while (len > 0) {
        long val = *src++;
        *dest++ = val;
        result ^= val;
        len--;
    }
    return result;
}
```

```x86asm
# Execution begins at address 0
        .pos 0
        irmovq stack, %rsp      # Set up stack pointer
        call main               # Execute main program
        halt                    # Terminate program

        .align 8
# Source block
src:
        .quad 0x00a
        .quad 0x0b0
        .quad 0xc00
# Destination block
dest:
        .quad 0x111
        .quad 0x222
        .quad 0x333

main:
        irmovq src, %rdi
        irmovq dest, %rsi
        irmovq $3, %rdx 
        call copy_block
        ret

# long copy_block(long *src, long *dest, long len)
# start in %rdi, %rsi, %rdx
copy_block:
        irmovq $0, %rax
        irmovq $8, %r8
        irmovq $1, %r9
        andq %rdx, %rdx
        jne loop
        ret

loop:
        mrmovq (%rdi), %r10
        addq %r8, %rsi
        rmmovq %r10, (%rsi)
        addq %r8, %rdi
        xorq %r10, %rax
        subq %r9, %rdx
        jne loop
        ret


# Stack starts here and grows to lower addresses
        .pos 0x200
stack: 

```

由于 Y86-64 的 addq subq 只支持寄存器操作, 所以我们需要使用 r8 r9 把需要使用的常数 `long* ++` 的 8 和 `len--` 的 1 保存起来

```bash
(base) kamilu@LZX:~/csapplab/05_arch_lab/sim/misc$ ./yas copy_block.ys
(base) kamilu@LZX:~/csapplab/05_arch_lab/sim/misc$ ./yis copy_block.yo
Stopped in 35 steps at PC = 0x13.  Status 'HLT', CC Z=1 S=0 O=0
Changes to registers:
%rax:   0x0000000000000000      0x0000000000000cba
%rsp:   0x0000000000000000      0x0000000000000200
%rsi:   0x0000000000000000      0x0000000000000048
%rdi:   0x0000000000000000      0x0000000000000030
%r8:    0x0000000000000000      0x0000000000000008
%r9:    0x0000000000000000      0x0000000000000001
%r10:   0x0000000000000000      0x0000000000000c00

Changes to memory:
0x0038: 0x0000000000000222      0x000000000000000a
0x0040: 0x0000000000000333      0x00000000000000b0
0x0048: 0x000000000018f730      0x0000000000000c00
0x01f0: 0x0000000000000000      0x000000000000006f
0x01f8: 0x0000000000000000      0x0000000000000013
```

结果显示 result 的值是 src 的中的异或结果 `0xcba`, 并且原先 dest 的地址中 0x38 0x40 0x48 也被修改为了 src 中对应的值

## PartB

第二部分在 sim/seq 下完成, 在原有指令集中添加一个iaddq指令, 实现立即数+寄存器

![20230721134051](https://raw.githubusercontent.com/learner-lu/picbed/master/20230721134051.png)

![20230721133649](https://raw.githubusercontent.com/learner-lu/picbed/master/20230721133649.png)

参考其他指令的六个阶段, 我们可以将 iaddq 指令对应的部分仿写出来, 如下所示

|阶段|iaddq V, rB|
|:--:|:--:|
|取指|icode:ifun ⟵ M1[PC] <br/> rA:rB ⟵ M1[PC+1] <br/> valC ⟵ M8[PC+2] <br/> valP ⟵ PC+10|
|译码|valB ⟵ R[rB]|
|执行|valE ⟵ valB + valC <br/> Set CC|
|访存||
|写回|R[rB] ⟵ valE|
|更新PC|PC ⟵ valP|

> 其中 iaddq 涉及到立即数与寄存器值相加, 与 OPq 类似还需要设置 CC

接下来完成 seq-full.hcl 中的部分, 这部分照葫芦画瓢即可, 源文件中的变量名和注释已经给了很好的参考, 下面简单解释一下

42 行定义了 `wordsig IIADDQ	'I_IADDQ'` 为本次需要添加的 I_IADDQ 指令名为 `IIADDQ`

Fetch 阶段中 instr_valid 表示指令是否合法, need_regids 是否需要寄存器, need_valC 是否需要立即数. iaddq 都需要所以都添加进去

![20230727154034](https://raw.githubusercontent.com/learner-lu/picbed/master/20230727154034.png)

Decode 阶段 srcB 确定将哪个寄存器取值并放入 srcB, dstE 确定将计算得到的 valE 写回哪个寄存器

> 这里的 IRRMOVQ 对应的 & Cnd 没有理解什么意思...

![20230727154329](https://raw.githubusercontent.com/learner-lu/picbed/master/20230727154329.png)

Execute 阶段为 `valE ⟵ valB + valC`, 所以加入对应的位置, 以及需要设置 CC 的 set_cc

![20230727154738](https://raw.githubusercontent.com/learner-lu/picbed/master/20230727154738.png)

最后没有访存以及不需要修改更新PC的部分

使用 `make VERSION=full` 执行编译会报错, 解决办法为修改 Makefile

![20230727150404](https://raw.githubusercontent.com/learner-lu/picbed/master/20230727150404.png)

```Makefile
# 20 行改为
TKINC=-isystem /usr/include/tcl8.6
# 26 行改为
CFLAGS=-Wall -O2 -DUSE_INTERP_RESULT
```

注释掉 seq/ssim.c 的 844 845 行以及 pipe/psim.c 806、807 行

```c
/* Hack for SunOS */
// extern int matherr();
// int *tclDummyMathPtr = (int *) matherr;
```

接下来就没有问题了, 依次执行如下代码编译测试

```bash
make VERSION=full
(cd ../y86-code; make testssim)
(cd ../ptest; make SIM=../seq/ssim)
(cd ../ptest; make SIM=../seq/ssim TFLAGS=-i)
```

![20230727151230](https://raw.githubusercontent.com/learner-lu/picbed/master/20230727151230.png)

## 参考

- [deconx Lab04-Architecture_Lab](https://deconx.cn/docs/system/CSAPP/Lab04-Architecture_Lab)
- [【深入理解计算机系统】CSAPP-实验四:ArchLab全网最详细](https://blog.csdn.net/qq_42234461/article/details/108720264)
- [CSAPP : Arch Lab 解题报告](https://zhuanlan.zhihu.com/p/36793761)
- [CSAPP:ArchLab](https://zhuanlan.zhihu.com/p/559593472)
- [CSAPP-archlab](https://mcginn7.github.io/2020/02/21/CSAPP-archlab/)
- [littlecsd csapp-Archlab](https://littlecsd.net/2019/01/18/csapp-Archlab/)
- [gitbook hansimov](https://hansimov.gitbook.io/csapp/labs/arch-lab)