
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HTTP_CONFIG_H_INCLUDED_
#define _NGX_HTTP_CONFIG_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


typedef struct {
    void        **main_conf;
    void        **srv_conf;
    void        **loc_conf;
} ngx_http_conf_ctx_t;

/**
 * http模块ctx
 */
typedef struct {
    //解析配置文件前调用
    ngx_int_t   (*preconfiguration)(ngx_conf_t *cf);
    //完成配置文件的解析后调用
    ngx_int_t   (*postconfiguration)(ngx_conf_t *cf);
    /**
     * 当需要创建数据结构用户存储main级别(直属http{}块的配置项)的全局配置时，可以
     * 通过调用create_main_conf回调方法创建存储全局配置得结构体
     */
    void       *(*create_main_conf)(ngx_conf_t *cf);
    //常用户初始化main级别的配置项
    char       *(*init_main_conf)(ngx_conf_t *cf, void *conf);
    /**
     * 当需要创建数据结构用户存储src级别(直属于虚拟主机server{...}快的配置项)的配置项时，
     * 可以通过实现create_srv_conf回调方法创建存储src级别配置项的额结构体
     */
    void       *(*create_srv_conf)(ngx_conf_t *cf);
    //merge_srv_conf用于合并main级别和srv级别下的同名配置
    char       *(*merge_srv_conf)(ngx_conf_t *cf, void *prev, void *conf);
    /**
     * 当需要创建数据结构用户存储loc级别(直属于location{...}块的配置项)的配置项时，可以实现create_loc_conf回调方法
     */
    void       *(*create_loc_conf)(ngx_conf_t *cf);
    //用于合并srv级别和loc级别下的同名配置
    char       *(*merge_loc_conf)(ngx_conf_t *cf, void *prev, void *conf);
} ngx_http_module_t;


#define NGX_HTTP_MODULE           0x50545448   /* "HTTP" */

#define NGX_HTTP_MAIN_CONF        0x02000000
#define NGX_HTTP_SRV_CONF         0x04000000
#define NGX_HTTP_LOC_CONF         0x08000000
#define NGX_HTTP_UPS_CONF         0x10000000
#define NGX_HTTP_SIF_CONF         0x20000000
#define NGX_HTTP_LIF_CONF         0x40000000
#define NGX_HTTP_LMT_CONF         0x80000000


#define NGX_HTTP_MAIN_CONF_OFFSET  offsetof(ngx_http_conf_ctx_t, main_conf)
#define NGX_HTTP_SRV_CONF_OFFSET   offsetof(ngx_http_conf_ctx_t, srv_conf)
#define NGX_HTTP_LOC_CONF_OFFSET   offsetof(ngx_http_conf_ctx_t, loc_conf)


#define ngx_http_get_module_main_conf(r, module)                             \
    (r)->main_conf[module.ctx_index]
#define ngx_http_get_module_srv_conf(r, module)  (r)->srv_conf[module.ctx_index]
#define ngx_http_get_module_loc_conf(r, module)  (r)->loc_conf[module.ctx_index]


#define ngx_http_conf_get_module_main_conf(cf, module)                        \
    ((ngx_http_conf_ctx_t *) cf->ctx)->main_conf[module.ctx_index]
#define ngx_http_conf_get_module_srv_conf(cf, module)                         \
    ((ngx_http_conf_ctx_t *) cf->ctx)->srv_conf[module.ctx_index]
#define ngx_http_conf_get_module_loc_conf(cf, module)                         \
    ((ngx_http_conf_ctx_t *) cf->ctx)->loc_conf[module.ctx_index]

#define ngx_http_cycle_get_module_main_conf(cycle, module)                    \
    (cycle->conf_ctx[ngx_http_module.index] ?                                 \
        ((ngx_http_conf_ctx_t *) cycle->conf_ctx[ngx_http_module.index])      \
            ->main_conf[module.ctx_index]:                                    \
        NULL)


#endif /* _NGX_HTTP_CONFIG_H_INCLUDED_ */
