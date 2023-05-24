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
