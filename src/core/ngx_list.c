
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>

/**
 * 创建并初始化ngx_list_t
 */
extern
ngx_list_t *
ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    ngx_list_t  *list;

    list = ngx_palloc(pool, sizeof(ngx_list_t));
    if (list == NULL) {
        return NULL;
    }

    if (ngx_list_init(list, pool, n, size) != NGX_OK) {
        return NULL;
    }

    return list;
}

/**
 * 返回列表可用的空间地址
 */
void *
ngx_list_push(ngx_list_t *l)
{
    void             *elt;
    ngx_list_part_t  *last;

    last = l->last;
    //如果尾节点元素个数等于最大个数，说明已经满了
    if (last->nelts == l->nalloc) {

        /* the last part is full, allocate a new list part */
        //创建节点结构体
        last = ngx_palloc(l->pool, sizeof(ngx_list_part_t));
        if (last == NULL) {
            return NULL;
        }
        //分配元素空间
        last->elts = ngx_palloc(l->pool, l->nalloc * l->size);
        if (last->elts == NULL) {
            return NULL;
        }
        //初始化
        last->nelts = 0;
        last->next = NULL;

        l->last->next = last;
        l->last = last;
    }
    //返回第一个可用空间地址
    elt = (char *) last->elts + l->size * last->nelts;
    last->nelts++;

    return elt;
}
