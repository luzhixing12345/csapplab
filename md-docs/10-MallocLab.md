# MallocLab

## 前言

malloc lab是设计一个自己的用于分配和管理内存的malloc free的实验

在文件夹内可以看到两个short1-bal.rep short2-bal.rep的测试文件, 但这里的测试文件并不全, 在编译之前首先需要添加 `traces/` 文件夹, 可以clone[仓库](https://github.com/Ethan-Yan27/CSAPP-Labs)然后将其malloclab的traces文件夹移动到本实验的根目录, 然后修改config.h中的路径

```c
#define TRACEDIR "traces/"
```

```bash
└── traces
    ├── amptjp-bal.rep
    ├── binary-bal.rep
    ├── binary2-bal.rep
    ├── cccp-bal.rep
    ├── coalescing-bal.rep
    ├── cp-decl-bal.rep
    ├── expr-bal.rep
    ├── random-bal.rep
    ├── random2-bal.rep
    ├── realloc-bal.rep
    └── realloc2-bal.rep
```

make编译即可

## 前置知识

开始本实验之前建议先复习一下虚拟内存动态内存分配 9.9 节, 读者也可以阅读笔者的一些文档

- [动态内存分配](https://luzhixing12345.github.io/2023/03/30/%E4%BD%93%E7%B3%BB%E7%BB%93%E6%9E%84/%E5%8A%A8%E6%80%81%E5%86%85%E5%AD%98%E5%88%86%E9%85%8D/#%E5%AE%9E%E9%AA%8C%E6%8A%A5%E5%91%8A)
- [虚拟内存](https://luzhixing12345.github.io/2023/03/26/%E4%BD%93%E7%B3%BB%E7%BB%93%E6%9E%84/%E8%99%9A%E6%8B%9F%E5%86%85%E5%AD%98/)

这里做一个简要的回顾, 采用隐式静态链表连接各个块, 结构如下

![20230331005342](https://raw.githubusercontent.com/learner-lu/picbed/master/20230331005342.png)

堆中隐式链表的上各个块的组织结构如下

![20230330200747](https://raw.githubusercontent.com/learner-lu/picbed/master/20230330200747.png)

其中每一个正方形块代表 4 字节, 这里的 8/0 16/1 表示的意思是 `分配块的大小/已分配位`, 红色块为链表头, 蓝色块为实际申请的内存大小, 灰色块为填充位. 所以整个堆被划分为了几部分, 4字节空闲 + 8字节已分配 + 24字节空闲 + 12字节已分配, 中间穿插着一些头部信息块和填充块. 再创建一个链表结构保存头节点的相关信息, 以实现后续的分配释放

当分配器释放一个块的时候有四种情况, 其中中间空白处+前后的头/尾组成了一个完整的分配块, m1 m2为块大小, a/f 表示这个块是已分配的还是空闲的. 蓝色的块是当前分配器考虑释放的块

![20230331014333](https://raw.githubusercontent.com/learner-lu/picbed/master/20230331014333.png)

- 前后都是已分配的
- 前是已分配的, 后是空闲的
- 前是空闲的, 后是已分配的
- 前后都是空闲的

## 实验报告

CSAPP 书中给出了相关的代码说明, 这里做一个补充总结

### 任务目标

我们需要完成的是mm.h中声明的四个函数, 在mm.c中实现

```c
extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);
```

- **mm_init**: 测试程序会首先调用 mm_init 分配一块内存, 我们之后的所有 mm_malloc mmrealloc mm_free 都是在分配的这段内存中进行操作, 即创建一个内存池
- **mm_malloc**: 与malloc类似, 在堆中分配一段 size 大小的连续内存, 测试程序会将mm_malloc与libc中的malloc实现对比, 并且由于malloc返回的指针是八字节对齐的, 所以我们的mm_malloc实现也应该是八字节对齐
- **mm_free**: 与free类似, 释放 ptr 指针指向的内存. 同时我们必须要保证指针指向的地址是之前通过 mm_malloc 或 mm_realloc 分配的, 并且没有被释放过的
- **mm_realloc**: 与realloc 相似

  - ptr == NULL, 等同于 mm_malloc
  - size == 0, 等同于 mm_free
  - ptr != NULL && size != 0, 将原先 ptr 指针指向的内存重分配为 size 大小, 返回一个 ptr 指针可能与原先相同, 可能与原先不同, 这取决于 mm_realloc 的实现以及新老内存块的大小

我们并不能直接使用 malloc free realloc 等函数, 在 memlib.c 中提供了如下函数使用

- **mem_sbrk**: 扩展堆
- **mem_heap_lo**: 堆的最低地址(起始地址)
- **mem_heap_hi**: 堆的最高地址(结尾地址)
- **mem_heapsize**: 堆大小
- **mem_pagesize**: 返回系统页大小(linux 4kb)

### 堆检查器

由于动态内存分配器的设计, 在编写代码的过程中很容易导致一些内存未释放, 指针悬挂等问题. 为了更好的检查我们的内存分配器的正确性, 作者建议我们在开始之前先完成一个 堆检查器(heap checker), `int mm_check(void)` 主要用于检查

- 空闲列表中标记的空闲块都是空闲的么?
- 是否有连续的空闲块没有被合并?
- 所有的空闲块都在空闲列表中么?
- 空闲列表中的所有指针都指向了合法的空闲块么?
- 已分配的块是否存在重叠?
- 在堆上的指针是否指向的是合法的地址(已分配块的开头)?

关于这部分的设计与实现笔者将会放到最后面介绍, 笔者完成了此部分并扩展实现了一个堆内存的检查器: [valloc](https://github.com/luzhixing12345/valloc)

### memlib.c

memlib.c 中包含着实验内存模型的设计, 其中关键函数 `mem_init` 和 `mem_sbrk` 如下所示. 其中初始化阶段程序调用 mem_init 构建了整个实验内存系统. 宏 MAX_HEAP 的值是 `(20 * (1 << 20))` 也就是 20MB, mem_start_brk, mem_brk, mem_max_addr 分别代表堆的起始地址, 堆顶位置, 最高地址

> 这里说的初始化阶段调用 mem_init 是指测试程序 mdriver.c(289行) 去初始化整个实验, 整个实验是在 20MB 的一个堆内存中再做一个动态内存分配器. 这一步并不是前文提到的 mm_init, mm_init 是指初始化我们需要实现的动态内存分配器的一些数据结构等东西

注意这三个变量都是 static ,所以并不能在程序中直接调用, 需要依赖 memlib.c 中提供的函数间接调用.

mem_init 初始化后 mem_brk = mem_start_brk, 也就是堆大小暂时为 0

mem_sbrk 为堆扩容操作, 在函数内部将 mem_brk 堆顶指针上移, 返回扩容前的堆顶地址, 如果超过了最大堆大小则报错退出

正常来说 mem_sbrk 的函数原型 `void *sbrk(intptr_t increment)` 中的 increment 是支持正负两个方向的变化的, 这里没有考虑那么多, 也没必要考虑那么多...

```c
/* private variables */
static char *mem_start_brk; /* points to first byte of heap */
static char *mem_brk;       /* points to last byte of heap */
static char *mem_max_addr;  /* largest legal heap address */

/*
 * mem_init - initialize the memory system model
 */
void mem_init(void) {
    /* allocate the storage we will use to model the available VM */
    if ((mem_start_brk = (char *)malloc(MAX_HEAP)) == NULL) {
        fprintf(stderr, "mem_init_vm: malloc error\n");
        exit(1);
    }

    mem_max_addr = mem_start_brk + MAX_HEAP; /* max legal heap address */
    mem_brk = mem_start_brk;                 /* heap is empty initially */
}

void *mem_sbrk(int incr) {
    char *old_brk = mem_brk;

    if ((incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
        errno = ENOMEM;
        fprintf(stderr, "ERROR: mem_sbrk failed. Ran out of memory...\n");
        return (void *)-1;
    }
    mem_brk += incr;
    return (void *)old_brk;
}
```

### 宏定义

下面介绍一下代码中涉及到的宏定义

```c
#define WSIZE 4              // 单字大小
#define DSIZE 8              // 双字大小
#define CHUNKSIZE (1 << 12)  // 扩展堆时的默认大小

#define MAX(x, y) ((x) > (y) ? (x) : (y)) // 比较大小

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) == (val))

#define PACK(size, alloc) ((size) | (alloc))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp)-DSIZE)))

void *heap_listp;

static void *extend_heap(size_t words);  // 堆空间不足时扩展
static void *coalesce(void *bp);         // 合并空闲块

static void *first_fit(size_t size);
// static void *best_fit(size_t size);

void *(*search_method)(size_t size) = first_fit;  // 搜索算法

static void place(void *bp, size_t size);
```

其中 WSIZE 和 DSIZE 分别为单字和双字, CHUNKSIZE 为默认的扩展堆大小(4MB). MAX 比较大小

GET(p) 和 PUT(p,val) 分别为取值和赋值, 注意因为 p 是 `void *` 类型所以这里使用了 `unsigned int*` 来做一个强制类型转换读取四字节的数据

PACK GET_SIZE GETALLOC 这三个宏比较清晰, 对应上文提到的每一个块的头部的数据含义, 最低一位代表是否分配, 低三位因为对齐所以不考虑其对大小的影响

![20230331005342](https://raw.githubusercontent.com/learner-lu/picbed/master/20230331005342.png)

HDRP 根据实际块指针向前推4字节得到链表块的头部, 这里的变量名 bp 代表块指针(payload部分), p 是整个块的指针

> 这里的堆是从低地址向高地址增长,所以是-WSIZE.

FTPR 得到链表块的尾部, 块指针加上块大小, 再向后推一个头一个尾8字节得到当前块的尾部地址. 

NEXT_BLKP 和 PREV_BLKP 分别是得到前一个和后一个块指针

最后再定义一个全局符号 heap_listp 代表头指针, 用于后续查找. 全局符号 search_method 用于搜索算法的选择, 这样可以比较方便的切换 first_fit 和 best_fit 或者其他

### mm_init

mm_init 用于初始化动态内存分配器

```c
int mm_init(void) {
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += 2 * WSIZE;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
    return 0;
}
```

首先使用 `mem_sbrk` 扩展堆, 申请四个块. 这四个块用于维护我们后续的链表

- 块1 不使用, 只是作为填充位, 为了使分配的地址是 8 字节对齐的
- 块2,3 是一个 **序言块**, 这是一个 8 字节的已分配的块, 只有头尾块没有申请块, 作为整个链表的头部
- 块4 是一个结尾块, 这个块的设计比较巧妙

![20230521183701](https://raw.githubusercontent.com/learner-lu/picbed/master/20230521183701.png)

回顾一下前面提到的宏的含义, PUT 用于赋值, PACK 合并size和分配位. 最后将 heap_listp 指向序言块的中间位置. 接着调用 `extend_heap` 扩展堆大小

> 这里的 heap_listp 现在是 += 2 * WSIZE, 实际上改为 4 * WSIZE 更好, 因为跳过序言块直接指向第一个空闲块

`extend_heap` 传入的参数需要考虑 8 字节对齐, 所以传参的时候使用 `CHUNKSIZE / WSIZE` 然后在内部判断对于奇数补齐.

```c
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }
    PUT(HDRP(bp), PACK(size, 0));          // 头部块置0
    PUT(FTRP(bp), PACK(size, 0));          // 尾部块置0
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // 重置结尾块

    return coalesce(bp);
}
```

接下来的操作比较有意思, 申请了一块 size 大小的堆空间, 返回 bp 为原先 mem_brk 的位置, 新的 mem_brk 移到尾部

![20230521184918](https://raw.githubusercontent.com/learner-lu/picbed/master/20230521184918.png)

现在的结构如上图所示, 实际上分配的 size 是 (bp, mem_brk) 的区间, 但由于每一个块都是 WSIZE 大小, 所以相当于等效的前移一个块的位置. 因此可以直接将通过 mem_sbrk 返回的指针 bp 看作是一个空分配的区间块, PUT 设置其 HDRP 和 FTRP 为未分配0, 然后再通过 NEXT_BLKP(bp) 找到下一个的位置, 也就是新的结尾块.

`extend_heap` 会在两种情况下被调用

- 初始化堆
- mm_malloc 找不到一个可用的(足够大)块, 扩容堆空间

最后在结尾处调用了 `coalesce(bp)` 尝试合并块

> 因为扩容的时候可能最后一个是空闲块, 但是大小不够分配. 所以需要把新扩容的空闲块和最后一个空闲块合并为一个空闲块

考虑之前提到的合并的四种情况, 只需要考虑当前块前面和后面的块的状态

![20230331014333](https://raw.githubusercontent.com/learner-lu/picbed/master/20230331014333.png)

对应下面四种情况的代码, 通过之前定义的宏, 可以比较方便的找到对应的头尾, 修改为更新后的 size

```c
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // case 1: 前后都被占用
    if (prev_alloc && next_alloc) {
        return bp;
    } else if (prev_alloc && !next_alloc) {
        // case 2: 前占后不占
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return bp;
    } else if (!prev_alloc && next_alloc) {
        // case 3: 前不占后占
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return PREV_BLKP(bp);
    } else {
        // case 4: 前后都空
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        return PREV_BLKP(bp);
    }
}
```

另外现在回看使用的序言块和结尾块, 将开头和结尾都设置了已分配, 这样可以避免边界情况的判断, 省去不少麻烦

同时也可以看到之前定义的一些宏相当有作用, 看起来代码很简洁可读

值得一提的是, **如果需要修改一个块的 HDRP 和 FTRP 的size, 应该先修改 HDRP**, 因为 FTRP 的定位需要 GET_SIZE, 这依赖了 HDRP 的size的值

### mm_malloc

mm_malloc 的实现如下, 首先根据 size 重新计算一下实际上需要分配的块大小 block_size, 然后通过一个搜索算法找到一个合适的块, 使用 place 将这个块分配. 如果没有找到合适的块则扩容后 place

> 扩容的大小是 MAX(block_size, CHUNKSIZE) 

```c
void *mm_malloc(size_t size) {
    size_t block_size;   // 块大小
    size_t extend_size;  // 额外分配的堆空间

    void *bp;

    if (size == 0) {
        return NULL;
    }

    // 小于等于 DSIZE 的大小直接补齐 2*DSIZE
    if (size <= DSIZE) {
        block_size = 2 * DSIZE;
    } else {
        // DSIZE 对齐 + 头尾 DSIZE
        block_size = (size + DSIZE - 1) / DSIZE * DSIZE + DSIZE;
    }

    // 通过搜索算法找到一个合适的空闲块
    if ((bp = search_method(block_size)) != NULL) {
        place(bp, block_size);
        return bp;
    }

    // 如果没找到, 说明需要扩容堆空间
    extend_size = MAX(block_size, CHUNKSIZE);
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL) {
        // 扩容失败, 堆已达最大 MAX_HEAP
        return NULL;
    }
    place(bp, block_size);
    return bp;
}
```

首次适配算法就是找到第一个 free 的, 并且 size 足够大的

如果 block_size 是0, 那么就说明到达了结尾块

> 注意之前在 mm_init 的时候将结尾块设置了 size = 0, 除了这个块之外的所有块的大小至少是 DSIZE

```c
void *first_fit(size_t size) {
    void *bp = heap_listp;

    while (1) {
        // 已被分配, 下一个
        size_t block_size = GET_SIZE(HDRP(bp));
        if (GET_ALLOC(HDRP(bp))) {
            bp = NEXT_BLKP(bp);
        } else {
            // 空闲块
            if (block_size >= size) {
                // 大小足够, 直接返回
                return bp;
            } else {
                bp = NEXT_BLKP(bp);
            }
        }
        if (block_size == 0) {
            return NULL;
        }
    }
}
```

最佳适配就是找到一个满足的 block_size 最小的块用于分配

```c

void *best_fit(size_t size) {
    void *bp = heap_listp;
    void *best_bp = NULL;
    size_t min_block_size = -1;

    while (1) {
        // 已被分配, 下一个
        size_t block_size = GET_SIZE(HDRP(bp));
        if (GET_ALLOC(HDRP(bp))) {
            bp = NEXT_BLKP(bp);
        } else {
            // 空闲块
            if (block_size >= size) {
                if (block_size < min_block_size) {
                    min_block_size = block_size;
                    best_bp = bp;
                }
            }
            bp = NEXT_BLKP(bp);
        }
        if (block_size == 0) {
            break;
        }
    }
    return best_bp;
}
```

place 的实现如下, 需要判断一下当前块的剩余空间是否足够大, 是否分裂

最小的块的大小应该是只有一个 header 和 footer, 也就是 DSIZE, 小于等于这个值就说明不值得被分裂. 直接将一整块都分配过去, 也就是修改的分配的 size 为 block_size

```c
void place(void *bp, size_t size) {
    size_t block_size = GET_SIZE(HDRP(bp));
    size_t block_rest_size = block_size - size;
    // 剩余块的大小 <= 最小块大小
    if (block_rest_size <= DSIZE) {
        PUT(HDRP(bp), PACK(block_size, 1));
        PUT(FTRP(bp), PACK(block_size, 1));
    } else {
        // 分裂
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(block_rest_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(block_rest_size, 0));
    }
}
```

### mm_free

mm_free 的实现就比较简单了, 直接将这里的分配位置为 0 即可

> 当然, 这里的假设是free的ptr一定是之前已经分配过的, 真实情况的处理还需要考虑 double free 以及 ptr 地址无效的情况

```c
void mm_free(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}
```

### mm_realloc

mm_realloc 的实现稍微复杂一些, 首先回顾一下 realloc 的时候可能出现的情况

- ptr == NULL, 等同于 mm_malloc
- size == 0, 等同于 mm_free
- ptr != NULL && size != 0

对于最后一种情况再分类讨论一下

1. size == block_size: 直接返回
2. size < block_size: 在当前块中修改, 同时需要注意一下是否需要分裂
3. size > block_size: 
   - 下一个块是否是空闲且空间足够, 如果足够则合并后面的块, 同时需要注意一下是否需要分裂
   - 重新 mm_malloc 一个新的块, 然后把之前的使用 memcpy 复制到新的块, 释放原先的块

最后需要注意一点的是 `memcpy(bp, ptr, block_size - DSIZE)`, 这里的复制的大小是 block_size - DSIZE, 这是因为 block_size 实际上是整个分配块的大小, 也就是 header + payload + padding + footer 的大小, 实际有效的分配空间是 payload + padding, 这部分才是需要copy过去的大小, 因此对于 ptr 这个指针的位置复制 block_size - DSIZE 空间即可

```c
void *mm_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size = (size + DSIZE - 1) / DSIZE * DSIZE + DSIZE;

    size_t block_size = GET_SIZE(HDRP(ptr));

    if (size == block_size) {
        return ptr;
    } else if (size < block_size) {
        // 在当前分配块中修改大小
        size_t block_rest_size = block_size - size;
        // 判断是否分裂
        if (block_rest_size > DSIZE) {
            PUT(HDRP(ptr), PACK(size, 1));
            PUT(FTRP(ptr), PACK(size, 1));
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(block_rest_size, 0));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(block_rest_size, 0));
        }
        return ptr;
    } else {
        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) &&
            (GET_SIZE(HDRP(NEXT_BLKP(ptr))) >= (size - block_size))) {
            // 下一个块就有空间
            size_t next_block_rest_size = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + block_size - size;
            if (next_block_rest_size <= DSIZE) {
                size_t new_size = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + block_size;
                PUT(HDRP(ptr), PACK(new_size, 1));
                PUT(FTRP(ptr), PACK(new_size, 1));
                return ptr;
            } else {
                PUT(HDRP(ptr), PACK(size, 1));
                PUT(FTRP(ptr), PACK(size, 1));
                PUT(HDRP(NEXT_BLKP(ptr)), PACK(next_block_rest_size, 0));
                PUT(FTRP(NEXT_BLKP(ptr)), PACK(next_block_rest_size, 0));
                return ptr;
            }
        } else {
            void *bp = mm_malloc(size);
            if (bp == NULL) {
                return NULL;
            }
            memcpy(bp, ptr, block_size - DSIZE);
            mm_free(ptr);
            return bp;
        }
    }
}
```

### 实验结果

```bash
make
./mdriver -V -t traces/
```

first_fit 的结果 75/100

![20230522233023](https://raw.githubusercontent.com/learner-lu/picbed/master/20230522233023.png)

best_fit 的结果 68/100, 主要是因为 best_fit 需要扫描所有块

![20230523003450](https://raw.githubusercontent.com/learner-lu/picbed/master/20230523003450.png)

完整代码如下:

```c
#include "mm.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4              // 单字大小
#define DSIZE 8              // 双字大小
#define CHUNKSIZE (1 << 12)  // 扩展堆时的默认大小

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define PACK(size, alloc) ((size) | (alloc))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

void *heap_listp;

static void *extend_heap(size_t words);  // 堆空间不足时扩展
static void *coalesce(void *bp);         // 合并空闲块

static void *first_fit(size_t size);
static void *best_fit(size_t size);

void *(*search_method)(size_t size) = first_fit;  // 搜索算法

static void place(void *bp, size_t size);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += 4 * WSIZE;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

static void *extend_heap(size_t words) {
    void *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }
    PUT(HDRP(bp), PACK(size, 0));          // 头部块置0
    PUT(FTRP(bp), PACK(size, 0));          // 尾部块置0
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // 重置结尾块
    return coalesce(bp);
}

static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // case 1: 前后都被占用
    if (prev_alloc && next_alloc) {
        return bp;
    } else if (prev_alloc && !next_alloc) {
        // case 2: 前占后不占
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return bp;
    } else if (!prev_alloc && next_alloc) {
        // case 3: 前不占后占
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return PREV_BLKP(bp);
    } else {
        // case 4: 前后都空
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        return PREV_BLKP(bp);
    }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t block_size;   // 块大小
    size_t extend_size;  // 额外分配的堆空间

    void *bp;

    if (size == 0) {
        return NULL;
    }

    // 小于等于 DSIZE 的大小直接补齐 2*DSIZE
    if (size <= DSIZE) {
        block_size = 2 * DSIZE;
    } else {
        // DSIZE 对齐 + 头尾 DSIZE
        block_size = (size + DSIZE - 1) / DSIZE * DSIZE + DSIZE;
    }

    // 通过搜索算法找到一个合适的空闲块
    if ((bp = search_method(block_size)) != NULL) {
        place(bp, block_size);
        return bp;
    }

    // 如果没找到, 说明需要扩容堆空间
    extend_size = MAX(block_size, CHUNKSIZE);
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL) {
        // 扩容失败, 堆已达最大 MAX_HEAP
        return NULL;
    }
    place(bp, block_size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size = (size + DSIZE - 1) / DSIZE * DSIZE + DSIZE;

    size_t block_size = GET_SIZE(HDRP(ptr));

    if (size == block_size) {
        return ptr;
    } else if (size < block_size) {
        // 在当前分配块中修改大小
        size_t block_rest_size = block_size - size;
        // 判断是否分裂
        if (block_rest_size > DSIZE) {
            PUT(HDRP(ptr), PACK(size, 1));
            PUT(FTRP(ptr), PACK(size, 1));
            PUT(HDRP(NEXT_BLKP(ptr)), PACK(block_rest_size, 0));
            PUT(FTRP(NEXT_BLKP(ptr)), PACK(block_rest_size, 0));
            coalesce(NEXT_BLKP(ptr));
        }
        return ptr;
    } else {
        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) &&
            (GET_SIZE(HDRP(NEXT_BLKP(ptr))) >= (size - block_size))) {
            // 下一个块就有空间
            size_t next_block_rest_size = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + block_size - size;
            if (next_block_rest_size <= DSIZE) {
                size_t new_size = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + block_size;
                PUT(HDRP(ptr), PACK(new_size, 1));
                PUT(FTRP(ptr), PACK(new_size, 1));
                return ptr;
            } else {
                PUT(HDRP(ptr), PACK(size, 1));
                PUT(FTRP(ptr), PACK(size, 1));
                PUT(HDRP(NEXT_BLKP(ptr)), PACK(next_block_rest_size, 0));
                PUT(FTRP(NEXT_BLKP(ptr)), PACK(next_block_rest_size, 0));
                return ptr;
            }
        } else {
            void *bp = mm_malloc(size);
            if (bp == NULL) {
                return NULL;
            }
            memcpy(bp, ptr, block_size - DSIZE);
            mm_free(ptr);
            return bp;
        }
    }
}

void place(void *bp, size_t size) {
    size_t block_size = GET_SIZE(HDRP(bp));
    size_t block_rest_size = block_size - size;
    // 剩余块的大小 <= 最小块大小
    if (block_rest_size <= DSIZE) {
        PUT(HDRP(bp), PACK(block_size, 1));
        PUT(FTRP(bp), PACK(block_size, 1));
    } else {
        // 分裂
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(block_rest_size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(block_rest_size, 0));
    }
}

void *first_fit(size_t size) {
    void *bp = heap_listp;

    while (1) {
        // 已被分配, 下一个
        size_t block_size = GET_SIZE(HDRP(bp));
        if (GET_ALLOC(HDRP(bp))) {
            bp = NEXT_BLKP(bp);
        } else {
            // 空闲块
            if (block_size >= size) {
                // 大小足够, 直接返回
                return bp;
            } else {
                bp = NEXT_BLKP(bp);
            }
        }
        if (block_size == 0) {
            return NULL;
        }
    }
}

void *best_fit(size_t size) {
    void *bp = heap_listp;
    void *best_bp = NULL;
    size_t min_block_size = -1;

    while (1) {
        // 已被分配, 下一个
        size_t block_size = GET_SIZE(HDRP(bp));
        if (GET_ALLOC(HDRP(bp))) {
            bp = NEXT_BLKP(bp);
        } else {
            // 空闲块
            if (block_size >= size) {
                if (block_size < min_block_size) {
                    min_block_size = block_size;
                    best_bp = bp;
                }
            }
            bp = NEXT_BLKP(bp);
        }
        if (block_size == 0) {
            break;
        }
    }
    return best_bp;
}
```

## 优化

显然 75 分并不够理想, 我们需要考虑一下如下优化程序. 观察一下测试结果, 其中手册中提到了评分标准, 正确性20%, 空间利用率 util 和操作数Kops 分别 35%, 最后 10% 是代码规范性

```bash
Results for mm malloc:
trace  valid  util     ops      secs  Kops
 0       yes   99%    5694  0.004722  1206
 1       yes   99%    5848  0.004367  1339
 2       yes   99%    6648  0.007349   905
 3       yes  100%    5380  0.005558   968
 4       yes   66%   14400  0.000072200278
 5       yes   92%    4800  0.004250  1129
 6       yes   92%    4800  0.003929  1222
 7       yes   55%   12000  0.064229   187
 8       yes   51%   24000  0.215506   111
 9       yes   92%   14401  0.000096150010
10       yes   86%   14401  0.000055260888
Total          85%  112372  0.310133   362

Perf index = 51 (util) + 24 (thru) = 75/100
```

考虑一下导致分数不佳的原因, 也就是整体拖慢程序的性能的根源, 在于 first_fit 的搜索.

malloc 主要的难点就是在于如何在尽可能短的时间内找到一个合适的块用于分配, 目前的 first_fit 近乎 O(N) 的时间复杂度, 随着块的分配数量越来越多, 耗时会越来越长, 这显然是不甚理想的

### 显式空闲链表

显式空闲链表的又进一步设置了两个小块, pred 和 succ 分别记录前驱和后继. 如下所示

需要注意的是, **只有空闲块会有这两个结构**, 如果一个块被分配出去了, 那么还是在原先的 payload 的位置保存数据, 分配块是不需要记录前驱后继的信息的. 空闲块反正后面的空间也没有人使用, 占了也就占了. 这样就可以把 first_fit 搜索全部块优化为通过链表搜索空闲块.

> 这里使用双向链表的原因是为了分配的时候可以在 O(1) 找到前一个和后一个空闲块, 否则只能不断地 NEXT/PRED + GET_ALLOC 的去找

当然, 带来的一个问题就是又加入了新的结构, 每一个分配块的最小大小又增加了, 内部空间碎片率也会更高

![20230523015859](https://raw.githubusercontent.com/learner-lu/picbed/master/20230523015859.png)

与此同时还需要考虑的一个问题就是如何更新和维护这个双向链表. 如下图A所示, "?" 块刚刚被释放, 它需要作为一个空闲块加入链表

![20230523025234](https://raw.githubusercontent.com/learner-lu/picbed/master/20230523025234.png)

第一种方式 B 就是直接按照顺序加入, 但是显然这需要 O(N) 的时间搜索前一个空闲块的位置

第二种方式 C 直接将释放的块插入到链表的头部

![20230524152405](https://raw.githubusercontent.com/learner-lu/picbed/master/20230524152405.png)

```c
/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include "mm.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4              // 单字大小
#define DSIZE 8              // 双字大小
#define CHUNKSIZE (1 << 12)  // 扩展堆时的默认大小

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define PUT_ADDR(p, val) (*(unsigned long *)(p) = (unsigned long)(val))
#define VAL(val) (val == NULL ? 0 : (*(unsigned long *)val))

#define PACK(size, alloc) ((size) | (alloc))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define PRED(bp) ((char *)(bp))
#define SUCC(bp) ((char *)(bp) + DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))
#define NEXT_FREE_BLKP(bp) ((void *)*(unsigned long *)SUCC(bp))
#define PREV_FREE_BLKP(bp) ((void *)*(unsigned long *)PRED(bp))

void *heap_listp;

static void *extend_heap(size_t words);  // 堆空间不足时扩展

static void *first_fit(size_t size);
static void place(void *bp, size_t size);

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) {
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp = NULL;

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {
        return -1;
    }
    return 0;
}

static void *extend_heap(size_t words) {
    void *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) {
        return NULL;
    }
    PUT(HDRP(bp), PACK(size, 0));          // 头部块置0
    PUT(FTRP(bp), PACK(size, 0));          // 尾部块置0
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // 重置结尾块
    // 前面的块是是空闲块, 直接合并新的扩展的块
    if (!GET_ALLOC(FTRP(PREV_BLKP(bp)))) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else {
        PUT_ADDR(PRED(bp), 0);
        PUT_ADDR(SUCC(bp), VAL(heap_listp));
        if (NEXT_FREE_BLKP(bp)) {
            PUT_ADDR(PRED(NEXT_FREE_BLKP(bp)), bp);
        }
        heap_listp = bp;
    }
    return bp;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t block_size;   // 块大小
    size_t extend_size;  // 额外分配的堆空间

    void *bp;

    if (size == 0) {
        return NULL;
    }

    // 小于等于 DSIZE 的大小直接补齐 2*DSIZE
    if (size <= DSIZE) {
        block_size = 3 * DSIZE;
    } else {
        // DSIZE 对齐 + 头尾 DSIZE
        block_size = (size + DSIZE - 1) / DSIZE * DSIZE + DSIZE;
    }

    // 通过搜索算法找到一个合适的空闲块
    if ((bp = first_fit(block_size)) != NULL) {
        place(bp, block_size);
        return bp;
    }

    // 如果没找到, 说明需要扩容堆空间
    extend_size = MAX(block_size, CHUNKSIZE);
    if ((bp = extend_heap(extend_size / WSIZE)) == NULL) {
        // 扩容失败, 堆已达最大 MAX_HEAP
        return NULL;
    }
    place(bp, block_size);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));

    // case 1: 前后都被占用, 加到链表头
    if (prev_alloc && next_alloc) {
        PUT_ADDR(PRED(bp), 0);
        PUT_ADDR(SUCC(bp), heap_listp);
        if (NEXT_FREE_BLKP(bp)) {
            PUT_ADDR(PRED(NEXT_FREE_BLKP(bp)), bp);
        }
        heap_listp = bp;
    } else if (prev_alloc && !next_alloc) {
        // case 2: 前占后不占, 合并后面的块, 修改链表
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        void *next_bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        // 更新链表

        if (PREV_FREE_BLKP(next_bp) && NEXT_FREE_BLKP(next_bp)) {
            PUT_ADDR(PRED(bp), PREV_FREE_BLKP(next_bp));
            PUT_ADDR(SUCC(bp), NEXT_FREE_BLKP(next_bp));
            PUT_ADDR(SUCC(PREV_FREE_BLKP(bp)), bp);
            PUT_ADDR(PRED(NEXT_FREE_BLKP(bp)), bp);
        } else if (!PREV_FREE_BLKP(next_bp) && NEXT_FREE_BLKP(next_bp)) {
            PUT_ADDR(PRED(bp), 0);
            heap_listp = bp;
            PUT_ADDR(SUCC(bp), NEXT_FREE_BLKP(next_bp));
            PUT_ADDR(PRED(NEXT_FREE_BLKP(next_bp)), bp);
        } else if (PREV_FREE_BLKP(next_bp) && !NEXT_FREE_BLKP(next_bp)) {
            PUT_ADDR(PRED(bp), PREV_FREE_BLKP(next_bp));
            PUT_ADDR(SUCC(PREV_FREE_BLKP(next_bp)), bp);
            PUT_ADDR(SUCC(bp), 0);
        } else {
            PUT_ADDR(PRED(bp), 0);
            heap_listp = bp;
            PUT_ADDR(SUCC(bp), 0);
        }
    } else if (!prev_alloc && next_alloc) {
        // case 3: 前不占后占, 直接合并
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    } else {

        // case 4: 前后都空, 合并三个空闲块, 更新第一个块的节点
        void *pre_bp = PREV_BLKP(bp);
        void *next_bp = NEXT_BLKP(bp);
        size += GET_SIZE(HDRP(pre_bp)) + GET_SIZE(FTRP(next_bp));
        PUT(HDRP(pre_bp), PACK(size, 0));
        PUT(FTRP(pre_bp), PACK(size, 0));
        // 更新链表
        if (PREV_FREE_BLKP(next_bp) && NEXT_FREE_BLKP(next_bp)) {
            PUT_ADDR(SUCC(PREV_FREE_BLKP(next_bp)), VAL(SUCC(next_bp)));
            PUT_ADDR(PRED(NEXT_FREE_BLKP(next_bp)), PREV_FREE_BLKP(next_bp));
        } else if (PREV_FREE_BLKP(next_bp) && !NEXT_FREE_BLKP(next_bp)) {
            PUT_ADDR(SUCC(PREV_FREE_BLKP(next_bp)), 0);
        } else if (!PREV_FREE_BLKP(next_bp) && NEXT_FREE_BLKP(next_bp)) {
            PUT_ADDR(PRED(NEXT_FREE_BLKP(next_bp)), 0);
            heap_listp = NEXT_FREE_BLKP(next_bp);
        } else {
            printf("%p\n",heap_listp);
        }
    }
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    if (size <= DSIZE) {
        size = 3 * DSIZE;
    } else {
        size = (size + DSIZE - 1) / DSIZE * DSIZE + DSIZE;
    }

    size_t block_size = GET_SIZE(HDRP(ptr));

    if (size == block_size) {
        return ptr;
    } else if (size < block_size) {
        // 在当前分配块中修改大小
        size_t block_rest_size = block_size - size;
        // 判断是否分裂
        if (block_rest_size >= 3 * DSIZE) {
            PUT(HDRP(ptr), PACK(size, 1));
            PUT(FTRP(ptr), PACK(size, 1));
            void *next_bp = NEXT_BLKP(ptr);
            // 多出来的块直接放在链表最前面
            PUT(HDRP(next_bp), PACK(block_rest_size, 0));
            PUT(FTRP(next_bp), PACK(block_rest_size, 0));
            PUT_ADDR(PRED(next_bp), 0);
            PUT_ADDR(SUCC(next_bp), heap_listp);
            if (NEXT_FREE_BLKP(next_bp)) {
                PUT_ADDR(PRED(NEXT_FREE_BLKP(next_bp)), next_bp);
            }
            heap_listp = next_bp;
        }
        return ptr;
    } else {
        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) &&
            (GET_SIZE(HDRP(NEXT_BLKP(ptr))) >= (size - block_size))) {
            // 下一个块就有空间
            size_t next_block_rest_size = GET_SIZE(HDRP(NEXT_BLKP(ptr))) + block_size - size;
            if (next_block_rest_size < 3 * DSIZE) {
                void *bp = NEXT_BLKP(ptr);
                size_t new_size = GET_SIZE(HDRP(bp)) + block_size;
                PUT(HDRP(ptr), PACK(new_size, 1));
                PUT(FTRP(ptr), PACK(new_size, 1));
                if (!PREV_FREE_BLKP(bp) && !NEXT_FREE_BLKP(bp)) {
                    heap_listp = NULL;
                } else if (PREV_FREE_BLKP(bp) && !NEXT_FREE_BLKP(bp)) {
                    PUT_ADDR(SUCC(PREV_FREE_BLKP(bp)), 0);
                } else if (!PREV_FREE_BLKP(bp) && NEXT_FREE_BLKP(bp)) {
                    PUT_ADDR(PRED(NEXT_FREE_BLKP(bp)), 0);
                    heap_listp = NEXT_FREE_BLKP(bp);
                } else {
                    PUT_ADDR(SUCC(PREV_FREE_BLKP(bp)), NEXT_FREE_BLKP(bp));
                    PUT_ADDR(PRED(NEXT_FREE_BLKP(bp)), PREV_FREE_BLKP(bp));
                }
                return ptr;
            } else {
                // 分裂
                void *bp = NEXT_BLKP(ptr);
                void *new_bp = (char *)ptr + size;
                void *pre_bp = PREV_FREE_BLKP(bp);
                void *next_bp = NEXT_FREE_BLKP(bp);

                if (!pre_bp && !next_bp) {
                    PUT_ADDR(PRED(new_bp), 0);
                    PUT_ADDR(SUCC(new_bp), 0);
                    heap_listp = new_bp;
                } else if (pre_bp && !next_bp) {
                    PUT_ADDR(PRED(new_bp), pre_bp);
                    PUT_ADDR(SUCC(pre_bp), new_bp);
                    PUT_ADDR(SUCC(new_bp), 0);
                } else if (!pre_bp && next_bp) {
                    PUT_ADDR(PRED(new_bp), 0);
                    heap_listp = new_bp;
                    PUT_ADDR(SUCC(new_bp), next_bp);
                    PUT_ADDR(PRED(next_bp), new_bp);
                } else {
                    PUT_ADDR(PRED(new_bp), pre_bp);
                    PUT_ADDR(SUCC(pre_bp), new_bp);
                    PUT_ADDR(SUCC(new_bp), next_bp);
                    PUT_ADDR(PRED(next_bp), new_bp);
                }
                PUT(HDRP(new_bp), PACK(next_block_rest_size, 0));
                PUT(FTRP(new_bp), PACK(next_block_rest_size, 0));
                PUT(HDRP(ptr), PACK(size, 1));
                PUT(FTRP(ptr), PACK(size, 1));

                return ptr;
            }
        } else {
            void *bp = mm_malloc(size);
            if (bp == NULL) {
                return NULL;
            }
            memcpy(bp, ptr, block_size - DSIZE);
            mm_free(ptr);
            return bp;
        }
    }
}

void place(void *bp, size_t size) {
    size_t block_size = GET_SIZE(HDRP(bp));
    size_t block_rest_size = block_size - size;
    // 剩余块的大小 <= 最小块大小
    if (block_rest_size < 3 * DSIZE) {
        PUT(HDRP(bp), PACK(block_size, 1));
        PUT(FTRP(bp), PACK(block_size, 1));

        if (!PREV_FREE_BLKP(bp) && !NEXT_FREE_BLKP(bp)) {
            heap_listp = NULL;
        } else if (PREV_FREE_BLKP(bp) && !NEXT_FREE_BLKP(bp)) {
            PUT_ADDR(SUCC(PREV_FREE_BLKP(bp)), 0);
        } else if (!PREV_FREE_BLKP(bp) && NEXT_FREE_BLKP(bp)) {
            PUT_ADDR(PRED(NEXT_FREE_BLKP(bp)), 0);
            heap_listp = NEXT_FREE_BLKP(bp);
        } else {
            PUT_ADDR(SUCC(PREV_FREE_BLKP(bp)), NEXT_FREE_BLKP(bp));
            PUT_ADDR(PRED(NEXT_FREE_BLKP(bp)), PREV_FREE_BLKP(bp));
        }
    } else {
        // 分裂
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        void *next_bp = NEXT_BLKP(bp);
        PUT(HDRP(next_bp), PACK(block_rest_size, 0));
        PUT(FTRP(next_bp), PACK(block_rest_size, 0));
        // 更新链表
        PUT_ADDR(PRED(next_bp), VAL(PRED(bp)));
        if (PREV_FREE_BLKP(bp)) {
            PUT_ADDR(SUCC(PREV_FREE_BLKP(bp)), next_bp);
        } else {
            heap_listp = next_bp;
        }
        PUT_ADDR(SUCC(next_bp), VAL(SUCC(bp)));
        if (NEXT_FREE_BLKP(bp)) {
            PUT_ADDR(PRED(NEXT_FREE_BLKP(bp)), next_bp);
        }
    }
}

void *first_fit(size_t size) {
    void *bp = heap_listp;

    // 没有空闲块
    if (bp == NULL) {
        return NULL;
    }

    while (1) {
        size_t block_size = GET_SIZE(HDRP(bp));
        if (block_size >= size) {
            return bp;
        } else {
            bp = NEXT_FREE_BLKP(bp);
            if (bp == NULL) {
                return NULL;
            }
        }
    }
}
```

```c
/*
 *  I implemented
 *  Seglist Allocators described in the ppt 
 *  
 *  - each size class has its own free list
 *  - one class for each two-power size 
 *  - {1},{2},{3,4},{5-8}, ..., {1025-2048}, ...
 *  
 *  - It is worth noting the difference between 
 *    (predecessor and successor) and (previous and next)
 *    (predecessor and successor) : pred and succ block in free list, 
 *    (previous and next) : previous and next block in heap memory
 * 
 * - Since we are not allowed to use any global or static arrays,
 *   I allocated a memory space in heap to keep these free lists (Or Seglist Allocators).
 *   This is described in visual text below.
 * 
 * -For blocks, I used the boundary tag coalescing technique.
 */

/* ALLOCATED BLOCK, FREE BLOCK, SEGREGATED FREE LIST, HEAP STRUCTURE

 1. Allocated Block 
 
             31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  |  | 1|
    bp ---> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                                                                                               |
            |                                                                                               |
            .                              Payload and padding                                              .
            .                                                                                               .
            .                                                                                               .
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer :   |                              size of the block                                       |     | 1|
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 
 
 2. Free block 
 
             31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  |  | 0|
      bp--> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to pred block in free list                                     |
    bp+4--> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to succ block in free list                                     |
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer :   |                              size of the block                                       |     | 0|
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

 3. Segregated Free Lists 
    (space is allocated in heap to keep this array of pointers)
    (**free_lists points to first pointer in the array)
    ----------------------------------------------------------
    |  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |  |size class 0  |size class 1  |      . . .
    |  |(bp of blck 0 |(bp of blck 0 |              
    |  | is kept here)|is kept here) |
    |  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+-
    -----------------------------------------------------------
              |               |     
         +--+--+--+--+   +--+--+--+--+
         | blck 0    |   | blck 0    |
         +--+--+--+--+   +--+--+--+--+
              |               |     
         +--+--+--+--+   +--+--+--+--+
         | blck 1    |   | blck 1    | 
         +--+--+--+--+   +--+--+--+--+ 

 4. Heap Memory After Free Lists
    (prologue and epilogue blocks are tricks that eliminate the edge conditions
    during coalescing)
              31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                               0 (Padding)                                            |  |  |  |
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                               8 (Prologue block)                                     |  |  | 1|
*heap_listp ->+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                               8 (Prologue block)                                     |  |  | 1|
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              .                                                                                               .
              .                             Both Free and Allocated Blocks inmemory space                     .
              .                                                                                               .
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
              |                              0 (Epilogue block hdr)                                  |  |  | 1|
              +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>


#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/

team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros*/
#define WSIZE 4 /*Word and header/footer size (bytes) */
#define DSIZE 8 /*Double word size (bytes) */
#define INITCHUNKSIZE (1<<6)
#define CHUNKSIZE (1<<12) /*Extend heap by this amount(bytes)*/

#define MAXNUMBER 16

#define MAX(x,y) ((x) > (y) ? (x) : (y) )
#define MIN(x, y) ((x) < (y) ? (x) : (y))


/*Pack a size and allocated bit into a word*/
#define PACK(size, alloc) ((size) | (alloc))

/*Read and write a word at address p*/
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

//put pointer ptr in p (just simple address)
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/*Read the size and allocated fields from address p*/
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/*Given block ptr bp, compute address of its header and footer*/
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

/*Given block ptr bp, compute address(bp) of next and previous blocks*/
#define NEXT_BLKP(bp) ((char *)(bp)+GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))

// Address of free block's predecessor and successor entries
#define PRED_PTR(bp) ((char *)(bp))
#define SUCC_PTR(bp) ((char *)(bp) + WSIZE)

// Address of free block's predecessor and successor on the segregated list
#define PRED(bp) (*(char **)(bp))
#define SUCC(bp) (*(char **)(SUCC_PTR(bp)))

//since we cannot use such thing as Array[i] = k,
//these trivial pointer calculators are used to get and set
//values from/tp free lists
#define GET_FREE_LIST_PTR(i) (*(free_lists+i))
#define SET_FREE_LIST_PTR(i, bp) (*(free_lists+i) = bp)

/*Global variables*/
static char *heap_listp = 0; 
static char **free_lists;

/*helper functions*/
static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *place(void *bp, size_t asize);
static void add(void *bp, size_t size);
static void delete(void *bp);

void mm_check(int verbose);


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    int i;

    //create space for keeping free lists 
    if ((long)(free_lists = mem_sbrk(MAXNUMBER*sizeof(char *))) == -1){
        return -1;
    }
    
     // initialize the free list
    for (int i = 0; i <= MAXNUMBER; i++) {
	    SET_FREE_LIST_PTR(i, NULL);
    }

    // initial empty heap
    if ((long)(heap_listp = mem_sbrk(4 * WSIZE)) == -1)
        return -1;
    
    PUT(heap_listp, 0);                            /* Alignment padding */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += (2*WSIZE); 
    
    if (extend_heap(INITCHUNKSIZE) == NULL)
        return -1;

    // mm_check(1);
    
    return 0;
}


/* 
 * 
 *     
 * mm_malloc - Allocate a block with at least size bytes of payload 
 */
void *mm_malloc(size_t size)
{
    size_t asize;      //adjust request block size to allow room for header and footer
                      // and to satisfy alignment requirement.
    size_t extendsize; //incase we need to extend heap 
    void *bp = NULL;  
    
   
    if (size == 0) {
        return NULL;
    }
    
    // satisfy alignment requirement
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = ALIGN(size+DSIZE);
    }
    //textbook-add overhead bytes and then round up to nearest multiple of 8
    
    int i = 0;
    size_t searchsize = asize;

    // search free lists for suitable free block
    while (i < MAXNUMBER) {
        if ((i == MAXNUMBER - 1) || ((searchsize <= 1) && 
        (GET_FREE_LIST_PTR(i)!= NULL))) {
            bp = GET_FREE_LIST_PTR(i);
            
            //in each size class
            while ((bp != NULL) && (asize > GET_SIZE(HDRP(bp))))
            {
                bp = PRED(bp);
            }
            //found it!
            if (bp != NULL){
                //place requested block 
                //(if there is an access, it will split in place())
                bp = place(bp, asize);
                //return the pointer of the newly allocated block
                return bp;
            }
        }
        
        searchsize = searchsize >> 1;
        i++;
    }
    
    // if allocator cannot find a fit, extend the heap with 
    //new free block
    if (bp == NULL) {
        extendsize = MAX(asize, CHUNKSIZE);
        //place requested block in the new free block
        if ((bp = extend_heap(extendsize)) == NULL)
            return NULL;
    }
    
    //(place() will optionally split the block)
    bp = place(bp, asize);
    
    // mm_check(1);
    
    // return pointer to newly allocated block
    return bp;
}


/*
 * mm_free - Freeing a block
 */
void mm_free(void *bp)
{
   size_t size = GET_SIZE(HDRP(bp));
    
    //change header and footer to free 
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    
    //add it to free lists
    add(bp, size);

    //incase there is free block in prev or next block 
    coalesce(bp);
    
    return;
}

/*
 * mm_realloc : returns a pointer to an allocated region of 
 * at least size bytes
 * 
 */
void *mm_realloc(void *bp, size_t size)
{   
    void *new_block = bp;
    int remainder;

    if (bp == NULL) { //the call is equivalent to mm_malloc(size)
        return mm_malloc(size);
    }
    if (size == 0) { //the call is equivalent to mm_free(ptr)
        mm_free(bp);
        return NULL;
    }

    //to satisfy alignment
    if (size <= DSIZE){
        size = 2 * DSIZE;
    }
    else{
        size = ALIGN(size + DSIZE);
    }

    //old block size is bigger
    //I think just return the original block
    if ((remainder = GET_SIZE(HDRP(bp)) - size) >= 0){
        //PUT(HDRP(bp), PACK(size, 1));
        //PUT(FTRP(bp), PACK(size, 1));
        //mm_free(NEXT_BLKP(bp));
        return bp;
    }
    //new block size is bigger and next block is not allocated
    //use adjacent block to minimize external fragmentation
    else if (!GET_ALLOC(HDRP(NEXT_BLKP(bp))) || !GET_SIZE(HDRP(NEXT_BLKP(bp)))){
        if ((remainder = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp))) - size) < 0){
            if (extend_heap(MAX(-remainder, CHUNKSIZE)) == NULL){
                return NULL;
            }
            remainder += MAX(-remainder, CHUNKSIZE);
        }
        delete(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size + remainder, 1));
        PUT(FTRP(bp), PACK(size + remainder, 1));
    } 
    //in this case where new block's size is bigger
    //but we cannot use adjacent block, address will be different 
    else{
        new_block = mm_malloc(size);
        memcpy(new_block, bp, GET_SIZE(HDRP(bp)));
        mm_free(bp);
    }

    return new_block;
    
}

/*helper function*/
static void *extend_heap(size_t size)
{
    void *bp;
    size_t asize; //adjusted size to maintain alignment               
    
    asize = ALIGN(size);
    
    if ((bp = mem_sbrk(asize)) == (void *)-1)
        return NULL;
    
    /*Initialize free block header/footer and the epilogue header*/
    PUT(HDRP(bp), PACK(asize, 0)); //Free block header
    PUT(FTRP(bp), PACK(asize, 0)); //Free block footer 
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //New epilouge header
    add(bp, asize);
    
    //in case previous block was free 
    return coalesce(bp);
}

//add to free lists
static void add(void *bp, size_t size) {
    int i = 0;
    void *curr = bp;
    void *succ = NULL;
    
    // Select size class 
    while ((i < MAXNUMBER - 1) && (size > 1)) {
        size = size >> 1;
        i++;
    }
    
    //now we are in particular size class
    //pred <- curr <- succ (we move that way)
    curr = GET_FREE_LIST_PTR(i);
    while ((curr != NULL) && (size > GET_SIZE(HDRP(curr)))) {
        succ = curr;
        curr = PRED(curr);
    }
    
    // adding bp 
    if (curr != NULL) {
        if (succ != NULL) { //curr - bp - succ
            SET_PTR(PRED_PTR(bp), curr);
            SET_PTR(SUCC_PTR(curr), bp);
            SET_PTR(SUCC_PTR(bp), succ);
            SET_PTR(PRED_PTR(succ), bp);
        } else { //adding at the end
            SET_PTR(PRED_PTR(bp), curr);
            SET_PTR(SUCC_PTR(curr), bp);
            SET_PTR(SUCC_PTR(bp), NULL);
            SET_FREE_LIST_PTR(i, bp);
        }
    } else {
        if (succ != NULL) { //adding at the other end
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), succ);
            SET_PTR(PRED_PTR(succ), bp);
        } else { //this size class has no nodes
            SET_PTR(PRED_PTR(bp), NULL);
            SET_PTR(SUCC_PTR(bp), NULL);
            SET_FREE_LIST_PTR(i, bp);
        }
    }
    
    return;
}

//delete from free list
static void delete(void *bp) {
    int i = 0;
    size_t size = GET_SIZE(HDRP(bp));
    
    // select size class
    while ((i < MAXNUMBER - 1) && (size > 1)) {
        size >>= 1;
        i++;
    }
    //delete node making changes to pred, succ pointers 
    //pred <- curr <- succ (we move that way)
    if (PRED(bp) != NULL) {
        if (SUCC(bp) != NULL) {//pred - bp - succ => pred - succ
            SET_PTR(SUCC_PTR(PRED(bp)), SUCC(bp));
            SET_PTR(PRED_PTR(SUCC(bp)), PRED(bp));
        } else {//deleting at the end
            SET_PTR(SUCC_PTR(PRED(bp)), NULL);
            SET_FREE_LIST_PTR(i, PRED(bp));
        }
    } else {
        if (SUCC(bp) != NULL) { //deleting at the other end
            SET_PTR(PRED_PTR(SUCC(bp)), NULL);
        } else {//this size class has only bp node
            SET_FREE_LIST_PTR(i, PRED(bp));
        }
    }
    
    return;
}

/*
* this is done in physical heap memory
* returns pointer to new free block
* coalesce is called before it goes in the free list
*/
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    
    if (prev_alloc && next_alloc) { //both allocated                    
        return bp;
    }
    else if (prev_alloc && !next_alloc) { //merge next block
        delete(bp);
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) { //merge with prev block            
        delete(bp);
        delete(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                      // merge with both
        delete(bp);
        delete(PREV_BLKP(bp));
        delete(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    add(bp, size);
    
    return bp;
}

/*place - place block of asize bytes at free block bp
          split if remainder would be at least minimum block size*/
static void *place(void *bp, size_t asize)
{
    size_t bp_size = GET_SIZE(HDRP(bp));
    size_t remainder = bp_size - asize;
    
    delete(bp);
    
    
    if (remainder <= DSIZE * 2) {
        // Do not split block
        PUT(HDRP(bp), PACK(bp_size, 1));
        PUT(FTRP(bp), PACK(bp_size, 1));
    }
    /*
     ususally we just do else here and split the block.
     But there are cases like in binary-bal.rep, binary2-bal.rep

    case :  
     allocated <blocks> - (small size block or big size bock)
     small - big - small - big - small - big

     if we free this blocks in order small-big-small-big-small-bg 
     it causes no problems.

     However, if we free only the big blocks like below, 
     small - big(freed) - small - big(freed) - small- 

     Even if we want to use the big freed blocks together, we can't because
     there are small allocated blocks in between.

     If there is a allocate call to size bigger than big block,
     - we have to find another free block.

     So what I am trying to do here is put allocated blocks 
     in continuous places so we can use the freed space 

     small-small-small-small-big(freed)-big(freed)-

     I set the size 96 to get the best result for binary-bal.rep 
     and binary2-bal.rep
     */
     else if (asize >= 96) {
        PUT(HDRP(bp), PACK(remainder, 0));
        PUT(FTRP(bp), PACK(remainder, 0));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(asize, 1));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(asize, 1));
        add(bp, remainder);
        return NEXT_BLKP(bp);
        
    }
    
    else {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(remainder, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(remainder, 0));
        add(NEXT_BLKP(bp), remainder);
    }
    return bp;
}

//this function prints information about the block for debugging 
static void printblock(void *bp) 
{
    size_t hsize, halloc, fsize, falloc;

    // mm_check(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  

    if (hsize == 0) {
	printf("%p: EOL\n", bp);
	return;
    }

    printf("%p: header: [%d:%c] footer: [%d:%c]\n", bp, 
	hsize, (halloc ? 'a' : 'f'), 
	fsize, (falloc ? 'a' : 'f')); 
}

//check if block is doubleword aligned and header match the footer
static void checkblock(void *bp) 
{
    if ((size_t)bp % 8)
	printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
	printf("Error: header does not match footer\n");
}

//minimal check of the heap for consistency
void mm_check(int verbose){
    char *bp = heap_listp;

    if (verbose)
	printf("Heap (%p):\n", heap_listp);

    if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
	printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
	if (verbose) 
	    printblock(bp);
	checkblock(bp);
    }

    if (verbose)
	printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
	printf("Bad epilogue header\n");
}
```


## 参考

- [CSAPP | Lab8-Malloc Lab 深入解析 - kccxin的文章 - 知乎](https://zhuanlan.zhihu.com/p/496366818)