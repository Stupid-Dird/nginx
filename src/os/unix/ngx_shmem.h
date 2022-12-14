
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_SHMEM_H_INCLUDED_
#define _NGX_SHMEM_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

//共享内存块对象
typedef struct {
    //共享内存块首地址
    u_char      *addr;
    //共享内存块大小
    size_t       size;
    //共享内存块名称
    ngx_str_t    name;
    //日志对象
    ngx_log_t   *log;
    //标识是否已经存在
    ngx_uint_t   exists;   /* unsigned  exists:1;  */
} ngx_shm_t;

//申请共享内存
ngx_int_t ngx_shm_alloc(ngx_shm_t *shm);
//释放共享内存
void ngx_shm_free(ngx_shm_t *shm);


#endif /* _NGX_SHMEM_H_INCLUDED_ */
