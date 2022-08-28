
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size,
    ngx_uint_t align);
static void *ngx_palloc_block(ngx_pool_t *pool, size_t size);
static void *ngx_palloc_large(ngx_pool_t *pool, size_t size);

/**
 * 创建堆，堆的大小为size，log是nginx日志对象
 */
ngx_pool_t *
ngx_create_pool(size_t size, ngx_log_t *log)
{
    ngx_pool_t  *p;
    //申请size大小的内存，如果系统支持内存对齐，则默认申请16字节对齐的地址，
    // 这里的ngx_memalign使用的是posix标准接口来分配内存，关于posix可以百度，这是一个可移植操作系统标准
    //nginx为很多基础类型和函数做了封装，以保证跨平台
    p = ngx_memalign(NGX_POOL_ALIGNMENT, size, log);
    //申请失败则返回NULL
    if (p == NULL) {
        return NULL;
    }
    //p->d 这里的d其实是一个ngx_pool_data_t的结构体，也可以映射为java的对象
    //也就是说在ngx_pool_t这个堆对象里面，有一个属性d，这个d是ngx_pool_data_t类型的
    //ngx_pool_data_t这个结构体或者说这个类型主要用户描述一个小堆，这里的小堆不等于堆
    //实际上ngx_pool_t堆由数个小堆和数个大堆ngx_pool_large_t组成，这里不理解大堆，没关系，稍后会详细讲解大堆
    p->d.last = (u_char *) p + sizeof(ngx_pool_t);
    //小堆的结束位置
    p->d.end = (u_char *) p + size;
    //下一个小堆的指针
    p->d.next = NULL;
    //当前小堆的分配内存失败次数，分配失败是因为，我们要的内存>(d.end-d.last)空闲的内存，那么d.failed就会+1
    p->d.failed = 0;
    //堆区实际可用大小
    size = size - sizeof(ngx_pool_t);
    //堆的最大内存
    p->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;
    //当前内存池的指针
    p->current = p;
    p->chain = NULL;
    p->large = NULL;
    p->cleanup = NULL;
    //日志对象
    p->log = log;
    //返回堆指针
    return p;
}

//释放内存池
void ngx_destroy_pool(ngx_pool_t *pool)
{
    ngx_pool_t          *p, *n;
    ngx_pool_large_t    *l;
    ngx_pool_cleanup_t  *c;
    //按顺序指向销毁时的回调函数
    for (c = pool->cleanup; c; c = c->next) {
        if (c->handler) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "run cleanup: %p", c);
            c->handler(c->data);
        }
    }
//调试时执行
#if (NGX_DEBUG)

    /*
     * we could allocate the pool->log from this pool
     * so we cannot use this log while free()ing the pool
     */

    for (l = pool->large; l; l = l->next) {
        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0, "free: %p", l->alloc);
    }

    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                       "free: %p, unused: %uz", p, p->d.end - p->d.last);

        if (n == NULL) {
            break;
        }
    }

#endif
    //遍历大堆对象，并释放大堆的内存
    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }
    //遍历小堆对象，并释放小堆的内存
    for (p = pool, n = pool->d.next; /* void */; p = n, n = n->d.next) {
        ngx_free(p);

        if (n == NULL) {
            break;
        }
    }
}

/**
 * 重置堆，大堆释放，小堆移动使用指针
 */
void ngx_reset_pool(ngx_pool_t *pool)
{
    ngx_pool_t        *p;
    ngx_pool_large_t  *l;

    for (l = pool->large; l; l = l->next) {
        if (l->alloc) {
            ngx_free(l->alloc);
        }
    }

    for (p = pool; p; p = p->d.next) {
        p->d.last = (u_char *) p + sizeof(ngx_pool_t);
        p->d.failed = 0;
    }

    pool->current = pool;
    pool->chain = NULL;
    pool->large = NULL;
}

/**
 * 地址对齐的方式分配内存
 */
void * ngx_palloc(ngx_pool_t *pool, size_t size){
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 1);
    }
#endif

    return ngx_palloc_large(pool, size);
}

/**
 * 忽略地址对齐的方式分配内存
 */
void *ngx_pnalloc(ngx_pool_t *pool, size_t size){
#if !(NGX_DEBUG_PALLOC)
    if (size <= pool->max) {
        return ngx_palloc_small(pool, size, 0);
    }
#endif

    return ngx_palloc_large(pool, size);
}

//小内存分配,size为申请的内存大小,align表示是否使用对齐地址
//如果所有的内存池都都没有空间分配，则新建一个内存池并追加到内存池最尾部
static ngx_inline void *ngx_palloc_small(ngx_pool_t *pool, size_t size, ngx_uint_t align)
{
    u_char      *m;
    ngx_pool_t  *p;
    //获取当前的内存池
    p = pool->current;
    //从p开始，遍历小堆链表，如果某个小堆可以分配，则返回此小堆空闲空间首地址，作为新分配的的内存开始
    do {
        //获取小堆最后使用的位置
        m = p->d.last;
        //这里会将m变成16的倍数，假设这里m原来的值是56，指向下面宏(不知道宏就当成函数),m就会变成64
        if (align) {
            m = ngx_align_ptr(m, NGX_ALIGNMENT);
        }
        //判断小堆的空闲内存是否大于size，即d.end-m>size
        if ((size_t) (p->d.end - m) >= size) {
            p->d.last = m + size;
            //如果大于则直接返回m
            return m;
        }
        //继续循环
        p = p->d.next;

    } while (p);

    return ngx_palloc_block(pool, size);
}

//这个方法会新建一个个第一个小堆相同大小的小堆，并分配size大小空间返回
static void * ngx_palloc_block(ngx_pool_t *pool, size_t size)
{
    u_char      *m;
    size_t       psize;
    ngx_pool_t  *p, *new;
    //第一个小堆的总大小
    psize = (size_t) (pool->d.end - (u_char *) pool);
    //分配一块和第一个小堆相同大小的空间
    m = ngx_memalign(NGX_POOL_ALIGNMENT, psize, pool->log);
    if (m == NULL) {
        return NULL;
    }

    new = (ngx_pool_t *) m;
    //新的小堆的结束位置
    new->d.end = m + psize;
    new->d.next = NULL;
    new->d.failed = 0;

    m += sizeof(ngx_pool_data_t);
    //地址对齐
    m = ngx_align_ptr(m, NGX_ALIGNMENT);
    //分配size大小空间
    new->d.last = m + size;
    //遍历所有小堆，并将failed++，能到这里都是因为没有小堆可以分配内存
    //分配失败超过4次的，都会将堆的当前可用小堆指向下一个，以寻求空余的小堆内存
    for (p = pool->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool->current = p->d.next;
        }
    }
    //将新的小堆放到末尾
    p->d.next = new;

    return m;
}

/**
 * 分配一个大的堆
 */
static void * ngx_palloc_large(ngx_pool_t *pool, size_t size)
{
    void              *p;
    ngx_uint_t         n;
    ngx_pool_large_t  *large;
    //申请size大小的内存
    p = ngx_alloc(size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    n = 0;
    //遍历大内存堆对象
    for (large = pool->large; large; large = large->next) {
        //如果大堆对象没有分配实际堆内存，则将上面刚分配的内存作为实际堆内存，并返回
        if (large->alloc == NULL) {
            large->alloc = p;
            return p;
        }
        //如果遍历了3个大堆对象都存在实际堆内存，则结束当前循环
        if (n++ > 3) {
            break;
        }
    }
    //向小堆内存池分配ngx_pool_large_t结构体大小的内存
    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    //如果分配失败，则释放原来申请的大堆内存
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }
    //指向大堆地址
    large->alloc = p;
    //将当前新建的大堆对象作为内存池的头节点
    large->next = pool->large;
    pool->large = large;

    return p;
}


void *
ngx_pmemalign(ngx_pool_t *pool, size_t size, size_t alignment)
{
    void              *p;
    ngx_pool_large_t  *large;

    p = ngx_memalign(alignment, size, pool->log);
    if (p == NULL) {
        return NULL;
    }

    large = ngx_palloc_small(pool, sizeof(ngx_pool_large_t), 1);
    if (large == NULL) {
        ngx_free(p);
        return NULL;
    }

    large->alloc = p;
    large->next = pool->large;
    pool->large = large;

    return p;
}

//释放大堆内存，p为需要释放的大堆内存地址
ngx_int_t ngx_pfree(ngx_pool_t *pool, void *p)
{
    ngx_pool_large_t  *l;
    //遍历大堆内存对象
    for (l = pool->large; l; l = l->next) {
        //如果大堆内存地址等于传入的p的地址，则释放这个大堆内存
        if (p == l->alloc) {
            ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, pool->log, 0,
                           "free: %p", l->alloc);
            //将大堆内存释放，并置为NULL
            ngx_free(l->alloc);
            l->alloc = NULL;

            return NGX_OK;
        }
    }

    return NGX_DECLINED;
}


void *
ngx_pcalloc(ngx_pool_t *pool, size_t size)
{
    void *p;

    p = ngx_palloc(pool, size);
    if (p) {
        ngx_memzero(p, size);
    }

    return p;
}


ngx_pool_cleanup_t *
ngx_pool_cleanup_add(ngx_pool_t *p, size_t size)
{
    ngx_pool_cleanup_t  *c;

    c = ngx_palloc(p, sizeof(ngx_pool_cleanup_t));
    if (c == NULL) {
        return NULL;
    }

    if (size) {
        c->data = ngx_palloc(p, size);
        if (c->data == NULL) {
            return NULL;
        }

    } else {
        c->data = NULL;
    }

    c->handler = NULL;
    c->next = p->cleanup;

    p->cleanup = c;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, p->log, 0, "add cleanup: %p", c);

    return c;
}


void
ngx_pool_run_cleanup_file(ngx_pool_t *p, ngx_fd_t fd)
{
    ngx_pool_cleanup_t       *c;
    ngx_pool_cleanup_file_t  *cf;

    for (c = p->cleanup; c; c = c->next) {
        if (c->handler == ngx_pool_cleanup_file) {

            cf = c->data;

            if (cf->fd == fd) {
                c->handler(cf);
                c->handler = NULL;
                return;
            }
        }
    }
}


void
ngx_pool_cleanup_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d",
                   c->fd);

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


void
ngx_pool_delete_file(void *data)
{
    ngx_pool_cleanup_file_t  *c = data;

    ngx_err_t  err;

    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, c->log, 0, "file cleanup: fd:%d %s",
                   c->fd, c->name);

    if (ngx_delete_file(c->name) == NGX_FILE_ERROR) {
        err = ngx_errno;

        if (err != NGX_ENOENT) {
            ngx_log_error(NGX_LOG_CRIT, c->log, err,
                          ngx_delete_file_n " \"%s\" failed", c->name);
        }
    }

    if (ngx_close_file(c->fd) == NGX_FILE_ERROR) {
        ngx_log_error(NGX_LOG_ALERT, c->log, ngx_errno,
                      ngx_close_file_n " \"%s\" failed", c->name);
    }
}


#if 0

static void *
ngx_get_cached_block(size_t size)
{
    void                     *p;
    ngx_cached_block_slot_t  *slot;

    if (ngx_cycle->cache == NULL) {
        return NULL;
    }

    slot = &ngx_cycle->cache[(size + ngx_pagesize - 1) / ngx_pagesize];

    slot->tries++;

    if (slot->number) {
        p = slot->block;
        slot->block = slot->block->next;
        slot->number--;
        return p;
    }

    return NULL;
}

#endif
