# ArchLab

## 前言

在 `make` 构建的时候会出现一些问题,比如 flex bison 未找到, `.usr/bin/ld: cannot find -lfl` 等,运行如下指令安装即可

```bash
sudo apt install -y flex bison
sudo apt install -y tk-dev
```

笔者学习CSAPP课程跳过了第四章处理器结构,将其放在下学期CPU流水线设计一起学习的. 这里