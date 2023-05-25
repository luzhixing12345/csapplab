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
static void *coalesce(void *bp);         // 合并空闲块

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
    return coalesce(bp);
}

void *coalesce(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));
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
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
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
            // 永远不应该到达这里...
            printf("%p\n", heap_listp);
        }
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

    // 小于等于 DSIZE 的大小直接补齐 3*DSIZE
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
    coalesce(bp);
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
        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) && (GET_SIZE(HDRP(NEXT_BLKP(ptr))) >= (size - block_size))) {
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