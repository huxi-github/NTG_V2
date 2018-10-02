/*
 * ntg_config_file.c
 *
 *  Created on: Aug 18, 2015
 *      Author: tzh
 */

#include "../ntg_config.h"
#include "../ntg_core.h"

#include "ntg_cycle.h"
#include "ntg_string.h"
#include "ntg_queue.h"
#include "ntg_parse.h"
#include "ntg_time.h"
#include "ntg_parse.h"
#include "ntg_files.h"

#define NTG_CONF_BUFFER  4096///< 配置buf大小

static ntg_int_t ntg_conf_handler(ntg_conf_t *cf, ntg_int_t last);
static ntg_int_t ntg_conf_read_token(ntg_conf_t *cf);
static void ntg_conf_flush_files(ntg_cycle_t *cycle);

///配置模块命令
static ntg_command_t  ntg_conf_commands[] = {

    { ntg_string("include"),
      NTG_ANY_CONF|NTG_CONF_TAKE1,
      ntg_conf_include,
      0,
      0,
      NULL },

      ntg_null_command
};

///配置模块
ntg_module_t  ntg_conf_module = {
    NTG_MODULE_V1,
    NULL,                                  /* module context */
    ntg_conf_commands,                     /* module directives */
    NTG_CONF_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    ntg_conf_flush_files,                  /* exit process */
    NULL,                                  /* exit master */
    NTG_MODULE_V1_PADDING
};


/* The eight fixed arguments */
/**
 * @name 固定参数标志数组
 * @{
 */
static ntg_uint_t argument_number[] = {
    NTG_CONF_NOARGS,	///< 没0个参数
    NTG_CONF_TAKE1,		///< 1个参数
    NTG_CONF_TAKE2,		///< 2个参数
    NTG_CONF_TAKE3,		///< 3个参数
    NTG_CONF_TAKE4,		///< 4个参数
    NTG_CONF_TAKE5,		///< 5个参数
    NTG_CONF_TAKE6,		///< 6个参数
    NTG_CONF_TAKE7		///< 7个参数
};
/**@}*/

/**
 * 配置参数获取
 * @param[in] cf 配置对象
 * @return 成功返回NTG_CONF_OK
 * @note 如果存在-g输入参数,还完成了对该参数的解析.
 */
char *
ntg_conf_param(ntg_conf_t *cf)
{
    char             *rv;
    ntg_str_t        *param;
    ntg_buf_t         b;
    ntg_conf_file_t   conf_file;

    param = &cf->cycle->conf_param;

    if (param->len == 0) {
        return NTG_CONF_OK;
    }

    //分配并初始化一个配置文件对象
    ntg_memzero(&conf_file, sizeof(ntg_conf_file_t));

    ntg_memzero(&b, sizeof(ntg_buf_t));

    b.start = param->data;
    b.pos = param->data;
    b.last = param->data + param->len;
    b.end = b.last;
    b.temporary = 1;

    conf_file.file.fd = NTG_INVALID_FILE;
    conf_file.file.name.data = NULL;
    conf_file.line = 0;

    cf->conf_file = &conf_file;
    cf->conf_file->buffer = &b;

    rv = ntg_conf_parse(cf, NULL);//表示有配置参数的情况

    cf->conf_file = NULL;

    return rv;
}

/**
 * 配置文件的解析
 * @param[in] cf 配置对象
 * @param[in] filename 配置文件
 * @return 成功返回NTG_CONF_OK, 否则返回NTG_CONF_ERROR
 * @note 当filename == NULL时,对输入的-g参数进行解析
 */
char *
ntg_conf_parse(ntg_conf_t *cf, ntg_str_t *filename)
{
    char             *rv;
    ntg_fd_t          fd;
    ntg_int_t         rc;
    ntg_buf_t         buf;
    ntg_conf_file_t  *prev, conf_file;
    enum {
        parse_file = 0,
        parse_block,
        parse_param
    } type;//解析对象的类型

#if (NTG_SUPPRESS_WARN)
    fd = NTG_INVALID_FILE;
    prev = NULL;
#endif

    if (filename) {

        /* open configuration file */

        fd = ntg_open_file(filename->data, NTG_FILE_RDONLY, NTG_FILE_OPEN, 0);
        if (fd == NTG_INVALID_FILE) {
            ntg_conf_log_error(NTG_LOG_EMERG, cf, ntg_errno,
                               ntg_open_file_n " \"%s\" failed",
                               filename->data);
            return NTG_CONF_ERROR;
        }

        prev = cf->conf_file;

        cf->conf_file = &conf_file;
        //获取配置文件信息
        if (ntg_fd_info(fd, &cf->conf_file->file.info) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_EMERG, cf->log, ntg_errno,
                          ntg_fd_info_n " \"%s\" failed", filename->data);
        }
        //初始化配置文件的buffer
        cf->conf_file->buffer = &buf;

        buf.start = ntg_alloc(NTG_CONF_BUFFER, cf->log);
        if (buf.start == NULL) {
            goto failed;
        }

        buf.pos = buf.start;
        buf.last = buf.start;
        buf.end = buf.last + NTG_CONF_BUFFER;
        buf.temporary = 1;//临时分配的空间
        //初始化配置文件属性
        cf->conf_file->file.fd = fd;
        cf->conf_file->file.name.len = filename->len;
        cf->conf_file->file.name.data = filename->data;
        cf->conf_file->file.offset = 0;
        cf->conf_file->file.log = cf->log;
        cf->conf_file->line = 1;

        type = parse_file;

    } else if (cf->conf_file->file.fd != NTG_INVALID_FILE) {

        type = parse_block;

    } else {
        type = parse_param;
    }


    for ( ;; ) {
        rc = ntg_conf_read_token(cf);//读取一个标签

        /*
         * ntg_conf_read_token() may return
         *
         *    NTG_ERROR             there is error
         *    NTG_OK                the token terminated by ";" was found
         *    NTG_CONF_BLOCK_START  the token terminated by "{" was found
         *    NTG_CONF_BLOCK_DONE   the "}" was found
         *    NTG_CONF_FILE_DONE    the configuration file is done
         */

        if (rc == NTG_ERROR) {
            goto done;
        }

        if (rc == NTG_CONF_BLOCK_DONE) {

            if (type != parse_block) {
                ntg_conf_log_error(NTG_LOG_EMERG, cf, 0, "unexpected \"}\"");
                goto failed;
            }

            goto done;
        }

        if (rc == NTG_CONF_FILE_DONE) {

            if (type == parse_block) {
                ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                   "unexpected end of file, expecting \"}\"");
                goto failed;
            }

            goto done;
        }

        if (rc == NTG_CONF_BLOCK_START) {

            if (type == parse_param) {
                ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                   "block directives are not supported "
                                   "in -g option");
                goto failed;
            }
        }

        /* rc == NTG_OK || rc == NTG_CONF_BLOCK_START */

        if (cf->handler) {//指定有配置处理函数

            /*
             * the custom handler, i.e., that is used in the http's
             * "types { ... }" directive
             */

            if (rc == NTG_CONF_BLOCK_START) {
                ntg_conf_log_error(NTG_LOG_EMERG, cf, 0, "unexpected \"{\"");
                goto failed;
            }

            rv = (*cf->handler)(cf, NULL, cf->handler_conf);
            if (rv == NTG_CONF_OK) {
                continue;
            }

            if (rv == NTG_CONF_ERROR) {
                goto failed;
            }

            ntg_conf_log_error(NTG_LOG_EMERG, cf, 0, rv);

            goto failed;
        }

        //默认处理函数
        rc = ntg_conf_handler(cf, rc);

        if (rc == NTG_ERROR) {
            goto failed;
        }
    }

failed:

    rc = NTG_ERROR;

done:

    if (filename) {
        if (cf->conf_file->buffer->start) {
            ntg_free(cf->conf_file->buffer->start);
        }

        if (ntg_close_file(fd) == NTG_FILE_ERROR) {
            ntg_log_error(NTG_LOG_ALERT, cf->log, ntg_errno,
                          ntg_close_file_n " %s failed",
                          filename->data);
            rc = NTG_ERROR;
        }

        cf->conf_file = prev;//重置之前的配置文件
    }

    if (rc == NTG_ERROR) {
        return NTG_CONF_ERROR;
    }

    return NTG_CONF_OK;
}

/**
 * 默认配置处理函数
 * @param[in] cf 配置对象
 * @param[in] last ntg_conf_read_token的返回值
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 * @note 对找到的标签进行进行匹配处理. 遍历所有模块的所有命令,查找与之匹配的项.
 */
static ntg_int_t
ntg_conf_handler(ntg_conf_t *cf, ntg_int_t last)
{
    char           *rv;
    void           *conf, **confp;
    ntg_uint_t      i, found;
    ntg_str_t      *name;
    ntg_command_t  *cmd;

    name = cf->args->elts;
//    printf("-----%s-ntg_conf_handler---\n",name->data);
    found = 0;
    //遍历所有的模块
    for (i = 0; ntg_modules[i]; i++) {

        cmd = ntg_modules[i]->commands;
        if (cmd == NULL) {
            continue;
        }
        //遍历模块所有的命令
        for ( /* void */ ; cmd->name.len; cmd++) {
        	/*1)测试是否匹配*/
        	//检查命令是否相同
            if (name->len != cmd->name.len) {
                continue;
            }

            if (ntg_strcmp(name->data, cmd->name.data) != 0) {
                continue;
            }

            found = 1;
            /*2)测试有效性*/
            //模块类型是否一致,conf_module直接跳过
            if (ntg_modules[i]->type != NTG_CONF_MODULE
                && ntg_modules[i]->type != cf->module_type)
            {
            	//不是配置模块,且模块类型不匹配
                continue;
            }

            /* is the directive's location right ? */

            if (!(cmd->type & cf->cmd_type)) {//命令类型是否一致
                continue;
            }

            if (!(cmd->type & NTG_CONF_BLOCK) && last != NTG_OK) {
            	//命令不是配置块或不完整
                ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                  "directive \"%s\" is not terminated by \";\"",
                                  name->data);
                return NTG_ERROR;
            }

            if ((cmd->type & NTG_CONF_BLOCK) && last != NTG_CONF_BLOCK_START) {
                ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                   "directive \"%s\" has no opening \"{\"",
                                   name->data);
                return NTG_ERROR;
            }

            /* is the directive's argument count right ? *///指令的参数数量是否正确

            if (!(cmd->type & NTG_CONF_ANY)) {

                if (cmd->type & NTG_CONF_FLAG) {

                    if (cf->args->nelts != 2) {
                        goto invalid;
                    }

                } else if (cmd->type & NTG_CONF_1MORE) {

                    if (cf->args->nelts < 2) {
                        goto invalid;
                    }

                } else if (cmd->type & NTG_CONF_2MORE) {

                    if (cf->args->nelts < 3) {
                        goto invalid;
                    }

                } else if (cf->args->nelts > NTG_CONF_MAX_ARGS) {

                    goto invalid;

                } else if (!(cmd->type & argument_number[cf->args->nelts - 1])){

                    goto invalid;
                }
            }// end of  if (!(cmd->type & NTG_CONF_ANY))

            /* set up the directive's configuration context */

            conf = NULL;

            if (cmd->type & NTG_DIRECT_CONF) {
                conf = ((void **) cf->ctx)[ntg_modules[i]->index];

            } else if (cmd->type & NTG_MAIN_CONF) {
                conf = &(((void **) cf->ctx)[ntg_modules[i]->index]);

            } else if (cf->ctx) {//内部模块配置
                confp = *(void **) ((char *) cf->ctx + cmd->conf);

                if (confp) {
                    conf = confp[ntg_modules[i]->ctx_index];
                }
            }

            rv = cmd->set(cf, cmd, conf);

            if (rv == NTG_CONF_OK) {
                return NTG_OK;
            }

            if (rv == NTG_CONF_ERROR) {
                return NTG_ERROR;
            }

            ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                               "\"%s\" directive %s", name->data, rv);

            return NTG_ERROR;
        }// end of for ( /* void */ ; cmd->name.len; cmd++)
    }// end of for (i = 0; ntg_modules[i]; i++)

    if (found) {//表示有指令,但不能处理
        ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                           "\"%s\" directive is not allowed here", name->data);

        return NTG_ERROR;
    }

    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                       "unknown directive \"%s\"", name->data);

    return NTG_ERROR;

invalid:

    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                       "invalid number of arguments in \"%s\" directive",
                       name->data);

    return NTG_ERROR;
}

/**
 * 读取一个标签
 * @param[in] cf 配置对象
 * @return
 *    NTG_ERROR             there is error
 *    NTG_OK                the token terminated by ";" was found
 *    NTG_CONF_BLOCK_START  the token terminated by "{" was found
 *    NTG_CONF_BLOCK_DONE   the "}" was found
 *    NTG_CONF_FILE_DONE    the configuration file is done
 * @note 每个标签里面可以包含多个元素,共同构成一个解释的整体.
 */
static ntg_int_t
ntg_conf_read_token(ntg_conf_t *cf)
{
    u_char      *start, ch, *src, *dst;
    off_t        file_size;
    size_t       len;
    ssize_t      n, size;
    ntg_uint_t   found, need_space, last_space, sharp_comment, variable;
    ntg_uint_t   quoted, s_quoted, d_quoted, start_line;
    ntg_str_t   *word;
    ntg_buf_t   *b;

    found = 0;//发现一个标签
    need_space = 0;//需要空格
    last_space = 1;//寻找最后一个空白符
    sharp_comment = 0;//表示注释行
    variable = 0;//表示变量标志
    quoted = 0;//表示有'\'
    s_quoted = 0;//单引号
    d_quoted = 0;//双引号

    cf->args->nelts = 0;
    b = cf->conf_file->buffer;
    start = b->pos;
    start_line = cf->conf_file->line;

    file_size = ntg_file_size(&cf->conf_file->file.info);

    for ( ;; ) {

        if (b->pos >= b->last) {//到达buffer中的数据结尾

            if (cf->conf_file->file.offset >= file_size) {//偏移到溢出处理

                if (cf->args->nelts > 0 || !last_space) {

                    if (cf->conf_file->file.fd == NTG_INVALID_FILE) {
                        ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                           "unexpected end of parameter, "
                                           "expecting \";\"");
                        return NTG_ERROR;
                    }

                    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                  "unexpected end of file, "
                                  "expecting \";\" or \"}\"");
                    return NTG_ERROR;
                }

                return NTG_CONF_FILE_DONE;
            }

            len = b->pos - start;

            if (len == NTG_CONF_BUFFER) {//配置参数太长
                cf->conf_file->line = start_line;

                if (d_quoted) {//双引号
                    ch = '"';

                } else if (s_quoted) {//单引号
                    ch = '\'';

                } else {
                    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                       "too long parameter \"%*s...\" started",
                                       10, start);
                    return NTG_ERROR;
                }

                ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                   "too long parameter, probably "
                                   "missing terminating \"%c\" character", ch);
                return NTG_ERROR;
            }

            if (len) {
                ntg_memmove(b->start, start, len);
            }
            //计算可容纳的数据大小
            size = (ssize_t) (file_size - cf->conf_file->file.offset);

            if (size > b->end - (b->start + len)) {
                size = b->end - (b->start + len);
            }
            //读取数据到buffer中
            n = ntg_read_file(&cf->conf_file->file, b->start + len, size,
                              cf->conf_file->file.offset);

            if (n == NTG_ERROR) {
                return NTG_ERROR;
            }

            if (n != size) {
                ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                   ntg_read_file_n " returned "
                                   "only %z bytes instead of %z",
                                   n, size);
                return NTG_ERROR;
            }

            b->pos = b->start + len;
            b->last = b->pos + n;
            start = b->start;
        }

        ch = *b->pos++;
        //对不同的符号进行处理
        if (ch == LF) {
            cf->conf_file->line++;

            if (sharp_comment) {
                sharp_comment = 0;
            }
        }

        if (sharp_comment) {//表示为注释行
            continue;
        }

        if (quoted) {//之前发现'\'
            quoted = 0;
            continue;
        }

        if (need_space) {//需要空白符
            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                last_space = 1;//寻找最后一个空白符
                need_space = 0;
                continue;
            }


            if (ch == ';') {
                return NTG_OK;
            }

            if (ch == '{') {
                return NTG_CONF_BLOCK_START;
            }

            if (ch == ')') {
                last_space = 1;
                need_space = 0;

            } else {
                 ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                    "unexpected \"%c\"", ch);
                 return NTG_ERROR;
            }
        }

        if (last_space) {//查找最后一个空白符
            if (ch == ' ' || ch == '\t' || ch == CR || ch == LF) {
                continue;
            }
            //更新开始处状态
            start = b->pos - 1;
            start_line = cf->conf_file->line;

            switch (ch) {

            case ';':
            case '{'://; 和{ 前一定有一个标签,如果没有表示出错
                if (cf->args->nelts == 0) {
                    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                       "unexpected \"%c\"", ch);
                    return NTG_ERROR;
                }

                if (ch == '{') {
                    return NTG_CONF_BLOCK_START;
                }

                return NTG_OK;

            case '}':// } 前必定有; 或} 表示标签的结束,所以当到达}时必定没有标签
                if (cf->args->nelts != 0) {
                    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                                       "unexpected \"}\"");
                    return NTG_ERROR;
                }

                return NTG_CONF_BLOCK_DONE;

            case '#'://注释开始处
                sharp_comment = 1;
                continue;

            case '\\':
                quoted = 1;
                last_space = 0;
                continue;

            case '"'://
                start++;
                d_quoted = 1;
                last_space = 0;
                continue;

            case '\'':
                start++;
                s_quoted = 1;
                last_space = 0;
                continue;

            default:
                last_space = 0;
            }

        } else {
            if (ch == '{' && variable) {
                continue;
            }

            variable = 0;

            if (ch == '\\') {
                quoted = 1;
                continue;
            }

            if (ch == '$') {//表示变量
                variable = 1;//发现变量
                continue;
            }

            if (d_quoted) {
                if (ch == '"') {
                    d_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (s_quoted) {
                if (ch == '\'') {
                    s_quoted = 0;
                    need_space = 1;
                    found = 1;
                }

            } else if (ch == ' ' || ch == '\t' || ch == CR || ch == LF
                       || ch == ';' || ch == '{')
            {
                last_space = 1;
                found = 1;
            }

            if (found) {
            	//分配一个ntg_str_t
                word = ntg_array_push(cf->args);
                if (word == NULL) {
                    return NTG_ERROR;
                }
                //分配内存
                word->data = ntg_pnalloc(cf->pool, b->pos - start + 1);
                if (word->data == NULL) {
                    return NTG_ERROR;
                }
                //拷贝内容
                for (dst = word->data, src = start, len = 0;
                     src < b->pos - 1;
                     len++)
                {
                    if (*src == '\\') {
                        switch (src[1]) {
                        case '"':
                        case '\'':
                        case '\\':
                            src++;
                            break;

                        case 't':
                            *dst++ = '\t';
                            src += 2;
                            continue;

                        case 'r':
                            *dst++ = '\r';
                            src += 2;
                            continue;

                        case 'n':
                            *dst++ = '\n';
                            src += 2;
                            continue;
                        }

                    }
                    *dst++ = *src++;
                }
                *dst = '\0';
                word->len = len;

                if (ch == ';') {
                    return NTG_OK;
                }

                if (ch == '{') {
                    return NTG_CONF_BLOCK_START;
                }

                found = 0;
            }
        }
    }
}

/**
 * 配置模块的include命令处理函数
 * @param[in] cf 配置对象
 * @param[in] cmd 带处理的命令
 * @param[in] conf 命令将以存放的位置
 * @return 成功返回NTG_CONF_OK, 否则返回NTG_CONF_ERROR
 * @note TODO 暂时没有实现
 */
char *
ntg_conf_include(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char        *rv;
    ntg_int_t    n;
    ntg_str_t   *value, file, name;
    ntg_glob_t   gl;

    value = cf->args->elts;
    file = value[1];

    ntg_log_debug1(NTG_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

    if (ntg_conf_full_name(cf->cycle, &file, 1) != NTG_OK) {
        return NTG_CONF_ERROR;
    }

    if (strpbrk((char *) file.data, "*?[") == NULL) {

        ntg_log_debug1(NTG_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

        return ntg_conf_parse(cf, &file);
    }

    ntg_memzero(&gl, sizeof(ntg_glob_t));

    gl.pattern = file.data;
    gl.log = cf->log;
    gl.test = 1;

    if (ntg_open_glob(&gl) != NTG_OK) {
        ntg_conf_log_error(NTG_LOG_EMERG, cf, ntg_errno,
                           ntg_open_glob_n " \"%s\" failed", file.data);
        return NTG_CONF_ERROR;
    }

    rv = NTG_CONF_OK;

    for ( ;; ) {
        n = ntg_read_glob(&gl, &name);

        if (n != NTG_OK) {
            break;
        }

        file.len = name.len++;
        file.data = ntg_pstrdup(cf->pool, &name);
        if (file.data == NULL) {
            return NTG_CONF_ERROR;
        }

        ntg_log_debug1(NTG_LOG_DEBUG_CORE, cf->log, 0, "include %s", file.data);

        rv = ntg_conf_parse(cf, &file);

        if (rv != NTG_CONF_OK) {
            break;
        }
    }

    ntg_close_glob(&gl);

    return rv;
}

/**
 * 获取文件完整路经
 * @param[in] cycle 循环对象指针
 * @param[in][out] name 文件名
 * @param[in] conf_prefix 配置前缀
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
ntg_int_t
ntg_conf_full_name(ntg_cycle_t *cycle, ntg_str_t *name, ntg_uint_t conf_prefix)
{
    ntg_str_t  *prefix;
    //设置前缀
    prefix = conf_prefix ? &cycle->conf_prefix : &cycle->prefix;

    return ntg_get_full_name(cycle->pool, prefix, name);
}

/**
 * 获取一个打开文件对象
 * @param[in] cycle 全局循环对象
 * @param[in] name 名称
 * @return 成功返回一个打开文件对象,否则返回NULL
 * @note TODO
 */
ntg_open_file_t *
ntg_conf_open_file(ntg_cycle_t *cycle, ntg_str_t *name)
{
    ntg_str_t         full;
    ntg_uint_t        i;
    ntg_list_part_t  *part;
    ntg_open_file_t  *file;

#if (NTG_SUPPRESS_WARN)
    ntg_str_null(&full);
#endif
    //1) 检查是否文件已打开
    if (name->len) {
        full = *name;

        if (ntg_conf_full_name(cycle, &full, 0) != NTG_OK) {
            return NULL;
        }
        //遍历链表
        part = &cycle->open_files.part;
        file = part->elts;

        for (i = 0; /* void */ ; i++) {

            if (i >= part->nelts) {
                if (part->next == NULL) {
                    break;
                }
                part = part->next;
                file = part->elts;
                i = 0;
            }

            if (full.len != file[i].name.len) {
                continue;
            }

            if (ntg_strcmp(full.data, file[i].name.data) == 0) {
                return &file[i];
            }
        }
    }
    //2) 添加一个open_file对象
    file = ntg_list_push(&cycle->open_files);
    if (file == NULL) {
        return NULL;
    }

    if (name->len) {//如果文件存在
        file->fd = NTG_INVALID_FILE;
        file->name = full;

    } else {
        file->fd = ntg_stderr;
        file->name = *name;
    }

    file->flush = NULL;
    file->data = NULL;

    return file;
}

/**
 * 进程退出时,配置模块的处理函数
 * @param[in] cycle 循环体
 */
static void
ntg_conf_flush_files(ntg_cycle_t *cycle)
{
    ntg_uint_t        i;
    ntg_list_part_t  *part;
    ntg_open_file_t  *file;

    ntg_log_debug0(NTG_LOG_DEBUG_CORE, cycle->log, 0, "flush files");

    part = &cycle->open_files.part;
    file = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            file = part->elts;
            i = 0;
        }

        if (file[i].flush) {
            file[i].flush(&file[i], cycle->log);
        }
    }
}

/**
 * 带配置的错误日志输出
 * @param[in] level 错误等级
 * @param[in] cf 配置对象
 * @param[in] err 错误码, 不为0,则将错误码进行文本输出
 * @param[in] fmt 格式化字符串
 */
void
ntg_conf_log_error(ntg_uint_t level, ntg_conf_t *cf, ntg_err_t err,const char *fmt, ...)
{
    u_char   errstr[NTG_MAX_CONF_ERRSTR], *p, *last;
    va_list  args;

    last = errstr + NTG_MAX_CONF_ERRSTR;

    va_start(args, fmt);
    p = ntg_vslprintf(errstr, last, fmt, args);
    va_end(args);

    if (err) {
        p = ntg_log_errno(p, last, err);
    }

    if (cf->conf_file == NULL) {
    	//错误级别比日志级别高才有输出
        ntg_log_error(level, cf->log, 0, "%*s", p - errstr, errstr);
        return;
    }

    if (cf->conf_file->file.fd == NTG_INVALID_FILE) {
        ntg_log_error(level, cf->log, 0, "%*s in command line",
                      p - errstr, errstr);
        return;
    }

    ntg_log_error(level, cf->log, 0, "%*s in %s:%ui",
                  p - errstr, errstr,
                  cf->conf_file->file.name.data, cf->conf_file->line);
}


/*
 * 系统提供的几个配置设置函数
 */
/**
 * 设置标志槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_flag_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    ntg_str_t        *value;
    ntg_flag_t       *fp;
    ntg_conf_post_t  *post;

    fp = (ntg_flag_t *) (p + cmd->offset);

    if (*fp != NTG_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    if (ntg_strcasecmp(value[1].data, (u_char *) "on") == 0) {
        *fp = 1;

    } else if (ntg_strcasecmp(value[1].data, (u_char *) "off") == 0) {
        *fp = 0;

    } else {
        ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                     "invalid value \"%s\" in \"%s\" directive, "
                     "it must be \"on\" or \"off\"",
                     value[1].data, cmd->name.data);
        return NTG_CONF_ERROR;
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, fp);
    }

    return NTG_CONF_OK;
}

/**
 * 设置标str槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_str_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    ntg_str_t        *field, *value;
    ntg_conf_post_t  *post;

    field = (ntg_str_t *) (p + cmd->offset);

    if (field->data) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *field = value[1];

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, field);
    }

    return NTG_CONF_OK;
}

/**
 * 设置数组槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_str_array_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    ntg_str_t         *value, *s;
    ntg_array_t      **a;
    ntg_conf_post_t   *post;

    a = (ntg_array_t **) (p + cmd->offset);

    if (*a == NTG_CONF_UNSET_PTR) {
        *a = ntg_array_create(cf->pool, 4, sizeof(ntg_str_t));
        if (*a == NULL) {
            return NTG_CONF_ERROR;
        }
    }

    s = ntg_array_push(*a);
    if (s == NULL) {
        return NTG_CONF_ERROR;
    }

    value = cf->args->elts;

    *s = value[1];

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, s);
    }

    return NTG_CONF_OK;
}

/**
 * 设置keyal槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_keyval_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    ntg_str_t         *value;
    ntg_array_t      **a;
    ntg_keyval_t      *kv;
    ntg_conf_post_t   *post;

    a = (ntg_array_t **) (p + cmd->offset);

    if (*a == NULL) {
        *a = ntg_array_create(cf->pool, 4, sizeof(ntg_keyval_t));
        if (*a == NULL) {
            return NTG_CONF_ERROR;
        }
    }

    kv = ntg_array_push(*a);
    if (kv == NULL) {
        return NTG_CONF_ERROR;
    }

    value = cf->args->elts;

    kv->key = value[1];
    kv->value = value[2];

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, kv);
    }

    return NTG_CONF_OK;
}

/**
 * 设置数字槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_num_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    ntg_int_t        *np;
    ntg_str_t        *value;
    ntg_conf_post_t  *post;


    np = (ntg_int_t *) (p + cmd->offset);

    if (*np != NTG_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;
    *np = ntg_atoi(value[1].data, value[1].len);
    if (*np == NTG_ERROR) {
        return "invalid number";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, np);
    }

    return NTG_CONF_OK;
}

/**
 * 设置大小槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_size_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    size_t           *sp;
    ntg_str_t        *value;
    ntg_conf_post_t  *post;


    sp = (size_t *) (p + cmd->offset);
    if (*sp != NTG_CONF_UNSET_SIZE) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *sp = ntg_parse_size(&value[1]);
    if (*sp == (size_t) NTG_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, sp);
    }

    return NTG_CONF_OK;
}

/**
 * 设置偏移槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_off_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    off_t            *op;
    ntg_str_t        *value;
    ntg_conf_post_t  *post;


    op = (off_t *) (p + cmd->offset);
    if (*op != NTG_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *op = ntg_parse_offset(&value[1]);
    if (*op == (off_t) NTG_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, op);
    }

    return NTG_CONF_OK;
}

/**
 * 设置毫秒槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_msec_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    ntg_msec_t       *msp;
    ntg_str_t        *value;
    ntg_conf_post_t  *post;


    msp = (ntg_msec_t *) (p + cmd->offset);
    if (*msp != NTG_CONF_UNSET_MSEC) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *msp = ntg_parse_time(&value[1], 0);
    if (*msp == (ntg_msec_t) NTG_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, msp);
    }

    return NTG_CONF_OK;
}

/**
 * 设置秒槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_sec_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    time_t           *sp;
    ntg_str_t        *value;
    ntg_conf_post_t  *post;


    sp = (time_t *) (p + cmd->offset);
    if (*sp != NTG_CONF_UNSET) {
        return "is duplicate";
    }

    value = cf->args->elts;

    *sp = ntg_parse_time(&value[1], 1);
    if (*sp == (time_t) NTG_ERROR) {
        return "invalid value";
    }

    if (cmd->post) {
        post = cmd->post;
        return post->post_handler(cf, post, sp);
    }

    return NTG_CONF_OK;
}

/**
 * 设置bufs槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_bufs_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char *p = conf;

    ntg_str_t   *value;
    ntg_bufs_t  *bufs;


    bufs = (ntg_bufs_t *) (p + cmd->offset);
    if (bufs->num) {
        return "is duplicate";
    }

    value = cf->args->elts;

    bufs->num = ntg_atoi(value[1].data, value[1].len);
    if (bufs->num == NTG_ERROR || bufs->num == 0) {
        return "invalid value";
    }

    bufs->size = ntg_parse_size(&value[2]);
    if (bufs->size == (size_t) NTG_ERROR || bufs->size == 0) {
        return "invalid value";
    }

    return NTG_CONF_OK;
}

/**
 * 设置枚举槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_enum_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    ntg_uint_t       *np, i;
    ntg_str_t        *value;
    ntg_conf_enum_t  *e;

    np = (ntg_uint_t *) (p + cmd->offset);

    if (*np != NTG_CONF_UNSET_UINT) {
        return "is duplicate";
    }

    value = cf->args->elts;
    e = cmd->post;

    for (i = 0; e[i].name.len != 0; i++) {
        if (e[i].name.len != value[1].len
            || ntg_strcasecmp(e[i].name.data, value[1].data) != 0)
        {
            continue;
        }

        *np = e[i].value;

        return NTG_CONF_OK;
    }

    ntg_conf_log_error(NTG_LOG_WARN, cf, 0,
                       "invalid value \"%s\"", value[1].data);

    return NTG_CONF_ERROR;
}

/**
 * 设置bitmask槽
 * @param[in] cf 配置对象
 * @param[in] cmd 命令对象
 * @param[in] conf 存储配置信息的结构体
 * @return	成功返回NTG_CONF_OK,否则返回NTG_CONF_ERROR.
 */
char *
ntg_conf_set_bitmask_slot(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    char  *p = conf;

    ntg_uint_t          *np, i, m;
    ntg_str_t           *value;
    ntg_conf_bitmask_t  *mask;


    np = (ntg_uint_t *) (p + cmd->offset);
    value = cf->args->elts;
    mask = cmd->post;

    for (i = 1; i < cf->args->nelts; i++) {
        for (m = 0; mask[m].name.len != 0; m++) {

            if (mask[m].name.len != value[i].len
                || ntg_strcasecmp(mask[m].name.data, value[i].data) != 0)
            {
                continue;
            }

            if (*np & mask[m].mask) {
                ntg_conf_log_error(NTG_LOG_WARN, cf, 0,
                                   "duplicate value \"%s\"", value[i].data);

            } else {
                *np |= mask[m].mask;
            }

            break;
        }

        if (mask[m].name.len == 0) {
            ntg_conf_log_error(NTG_LOG_WARN, cf, 0,
                               "invalid value \"%s\"", value[i].data);

            return NTG_CONF_ERROR;
        }
    }

    return NTG_CONF_OK;
}


#if 0

char *
ntg_conf_unsupported(ntg_conf_t *cf, ntg_command_t *cmd, void *conf)
{
    return "unsupported on this platform";
}

#endif

/**
 *
 * @param cf
 * @param post
 * @param data
 * @return
 */
char *
ntg_conf_deprecated(ntg_conf_t *cf, void *post, void *data)
{
    ntg_conf_deprecated_t  *d = post;

    ntg_conf_log_error(NTG_LOG_WARN, cf, 0,
                       "the \"%s\" directive is deprecated, "
                       "use the \"%s\" directive instead",
                       d->old_name, d->new_name);

    return NTG_CONF_OK;
}


char *
ntg_conf_check_num_bounds(ntg_conf_t *cf, void *post, void *data)
{
    ntg_conf_num_bounds_t  *bounds = post;
    ntg_int_t  *np = data;

    if (bounds->high == -1) {
        if (*np >= bounds->low) {
            return NTG_CONF_OK;
        }

        ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                           "value must be equal to or greater than %i",
                           bounds->low);

        return NTG_CONF_ERROR;
    }

    if (*np >= bounds->low && *np <= bounds->high) {
        return NTG_CONF_OK;
    }

    ntg_conf_log_error(NTG_LOG_EMERG, cf, 0,
                       "value must be between %i and %i",
                       bounds->low, bounds->high);

    return NTG_CONF_ERROR;
}

