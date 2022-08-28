
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_PALLOC_H_INCLUDED_
#define _NGX_PALLOC_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * NGX_MAX_ALLOC_FROM_POOL should be (ngx_pagesize - 1), i.e. 4095 on x86.
 * On Windows NT it decreases a number of locked pages in a kernel.
 */
#define NGX_MAX_ALLOC_FROM_POOL  (ngx_pagesize - 1)

#define NGX_DEFAULT_POOL_SIZE    (16 * 1024)

#define NGX_POOL_ALIGNMENT       16
#define NGX_MIN_POOL_SIZE                                                     \
    ngx_align((sizeof(ngx_pool_t) + 2 * sizeof(ngx_pool_large_t)),            \
              NGX_POOL_ALIGNMENT)

//内存池销毁时的回调函数
typedef void (*ngx_pool_cleanup_pt)(void *data);

typedef struct ngx_pool_cleanup_s  ngx_pool_cleanup_t;

//内存池销毁时的回调函数链表
struct ngx_pool_cleanup_s {
    //内存池销毁时的回调函数
    ngx_pool_cleanup_pt   handler;
    //函数参数
    void                 *data;
    //下一个回调函数
    ngx_pool_cleanup_t   *next;
};


typedef struct ngx_pool_large_s  ngx_pool_large_t;

//大内存堆对象结构体
struct ngx_pool_large_s {
    //用户构成链表
    ngx_pool_large_t     *next;
    //指向真正的大块内存
    void                 *alloc;
};

//内存池小堆的结构体，只用于小内存的分配
typedef struct {
    //内存池最后使用的位置，也就是空闲的内存开始位置
    u_char               *last;
    //内存池内存区域的结束位置
    u_char               *end;
    //下一个内存池对象
    ngx_pool_t           *next;
    //从当前堆分配内存失败的次数,如果次数超过4次，则下次将不再尝试从此内存池分配内存
    ngx_uint_t            failed;
} ngx_pool_data_t;

//堆
struct ngx_pool_s {
    //小堆内存头节点
    ngx_pool_data_t       d;
    //小堆的最大空间，max会用来划分分配的空间在小堆还是大堆，超过max的则在大堆分配
    size_t                max;
    //当前内存池的指针
    ngx_pool_t           *current;
    ngx_chain_t          *chain;
    //大堆内存对象头节点
    ngx_pool_large_t     *large;
    //内存池销毁时的回调函数链表
    ngx_pool_cleanup_t   *cleanup;
    //日志对象
    ngx_log_t            *log;
};


typedef struct {
    ngx_fd_t              fd;
    u_char               *name;
    ngx_log_t            *log;
} ngx_pool_cleanup_file_t;


ngx_pool_t *ngx_create_pool(size_t size, ngx_log_t *log);
void ngx_destroy_pool(ngx_pool_t *pool);
void ngx_reset_pool(ngx_pool_t *pool);

void *ngx_palloc(ngx_pool_t *pool, size_t size);
void *ngx_pnalloc(ngx_pool_t *pool, size_t size);
void *ngx_pcalloc(ngx_pool_t *pool, size_t size);
void *ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment);
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p);


ngx_pool_cleanup_t *ngx_pool_cleanup_add(ngx_pool_t *p, size_t size);
void ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd);
void ngx_pool_cleanup_file(void *data);
void ngx_pool_delete_file(void *data);


#endif /* _NGX_PALLOC_H_INCLUDED_ */
