
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_SLAB_H_INCLUDED_
#define _NGX_SLAB_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_slab_page_s  ngx_slab_page_t;

//页管理结构体
struct ngx_slab_page_s {
    //当前共享内存
    uintptr_t         slab;
    //下一页管理结构体指针
    ngx_slab_page_t  *next;
    //上一个页
    uintptr_t         prev;
};

//内存页统计信息
typedef struct {
    //总大小
    ngx_uint_t        total;
    //已使用的大小
    ngx_uint_t        used;
    //要求大小
    ngx_uint_t        reqs;
    //分配内存失败次数
    ngx_uint_t        fails;
} ngx_slab_stat_t;

//内存池结构体
typedef struct {
    //互斥锁
    ngx_shmtx_sh_t    lock;
    //可分配的最小内存
    size_t            min_size;
    //最小内存的对应的偏移值（3代表min_size为2^3）
    size_t            min_shift;
    //指向第一页的管理结构
    ngx_slab_page_t  *pages;
    //指向最后一页的管理结构
    ngx_slab_page_t  *last;
    //用于管理空闲页面
    ngx_slab_page_t   free;
    //记录每种规格内存统计信息(小块内存)
    ngx_slab_stat_t  *stats;
    //空闲页数
    ngx_uint_t        pfree;

    u_char           *start;
    u_char           *end;

    ngx_shmtx_t       mutex;

    u_char           *log_ctx;
    u_char            zero;

    unsigned          log_nomem:1;

    void             *data;
    void             *addr;
} ngx_slab_pool_t;


void ngx_slab_sizes_init(void);
void ngx_slab_init(ngx_slab_pool_t *pool);
void *ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_calloc_locked(ngx_slab_pool_t *pool, size_t size);
void ngx_slab_free(ngx_slab_pool_t *pool, void *p);
void ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p);


#endif /* _NGX_SLAB_H_INCLUDED_ */
