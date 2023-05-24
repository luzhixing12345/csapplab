# PerformanceLab

## 前言

performance lab 实验用于理解一些程序中的优化手段, 但是本身这个实验设计的比较粗糙, 甚至没有一个很好的评判标准, 程序使用的计时器甚至每次运行得分都会有所波动...

看了些其他的文章也都说这个实验已经被Cachelab取代了,不过这个实验本身并不算很难,简单的花一点时间就可以搞定

> 做下来感觉价值确实不是很高

## 实验背景

实验pdf里写的比较清晰了,需要完成kernels.c里面的两个函数rotate和smooth,是两个图像处理里面十分常用的函数

图片是一个NxN的矩阵,其中rotate函数就是将图片逆时针旋转九十度

![20230203204608](https://raw.githubusercontent.com/learner-lu/picbed/master/20230203204608.png)

smooth函数就是计算每个像素块3x3范围内的平均值

![20230203204709](https://raw.githubusercontent.com/learner-lu/picbed/master/20230203204709.png)

在kernels.c中给出了两个naive的函数实现,但明显存在不足之处以及可以优化的地方

本身在优化里面课本上给出了几种常见的程序优化方式

- 代码移动(code motion):例如将for循环中的a.size()提取出来
- 循环展开(loop unrolling): 减少了索引计算的次数(i++)和条件分支的判断次数(i<100),同时在cache映射方面也会有好处
- 分块(blocking):cachelab的核心部分,充分利用缓存,减少访存次数和miss次数
- 消除内存引用: 例如用一个temp变量计算和,然后再赋值给A[dst]而不是每次都A[dst]+=value
- 累积变量和重新组合，提高指令并行性
- 功能性风格重写条件操作，即用三元运算符

两个函数分别50分满分

## 实验报告

### rotate

很显然src dst两个矩阵的旋转变换分块的方式要好很多,分块之后再循环展开,其他的优化方法确实这里也用不上

参考[分块优化思路](https://segmentfault.com/a/1190000038753527),从8x8分块优化到32x32,然后再展开

```c
void rotate(int dim, pixel* src, pixel* dst)
{
    int i,j;
    int dst_base = (dim-1)*dim;
    dst +=dst_base;
    for(i = 0;i < dim;i += 32){
        for(j = 0;j < dim;j++){
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src +=dim; dst++;
            *dst = *src; src++;
            src -= (dim<<5)-dim;              //src -=31*dim;
            dst -=31+dim;
        }
        dst +=dst_base + dim;
        dst +=32;
        src +=(dim<<5)-dim;                    //src +=31*dim;
    }
}
```

### smooth

对于smooth函数的优化思路就比较明显了,首先每次都调用avg算一个3x3没必要,用一个变量保存移动的时候仅更新6个即可

减少函数调用等等,这里直接给出代码修改

先处理角,处理边的情况,然后处理中心区域,最后再处理边角的情况

```c
void smooth(int dim, pixel* src, pixel* dst)
{
    int i, j;
    int dim0 = dim;
    int dim1 = dim - 1;
    int dim2 = dim - 2;
    pixel *P1, *P2, *P3;
    pixel* dst1;
    P1 = src;
    P2 = P1 + dim0;
    // 左上角像素处理
    dst->red = (P1->red + (P1 + 1)->red + P2->red + (P2 + 1)->red) >> 2;
    dst->green = (P1->green + (P1 + 1)->green + P2->green + (P2 + 1)->green) >> 2;
    dst->blue = (P1->blue + (P1 + 1)->blue + P2->blue + (P2 + 1)->blue) >> 2;
    dst++;
    // 上边界处理
    for (i = 1; i < dim1; i++) {
        dst->red = (P1->red + (P1 + 1)->red + (P1 + 2)->red + P2->red + (P2 + 1)->red + (P2 + 2)->red) / 6;
        dst->green = (P1->green + (P1 + 1)->green + (P1 + 2)->green + P2->green + (P2 + 1)->green + (P2 + 2)->green) / 6;
        dst->blue = (P1->blue + (P1 + 1)->blue + (P1 + 2)->blue + P2->blue + (P2 + 1)->blue + (P2 + 2)->blue) / 6;
        dst++;
        P1++;
        P2++;
    }
    // 右上角像素处理
    dst->red = (P1->red + (P1 + 1)->red + P2->red + (P2 + 1)->red) >> 2;
    dst->green = (P1->green + (P1 + 1)->green + P2->green + (P2 + 1)->green) >> 2;
    dst->blue = (P1->blue + (P1 + 1)->blue + P2->blue + (P2 + 1)->blue) >> 2;
    dst++;
    P1 = src;
    P2 = P1 + dim0;
    P3 = P2 + dim0;
    // 左边界处理
    for (i = 1; i < dim1; i++) {
        dst->red = (P1->red + (P1 + 1)->red + P2->red + (P2 + 1)->red + P3->red + (P3 + 1)->red) / 6;
        dst->green = (P1->green + (P1 + 1)->green + P2->green + (P2 + 1)->green + P3->green + (P3 + 1)->green) / 6;
        dst->blue = (P1->blue + (P1 + 1)->blue + P2->blue + (P2 + 1)->blue + P3->blue + (P3 + 1)->blue) / 6;
        dst++;
        dst1 = dst + 1;
        // 中间主体部分处理
        for (j = 1; j < dim2; j += 2) {
            // 同时处理两个像素
            dst->red = (P1->red + (P1 + 1)->red + (P1 + 2)->red + P2->red + (P2 + 1)->red + (P2 + 2)->red + P3->red + (P3 + 1)->red + (P3 + 2)->red) / 9;
            dst->green = (P1->green + (P1 + 1)->green + (P1 + 2)->green + P2->green + (P2 + 1)->green + (P2 + 2)->green + P3->green + (P3 + 1)->green + (P3 + 2)->green) / 9;
            dst->blue = (P1->blue + (P1 + 1)->blue + (P1 + 2)->blue + P2->blue + (P2 + 1)->blue + (P2 + 2)->blue + P3->blue + (P3 + 1)->blue + (P3 + 2)->blue) / 9;
            dst1->red = ((P1 + 3)->red + (P1 + 1)->red + (P1 + 2)->red + (P2 + 3)->red + (P2 + 1)->red + (P2 + 2)->red + (P3 + 3)->red + (P3 + 1)->red + (P3 + 2)->red) / 9;
            dst1->green = ((P1 + 3)->green + (P1 + 1)->green + (P1 + 2)->green + (P2 + 3)->green + (P2 + 1)->green + (P2 + 2)->green + (P3 + 3)->green + (P3 + 1)->green + (P3 + 2)->green) / 9;
            dst1->blue = ((P1 + 3)->blue + (P1 + 1)->blue + (P1 + 2)->blue + (P2 + 3)->blue + (P2 + 1)->blue + (P2 + 2)->blue + (P3 + 3)->blue + (P3 + 1)->blue + (P3 + 2)->blue) / 9;
            dst += 2;
            dst1 += 2;
            P1 += 2;
            P2 += 2;
            P3 += 2;
        }
        for (; j < dim1; j++) {
            dst->red = (P1->red + (P1 + 1)->red + (P1 + 2)->red + P2->red + (P2 + 1)->red + (P2 + 2)->red + P3->red + (P3 + 1)->red + (P3 + 2)->red) / 9;
            dst->green = (P1->green + (P1 + 1)->green + (P1 + 2)->green + P2->green + (P2 + 1)->green + (P2 + 2)->green + P3->green + (P3 + 1)->green + (P3 + 2)->green) / 9;
            dst->blue = (P1->blue + (P1 + 1)->blue + (P1 + 2)->blue + P2->blue + (P2 + 1)->blue + (P2 + 2)->blue + P3->blue + (P3 + 1)->blue + (P3 + 2)->blue) / 9;
            dst++;
            P1++;
            P2++;
            P3++;
        }
        // 右侧边界处理
        dst->red = (P1->red + (P1 + 1)->red + P2->red + (P2 + 1)->red + P3->red + (P3 + 1)->red) / 6;
        dst->green = (P1->green + (P1 + 1)->green + P2->green + (P2 + 1)->green + P3->green + (P3 + 1)->green) / 6;
        dst->blue = (P1->blue + (P1 + 1)->blue + P2->blue + (P2 + 1)->blue + P3->blue + (P3 + 1)->blue) / 6;
        dst++;
        P1 += 2;
        P2 += 2;
        P3 += 2;
    }
    // 右下角处理
    dst->red = (P1->red + (P1 + 1)->red + P2->red + (P2 + 1)->red) >> 2;
    dst->green = (P1->green + (P1 + 1)->green + P2->green + (P2 + 1)->green) >> 2;
    dst->blue = (P1->blue + (P1 + 1)->blue + P2->blue + (P2 + 1)->blue) >> 2;
    dst++;
    // 下边界处理
    for (i = 1; i < dim1; i++) {
        dst->red = (P1->red + (P1 + 1)->red + (P1 + 2)->red + P2->red + (P2 + 1)->red + (P2 + 2)->red) / 6;
        dst->green = (P1->green + (P1 + 1)->green + (P1 + 2)->green + P2->green + (P2 + 1)->green + (P2 + 2)->green) / 6;
        dst->blue = (P1->blue + (P1 + 1)->blue + (P1 + 2)->blue + P2->blue + (P2 + 1)->blue + (P2 + 2)->blue) / 6;
        dst++;
        P1++;
        P2++;
    }
    // 右下角像素处理
    dst->red = (P1->red + (P1 + 1)->red + P2->red + (P2 + 1)->red) >> 2;
    dst->green = (P1->green + (P1 + 1)->green + P2->green + (P2 + 1)->green) >> 2;
    dst->blue = (P1->blue + (P1 + 1)->blue + P2->blue + (P2 + 1)->blue) >> 2;
}
```

这里其实还可以用三个unsigned int变量记录sum_r,sum_g, sum_b,然后每次移动的时候只更新最右侧三个和减去最左侧三个

> 这里还判断了一下 Teamname 不能是 bovik, 改成 bovik1

```bash
Teamname: bovik1
Member 1: Harry Q. Bovik
Email 1: bovik@nowhere.edu

Rotate: Version = naive_rotate: Naive baseline implementation:
Dim             64      128     256     512     1024    Mean
Your CPEs       1.1     1.2     3.0     4.5     6.1
Baseline CPEs   14.7    40.1    46.4    65.9    94.5
Speedup         13.9    32.7    15.6    14.8    15.6    17.5

Rotate: Version = rotate: Current working version:
Dim             64      128     256     512     1024    Mean
Your CPEs       0.8     0.8     0.8     0.9     1.2
Baseline CPEs   14.7    40.1    46.4    65.9    94.5
Speedup         18.4    48.8    56.2    71.7    78.1    49.0

Smooth: Version = smooth: Current working version:
Dim             32      64      128     256     512     Mean
Your CPEs       7.5     7.7     7.7     7.7     7.8
Baseline CPEs   695.0   698.0   702.0   717.0   722.0
Speedup         93.1    90.3    90.7    92.8    93.1    92.0

Smooth: Version = naive_smooth: Naive baseline implementation:
Dim             32      64      128     256     512     Mean
Your CPEs       23.9    21.8    21.8    22.0    21.9
Baseline CPEs   695.0   698.0   702.0   717.0   722.0
Speedup         29.1    32.0    32.2    32.6    32.9    31.8

Summary of Your Best Scores:
  Rotate: 49.0 (rotate: Current working version)
  Smooth: 92.0 (smooth: Current working version)
```

就马马虎虎吧, 没什么想说的了

## 参考

- [CSAPP | Lab6-Performance Lab 深入解析](https://zhuanlan.zhihu.com/p/488141606)
- [github Performance Lab 笔记](https://github.com/Exely/CSAPP-Labs/blob/master/notes/perflab.md)
- [perf lab](https://segmentfault.com/a/1190000038753527)