# datalab

## 前言

```bash
tar xvf datalab-handout.tar
```

解压缩之后可以看到本次实验使用的是gcc工具,打开Makefile文件可以看到使用了-m32进行了编译

所以如果在编译的时候报错了还需要安装32位的gcc编译器

```bash
sudo apt-get install gcc-multilib
```

位运算运算符优先级

|运算符|优先级|
|:--:|:--:|
|()|1|
|-(单目) ! ~|2|
|* / %|3|
|+ -|4|
|<< >>|5|
|> >= < <=|6|
|== !=|7|
|&|8|
|^|9|
|\||10|

> [完整运算符优先级](https://baike.baidu.com/item/%E8%BF%90%E7%AE%97%E7%AC%A6%E4%BC%98%E5%85%88%E7%BA%A7/4752611)

## 实验解答

```c
int bitXor(int x, int y) {
  return ~(x&y)&~(~x&~y);
}
/*
 * tmin - return minimum two's complement integer
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
    return 1<<31;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
    return !((~(x+1)^x))&!!(x+1);
}
/*
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
    int mask = 0xaa | 0xaa << 8;
    mask = mask | mask << 16;
    return !((x&mask)^mask);
}
/*
 * negate - return -x
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
    return ~x+1;
}
//3
/*
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
    int first_match = !(x>>4^3);
    int second_match = (~0xa+1+(x&0xf))>>31&1;
    return first_match & second_match;
}
/*
 * conditional - same as x ? y : z
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
    int mask = !x + ~0;
    return (mask & y) | (~mask &z);
}
/*
 * isLessOrEqual - if x <= y  then return 1, else return 0
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
    int judge = !((y+~x+1)>>31);
    int same_signal = !((x^y)>>31);
    int x_neg_y_pos = (x>>31)&!(y>>31);
    return x_neg_y_pos | (same_signal & judge);
}
//4
/*
 * logicalNeg - implement the ! operator, using all of
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4
 */
int logicalNeg(int x) {
    int a = (x&(~x+1));
    int b = a + ~0;
    int c = b >> 31;
    return ~c + 1;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
    int b16,b8,b4,b2,b1,b0;
    int flag = x>>31;
    x = (flag&~x) | (~flag&x); //x为非正数则不变 ,x 为负数 则相当于按位取反
    b16 = !!(x>>16) <<4; //如果高16位不为0,则我们让b16=16
    x>>=b16; //如果高16位不为0 则我们右移动16位 来看高16位的情况
    //下面过程基本类似
    b8=!!(x>>8)<<3;
    x >>= b8;
    b4 = !!(x >> 4) << 2;
    x >>= b4;
    b2 = !!(x >> 2) << 1;
    x >>= b2;
    b1 = !!(x >> 1);
    x >>= b1;
    b0 = x;
    return b0+b1+b2+b4+b8+b16+1;
}
//float
/*
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
    int exp_mask = 0x7f800000;
    int exp = (uf&exp_mask)>>23;
    int frac_mask = 0x7fffff;
    int frac = uf & frac_mask;
    if (exp == 0) {
        if (frac>>23&1) {
            frac = frac * 2 - 0x7fffff;
        } else {
            frac = frac << 1;
        }
        return (uf & ~frac_mask) | frac;
    } else if (exp == 255) {
        return uf;
    } else {
        return (uf & ~exp_mask) | (exp+1)<<23;
    }
}
/*
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
    int exp_mask = 0x7f800000;
    int frac_mask = 0x7fffff;
    int exp = (uf & exp_mask)>>23;
    int frac = uf & frac_mask;
    int signal = uf>>31&1;
    int bias;
    int ans;
    if (exp < 127) {
        return 0;
    } else if (exp == 255 || exp > 127+31) {
        return 0x80000000;
    } else {
        if (exp==127+31){
            if (frac==0 && signal == 1) {
                return 0x80000000;
            } else {
                return 0x80000000;
            }
        } else {
            bias = exp-127;
            if (bias >= 23) {
                ans = (1<<bias) + (1<<bias>>23)*frac;
            } else {
                ans = 1<<bias;
            }
            return signal ? -ans:ans;
        }
    }
}
/*
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 *
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatPower2(int x) {
    int frac, exp;
    if (x < -149) return 0;
    else if (x >= -149 && x <= -127) {
        frac = x+149;
        return 1 << frac;
    } else if (x<128) {
        exp = x+127;
        return exp << 23;
    } else {
        return 255 << 23;
    }
}
```

我自己的做的时候 `howManyBits` 不知道怎么做,上网查的答案

其他的题目也都不是特别困难,看一下位运算应该也可以看懂

## 编译运行

```bash
make
# 编译会有一个警告,忽略即可
# 检查正确性,如果没有输出则说明所有函数实现符合要求,否则需要修改
./dlc bits.c
./btest
```

```bash
root@da1811a84ddc:~/csapplabs/01Data Lab/datalab-handout# ./dlc bits.c
root@da1811a84ddc:~/csapplabs/01Data Lab/datalab-handout# ./btest
Score   Rating  Errors  Function
 1      1       0       bitXor
 1      1       0       tmin
 1      1       0       isTmax
 2      2       0       allOddBits
 2      2       0       negate
 3      3       0       isAsciiDigit
 3      3       0       conditional
 3      3       0       isLessOrEqual
 4      4       0       logicalNeg
 4      4       0       howManyBits
 4      4       0       floatScale2
 4      4       0       floatFloat2Int
ERROR: Test floatPower2 failed.
  Timed out after 10 secs (probably infinite loop)
Total points: 32/36
```

最后的 `floatPower2` 超时是因为测试案例太多了,修改btest.c中的TIMEOUT_LIMIT宏,重新编译后通过

```c
// btest.c
#define TIMEOUT_LIMIT 20
```

```bash
root@da1811a84ddc:~/csapplabs/01Data Lab/datalab-handout# ./btest
Score   Rating  Errors  Function
 1      1       0       bitXor
 1      1       0       tmin
 1      1       0       isTmax
 2      2       0       allOddBits
 2      2       0       negate
 3      3       0       isAsciiDigit
 3      3       0       conditional
 3      3       0       isLessOrEqual
 4      4       0       logicalNeg
 4      4       0       howManyBits
 4      4       0       floatScale2
 4      4       0       floatFloat2Int
 4      4       0       floatPower2
Total points: 36/36
```

关于浮点数的知识笔者留下的印象也不多了,故特做了一文[计算机系统基础：浮点数的表示](https://zhuanlan.zhihu.com/p/580191161?)
