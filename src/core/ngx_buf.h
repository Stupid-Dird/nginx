
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_BUF_H_INCLUDED_
#define _NGX_BUF_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef void *            ngx_buf_tag_t;

typedef struct ngx_buf_s  ngx_buf_t;

/**
 * 这是一个数据缓冲区头
 * 可以理解为一个写入或者读取寻址器，自身并没有分配空间的能力，只能用来描述某一段内存或文件
 * 可以通过ngx_pool_t,来分配一块内存，然后通过ngx_buf_s来描述
 */
struct ngx_buf_s {
    //待处理的数据开始位置
    u_char          *pos;
    //下一个处理的数据位置
    u_char          *last;
    //处理文件时，待处理的文件开始标记
    off_t            file_pos;
    //处理文件时，待处理的文件结尾标记
    off_t            file_last;
    //缓冲区开始的指针地址
    u_char          *start;         /* start of buffer */
    //缓冲区结尾的指针地址
    u_char          *end;           /* end of buffer */
    //表示当前缓冲区的类型
    ngx_buf_tag_t    tag;
    // 引用的文件
    ngx_file_t      *file;
    //影子指向，表示接收到的上游服务器响应后，在向下游客户端转发时可能会把这块内存存储到文件中，也可能直接向下游发送，此时
    //Nginx绝不会重新复制一份内存用于新的目的，而是再次建立一个
    //ngx_buf_t结构体指向原内存，这样多个
    //ngx_buf_t结构体指向了同一块内存，它们之间的关系就通过，shadow成员来引用。这种设计过于复杂，通常不建议使用
    ngx_buf_t       *shadow;


    /* the buf's content could be changed */
    //1时表示数据在内存中且这段内存可以修改
    unsigned         temporary:1;

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed
     */
    //1时表示数据在内存中且这段内存不可以被修改
    unsigned         memory:1;

    /* the buf's content is mmap()ed and must not be changed */
    //1时表示这段内存是用
    //mmap系统调用映射过来的，不可以被修改
    unsigned         mmap:1;
    //1时表示可回收
    unsigned         recycled:1;
    //1时表示这段缓冲区处理的是文件而不是内存
    unsigned         in_file:1;
    //1时表示需要执行flush操作
    unsigned         flush:1;
    //标志位，对于操作这块缓冲区时是否使用同步方式，需谨慎考虑，这可能会阻塞Nginx进程
    //sync为
    //1时可能会有阻塞的方式进行
    //I/O操作，它的意义视使用它的
    //Nginx模块而定
    unsigned         sync:1;
    //标志位，表示是否是最后一块缓冲区，因为ngx_buf_t可以由ngx_chain_t链表串联起来，因此，当last_buf为1时，表示当前是最后一块待处理的缓冲区
    unsigned         last_buf:1;
    //标志位，表示是否是ngx_chain_t中的最后一块缓冲区
    unsigned         last_in_chain:1;
    //标志位，表示是否是最后一个影子缓冲区，与shadow域配合使用。通常不建议使用它
    unsigned         last_shadow:1;
    //标志位，表示当前缓冲区是否属于临时文件
    unsigned         temp_file:1;

    /* STUB */ int   num;
};

//ngx_buf作为链表节点时的包装结构
struct ngx_chain_s {
    //指向缓存区
    ngx_buf_t    *buf;
    //下一个缓存区节点
    ngx_chain_t  *next;
};


typedef struct {
    ngx_int_t    num;
    size_t       size;
} ngx_bufs_t;


typedef struct ngx_output_chain_ctx_s  ngx_output_chain_ctx_t;

typedef ngx_int_t (*ngx_output_chain_filter_pt)(void *ctx, ngx_chain_t *in);

typedef void (*ngx_output_chain_aio_pt)(ngx_output_chain_ctx_t *ctx,
    ngx_file_t *file);

struct ngx_output_chain_ctx_s {
    ngx_buf_t                   *buf;
    ngx_chain_t                 *in;
    ngx_chain_t                 *free;
    ngx_chain_t                 *busy;

    unsigned                     sendfile:1;
    unsigned                     directio:1;
    unsigned                     unaligned:1;
    unsigned                     need_in_memory:1;
    unsigned                     need_in_temp:1;
    unsigned                     aio:1;

#if (NGX_HAVE_FILE_AIO || NGX_COMPAT)
    ngx_output_chain_aio_pt      aio_handler;
#endif

#if (NGX_THREADS || NGX_COMPAT)
    ngx_int_t                  (*thread_handler)(ngx_thread_task_t *task,
                                                 ngx_file_t *file);
    ngx_thread_task_t           *thread_task;
#endif

    off_t                        alignment;

    ngx_pool_t                  *pool;
    ngx_int_t                    allocated;
    ngx_bufs_t                   bufs;
    ngx_buf_tag_t                tag;

    ngx_output_chain_filter_pt   output_filter;
    void                        *filter_ctx;
};


typedef struct {
    ngx_chain_t                 *out;
    ngx_chain_t                **last;
    ngx_connection_t            *connection;
    ngx_pool_t                  *pool;
    off_t                        limit;
} ngx_chain_writer_ctx_t;


#define NGX_CHAIN_ERROR     (ngx_chain_t *) NGX_ERROR


#define ngx_buf_in_memory(b)       ((b)->temporary || (b)->memory || (b)->mmap)
#define ngx_buf_in_memory_only(b)  (ngx_buf_in_memory(b) && !(b)->in_file)

#define ngx_buf_special(b)                                                   \
    (((b)->flush || (b)->last_buf || (b)->sync)                              \
     && !ngx_buf_in_memory(b) && !(b)->in_file)

#define ngx_buf_sync_only(b)                                                 \
    ((b)->sync && !ngx_buf_in_memory(b)                                      \
     && !(b)->in_file && !(b)->flush && !(b)->last_buf)

#define ngx_buf_size(b)                                                      \
    (ngx_buf_in_memory(b) ? (off_t) ((b)->last - (b)->pos):                  \
                            ((b)->file_last - (b)->file_pos))

ngx_buf_t *ngx_create_temp_buf(ngx_pool_t *pool, size_t size);
ngx_chain_t *ngx_create_chain_of_bufs(ngx_pool_t *pool, ngx_bufs_t *bufs);


#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

ngx_chain_t *ngx_alloc_chain_link(ngx_pool_t *pool);
#define ngx_free_chain(pool, cl)                                             \
    (cl)->next = (pool)->chain;                                              \
    (pool)->chain = (cl)



ngx_int_t ngx_output_chain(ngx_output_chain_ctx_t *ctx, ngx_chain_t *in);
ngx_int_t ngx_chain_writer(void *ctx, ngx_chain_t *in);

ngx_int_t ngx_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain,
    ngx_chain_t *in);
ngx_chain_t *ngx_chain_get_free_buf(ngx_pool_t *p, ngx_chain_t **free);
void ngx_chain_update_chains(ngx_pool_t *p, ngx_chain_t **free,
    ngx_chain_t **busy, ngx_chain_t **out, ngx_buf_tag_t tag);

off_t ngx_chain_coalesce_file(ngx_chain_t **in, off_t limit);

ngx_chain_t *ngx_chain_update_sent(ngx_chain_t *in, off_t sent);

#endif /* _NGX_BUF_H_INCLUDED_ */
