
# ArchLab

本节来处理

## 前言

在 `make` 构建的时候会出现一些问题 ,比如 flex bison 未找到 tcl 库没找到这种, `.usr/bin/ld: cannot find -lfl` 等,运行如下指令安装即可

```bash
sudo apt install tcl tcl-dev tk tk-dev
sudo apt install flex bison
```

编译还可能会遇到其他问题, 比如 ld 的时候multi define, 这个主要是因为弱符号强符号的问题, gcc-10 之后的默认选项变成了 -fcommon 所以如果你是 gcc-10 及更高版本会报错, 参考[fail to compile the y86 simulatur csapp](https://stackoverflow.com/questions/63152352/fail-to-compile-the-y86-simulatur-csapp) 要么将所有 Makefile 里面的 CFLAGS LCFLAGS 加上 -fcommon

不过我建议下一个低版本的 gcc-9 然后切为默认, 读者可参考[gcc版本切换](https://luzhixing12345.github.io/2023/03/14/环境配置/gcc版本切换/), 再编译就没问题了