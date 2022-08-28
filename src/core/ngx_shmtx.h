
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_SHMTX_H_INCLUDED_
#define _NGX_SHMTX_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

//原子锁结构体
typedef struct {
    ngx_atomic_t   lock;
#if (NGX_HAVE_POSIX_SEM)
    ngx_atomic_t   wait;
#endif
} ngx_shmtx_sh_t;

//文件锁结构体
typedef struct {
    //如果支持原子锁，则lock指向ngx_shmtx_sh_t
#if (NGX_HAVE_ATOMIC_OPS)
    ngx_atomic_t  *lock;
#if (NGX_HAVE_POSIX_SEM)
    ngx_atomic_t  *wait;
    ngx_uint_t     semaphore;
    sem_t          sem;
#endif
#else
    //如果不支持原子锁则使用文件锁
    ngx_fd_t       fd;
    u_char        *name;
#endif
    ngx_uint_t     spin;
} ngx_shmtx_t;

//创建锁
ngx_int_t ngx_shmtx_create(ngx_shmtx_t *mtx, ngx_shmtx_sh_t *addr,
    u_char *name);
//销毁锁
void ngx_shmtx_destroy(ngx_shmtx_t *mtx);
//尝试加锁
ngx_uint_t ngx_shmtx_trylock(ngx_shmtx_t *mtx);
//加锁直到成功后返回
void ngx_shmtx_lock(ngx_shmtx_t *mtx);
//释放锁
void ngx_shmtx_unlock(ngx_shmtx_t *mtx);
//强制解锁
ngx_uint_t ngx_shmtx_force_unlock(ngx_shmtx_t *mtx, ngx_pid_t pid);


#endif /* _NGX_SHMTX_H_INCLUDED_ */
