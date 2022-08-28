
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include "ngx_palloc.h"


typedef struct ngx_list_part_s  ngx_list_part_t;

//nList节点结构体
struct ngx_list_part_s {
    //元素空间，一个连续空间的数组
    void             *elts;
    //已经存储的元素个数
    ngx_uint_t        nelts;
    //下一个节点指针
    ngx_list_part_t  *next;
};

//nList结构体
typedef struct {
    //尾结点指针
    ngx_list_part_t  *last;
    //头节点指针
    ngx_list_part_t   part;
    //存放的元素的最大大小
    size_t            size;
    //每个节点能存放的最大元素个数
    ngx_uint_t        nalloc;
    //nList的节点都是内存连续的，其内存由pool来进行分配和管理
    ngx_pool_t       *pool;
} ngx_list_t;

//extern void* ngx_palloc(ngx_pool_t *pool, size_t size) ;

ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

static ngx_inline ngx_int_t
/**
 * nList的初始化函数
 * 初始化时会创建一个头节点，所以不是一个空列表
 */
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    //创建一个头结点
    list->part.elts = ngx_palloc(pool, n * size);
    //如果头结点等于NULL则创建失败
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }
    //初始化头节点结构体中的属性
    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}


/*
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *
 *  for (i = 0 ;; i++) {
 *
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;
 *          }
 *
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *
 *      ...  data[i] ...
 *
 *  }
 */


void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
