/**
 * @file 	ntg_hash.c
 * @brief
 * @details
 * @author	tzh
 * @date	Nov 15, 2015	
 * @version		V0.1
 * @copyright	tzh 
 */

#include "../ntg_config.h"
#include "../ntg_core.h"
#include "ntg_hash.h"


void *
ntg_hash_find(ntg_hash_t *hash, ntg_uint_t key, u_char *name, size_t len)
{
    ntg_uint_t       i;
    ntg_hash_elt_t  *elt;

#if 0
    ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0, "hf:\"%*s\"", len, name);
#endif

    elt = hash->buckets[key % hash->size];

    if (elt == NULL) {
        return NULL;
    }

    while (elt->value) {
        if (len != (size_t) elt->len) {
            goto next;
        }

        for (i = 0; i < len; i++) {
            if (name[i] != elt->name[i]) {
                goto next;
            }
        }

        return elt->value;

    next:

        elt = (ntg_hash_elt_t *) ntg_align_ptr(&elt->name[0] + elt->len,
                                               sizeof(void *));
        continue;
    }

    return NULL;
}


void *
ntg_hash_find_wc_head(ntg_hash_wildcard_t *hwc, u_char *name, size_t len)
{
    void        *value;
    ntg_uint_t   i, n, key;

#if 0
    ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0, "wch:\"%*s\"", len, name);
#endif

    n = len;

    while (n) {
        if (name[n - 1] == '.') {
            break;
        }

        n--;
    }

    key = 0;

    for (i = n; i < len; i++) {
        key = ntg_hash(key, name[i]);
    }

#if 0
    ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0, "key:\"%ui\"", key);
#endif

    value = ntg_hash_find(&hwc->hash, key, &name[n], len - n);

#if 0
    ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0, "value:\"%p\"", value);
#endif

    if (value) {

        /*
         * the 2 low bits of value have the special meaning:
         *     00 - value is data pointer for both "example.com"
         *          and "*.example.com";
         *     01 - value is data pointer for "*.example.com" only;
         *     10 - value is pointer to wildcard hash allowing
         *          both "example.com" and "*.example.com";
         *     11 - value is pointer to wildcard hash allowing
         *          "*.example.com" only.
         */

        if ((uintptr_t) value & 2) {

            if (n == 0) {

                /* "example.com" */

                if ((uintptr_t) value & 1) {
                    return NULL;
                }

                hwc = (ntg_hash_wildcard_t *)
                                          ((uintptr_t) value & (uintptr_t) ~3);
                return hwc->value;
            }

            hwc = (ntg_hash_wildcard_t *) ((uintptr_t) value & (uintptr_t) ~3);

            value = ntg_hash_find_wc_head(hwc, name, n - 1);

            if (value) {
                return value;
            }

            return hwc->value;
        }

        if ((uintptr_t) value & 1) {

            if (n == 0) {

                /* "example.com" */

                return NULL;
            }

            return (void *) ((uintptr_t) value & (uintptr_t) ~3);
        }

        return value;
    }

    return hwc->value;
}


void *
ntg_hash_find_wc_tail(ntg_hash_wildcard_t *hwc, u_char *name, size_t len)
{
    void        *value;
    ntg_uint_t   i, key;

#if 0
    ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0, "wct:\"%*s\"", len, name);
#endif

    key = 0;

    for (i = 0; i < len; i++) {
        if (name[i] == '.') {
            break;
        }

        key = ntg_hash(key, name[i]);
    }

    if (i == len) {
        return NULL;
    }

#if 0
    ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0, "key:\"%ui\"", key);
#endif

    value = ntg_hash_find(&hwc->hash, key, name, i);

#if 0
    ntg_log_error(NTG_LOG_ALERT, ntg_cycle->log, 0, "value:\"%p\"", value);
#endif

    if (value) {

        /*
         * the 2 low bits of value have the special meaning:
         *     00 - value is data pointer;
         *     11 - value is pointer to wildcard hash allowing "example.*".
         */

        if ((uintptr_t) value & 2) {

            i++;

            hwc = (ntg_hash_wildcard_t *) ((uintptr_t) value & (uintptr_t) ~3);

            value = ntg_hash_find_wc_tail(hwc, &name[i], len - i);

            if (value) {
                return value;
            }

            return hwc->value;
        }

        return value;
    }

    return hwc->value;
}


void *
ntg_hash_find_combined(ntg_hash_combined_t *hash, ntg_uint_t key, u_char *name,
    size_t len)
{
    void  *value;

    if (hash->hash.buckets) {
        value = ntg_hash_find(&hash->hash, key, name, len);

        if (value) {
            return value;
        }
    }

    if (len == 0) {
        return NULL;
    }

    if (hash->wc_head && hash->wc_head->hash.buckets) {
        value = ntg_hash_find_wc_head(hash->wc_head, name, len);

        if (value) {
            return value;
        }
    }

    if (hash->wc_tail && hash->wc_tail->hash.buckets) {
        value = ntg_hash_find_wc_tail(hash->wc_tail, name, len);

        if (value) {
            return value;
        }
    }

    return NULL;
}


#define NTG_HASH_ELT_SIZE(name)                                               \
    (sizeof(void *) + ntg_align((name)->key.len + 2, sizeof(void *)))
/**
 * 初始化hash对象
 * @param[in] hinit hash初始化对象
 * @param[in] names hash关键字对象
 * @param[in] nelts 元素的个数
 * @return 成功返回NTG_OK, 否则返回NTG_ERROR
 */
ntg_int_t
ntg_hash_init(ntg_hash_init_t *hinit, ntg_hash_key_t *names, ntg_uint_t nelts)
{
    u_char          *elts;
    size_t           len;
    u_short         *test;
    ntg_uint_t       i, n, key, size, start, bucket_size;
    ntg_hash_elt_t  *elt, **buckets;

    for (n = 0; n < nelts; n++) {//检查buchet_size
        if (hinit->bucket_size < NTG_HASH_ELT_SIZE(&names[n]) + sizeof(void *))
        {
            ntg_log_error(NTG_LOG_EMERG, hinit->pool->log, 0,
                          "could not build the %s, you should "
                          "increase %s_bucket_size: %i",
                          hinit->name, hinit->name, hinit->bucket_size);
            return NTG_ERROR;
        }
    }

    test = ntg_alloc(hinit->max_size * sizeof(u_short), hinit->pool->log);
    if (test == NULL) {
        return NTG_ERROR;
    }

    bucket_size = hinit->bucket_size - sizeof(void *);

    start = nelts / (bucket_size / (2 * sizeof(void *)));
    start = start ? start : 1;

    if (hinit->max_size > 10000 && nelts && hinit->max_size / nelts < 100) {
        start = hinit->max_size - 1000;
    }

    for (size = start; size <= hinit->max_size; size++) {

        ntg_memzero(test, size * sizeof(u_short));

        for (n = 0; n < nelts; n++) {
            if (names[n].key.data == NULL) {
                continue;
            }

            key = names[n].key_hash % size;
            test[key] = (u_short) (test[key] + NTG_HASH_ELT_SIZE(&names[n]));

#if 0
            ntg_log_error(NTG_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: %ui %ui \"%V\"",
                          size, key, test[key], &names[n].key);
#endif

            if (test[key] > (u_short) bucket_size) {
                goto next;
            }
        }

        goto found;

    next:

        continue;
    }

    size = hinit->max_size;

    ntg_log_error(NTG_LOG_WARN, hinit->pool->log, 0,
                  "could not build optimal %s, you should increase "
                  "either %s_max_size: %i or %s_bucket_size: %i; "
                  "ignoring %s_bucket_size",
                  hinit->name, hinit->name, hinit->max_size,
                  hinit->name, hinit->bucket_size, hinit->name);

found:

    for (i = 0; i < size; i++) {
        test[i] = sizeof(void *);
    }

    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }

        key = names[n].key_hash % size;
        test[key] = (u_short) (test[key] + NTG_HASH_ELT_SIZE(&names[n]));
    }

    len = 0;

    for (i = 0; i < size; i++) {
        if (test[i] == sizeof(void *)) {
            continue;
        }

        test[i] = (u_short) (ntg_align(test[i], ntg_cacheline_size));

        len += test[i];
    }

    if (hinit->hash == NULL) {
        hinit->hash = ntg_pcalloc(hinit->pool, sizeof(ntg_hash_wildcard_t)
                                             + size * sizeof(ntg_hash_elt_t *));
        if (hinit->hash == NULL) {
            ntg_free(test);
            return NTG_ERROR;
        }

        buckets = (ntg_hash_elt_t **)
                      ((u_char *) hinit->hash + sizeof(ntg_hash_wildcard_t));

    } else {
        buckets = ntg_pcalloc(hinit->pool, size * sizeof(ntg_hash_elt_t *));
        if (buckets == NULL) {
            ntg_free(test);
            return NTG_ERROR;
        }
    }

    elts = ntg_palloc(hinit->pool, len + ntg_cacheline_size);
    if (elts == NULL) {
        ntg_free(test);
        return NTG_ERROR;
    }

    elts = ntg_align_ptr(elts, ntg_cacheline_size);

    for (i = 0; i < size; i++) {
        if (test[i] == sizeof(void *)) {
            continue;
        }

        buckets[i] = (ntg_hash_elt_t *) elts;
        elts += test[i];

    }

    for (i = 0; i < size; i++) {
        test[i] = 0;
    }

    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }

        key = names[n].key_hash % size;
        elt = (ntg_hash_elt_t *) ((u_char *) buckets[key] + test[key]);

        elt->value = names[n].value;
        elt->len = (u_short) names[n].key.len;

        ntg_strlow(elt->name, names[n].key.data, names[n].key.len);

        test[key] = (u_short) (test[key] + NTG_HASH_ELT_SIZE(&names[n]));
    }

    for (i = 0; i < size; i++) {
        if (buckets[i] == NULL) {
            continue;
        }

        elt = (ntg_hash_elt_t *) ((u_char *) buckets[i] + test[i]);

        elt->value = NULL;
    }

    ntg_free(test);

    hinit->hash->buckets = buckets;
    hinit->hash->size = size;

#if 0

    for (i = 0; i < size; i++) {
        ntg_str_t   val;
        ntg_uint_t  key;

        elt = buckets[i];

        if (elt == NULL) {
            ntg_log_error(NTG_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: NULL", i);
            continue;
        }

        while (elt->value) {
            val.len = elt->len;
            val.data = &elt->name[0];

            key = hinit->key(val.data, val.len);

            ntg_log_error(NTG_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: %p \"%V\" %ui", i, elt, &val, key);

            elt = (ntg_hash_elt_t *) ntg_align_ptr(&elt->name[0] + elt->len,
                                                   sizeof(void *));
        }
    }

#endif

    return NTG_OK;
}


ntg_int_t
ntg_hash_wildcard_init(ntg_hash_init_t *hinit, ntg_hash_key_t *names,
    ntg_uint_t nelts)
{
    size_t                len, dot_len;
    ntg_uint_t            i, n, dot;
    ntg_array_t           curr_names, next_names;
    ntg_hash_key_t       *name, *next_name;
    ntg_hash_init_t       h;
    ntg_hash_wildcard_t  *wdc;

    if (ntg_array_init(&curr_names, hinit->temp_pool, nelts,
                       sizeof(ntg_hash_key_t))
        != NTG_OK)
    {
        return NTG_ERROR;
    }

    if (ntg_array_init(&next_names, hinit->temp_pool, nelts,
                       sizeof(ntg_hash_key_t))
        != NTG_OK)
    {
        return NTG_ERROR;
    }

    for (n = 0; n < nelts; n = i) {

#if 0
        ntg_log_error(NTG_LOG_ALERT, hinit->pool->log, 0,
                      "wc0: \"%V\"", &names[n].key);
#endif

        dot = 0;

        for (len = 0; len < names[n].key.len; len++) {
            if (names[n].key.data[len] == '.') {
                dot = 1;
                break;
            }
        }

        name = ntg_array_push(&curr_names);
        if (name == NULL) {
            return NTG_ERROR;
        }

        name->key.len = len;
        name->key.data = names[n].key.data;
        name->key_hash = hinit->key(name->key.data, name->key.len);
        name->value = names[n].value;

#if 0
        ntg_log_error(NTG_LOG_ALERT, hinit->pool->log, 0,
                      "wc1: \"%V\" %ui", &name->key, dot);
#endif

        dot_len = len + 1;

        if (dot) {
            len++;
        }

        next_names.nelts = 0;

        if (names[n].key.len != len) {
            next_name = ntg_array_push(&next_names);
            if (next_name == NULL) {
                return NTG_ERROR;
            }

            next_name->key.len = names[n].key.len - len;
            next_name->key.data = names[n].key.data + len;
            next_name->key_hash = 0;
            next_name->value = names[n].value;

#if 0
            ntg_log_error(NTG_LOG_ALERT, hinit->pool->log, 0,
                          "wc2: \"%V\"", &next_name->key);
#endif
        }

        for (i = n + 1; i < nelts; i++) {
            if (ntg_strncmp(names[n].key.data, names[i].key.data, len) != 0) {
                break;
            }

            if (!dot
                && names[i].key.len > len
                && names[i].key.data[len] != '.')
            {
                break;
            }

            next_name = ntg_array_push(&next_names);
            if (next_name == NULL) {
                return NTG_ERROR;
            }

            next_name->key.len = names[i].key.len - dot_len;
            next_name->key.data = names[i].key.data + dot_len;
            next_name->key_hash = 0;
            next_name->value = names[i].value;

#if 0
            ntg_log_error(NTG_LOG_ALERT, hinit->pool->log, 0,
                          "wc3: \"%V\"", &next_name->key);
#endif
        }

        if (next_names.nelts) {

            h = *hinit;
            h.hash = NULL;

            if (ntg_hash_wildcard_init(&h, (ntg_hash_key_t *) next_names.elts,
                                       next_names.nelts)
                != NTG_OK)
            {
                return NTG_ERROR;
            }

            wdc = (ntg_hash_wildcard_t *) h.hash;

            if (names[n].key.len == len) {
                wdc->value = names[n].value;
            }

            name->value = (void *) ((uintptr_t) wdc | (dot ? 3 : 2));

        } else if (dot) {
            name->value = (void *) ((uintptr_t) name->value | 1);
        }
    }

    if (ntg_hash_init(hinit, (ntg_hash_key_t *) curr_names.elts,
                      curr_names.nelts)
        != NTG_OK)
    {
        return NTG_ERROR;
    }

    return NTG_OK;
}

/**
 * 关键字hash函数
 * @param[in] data 关键字
 * @param[in] len 长度
 * @return 返回hash值
 */
ntg_uint_t
ntg_hash_key(u_char *data, size_t len)
{
    ntg_uint_t  i, key;

    key = 0;

    for (i = 0; i < len; i++) {
        key = ntg_hash(key, data[i]);
    }

    return key;
}

/**
 * 将关键字转为小写在计算hash值
 * @param[in] data 关键字
 * @param[in] len 长度
 * @return 返回hash值
 * @note lc表示转为小写字符在进行hash计算
 */
ntg_uint_t
ntg_hash_key_lc(u_char *data, size_t len)
{
    ntg_uint_t  i, key;

    key = 0;

    for (i = 0; i < len; i++) {
        key = ntg_hash(key, ntg_tolower(data[i]));//采用time31算法
    }

    return key;
}

/**
 * 将src字符串转为小写并存在dst,同时返回dst的hash值
 * @param[in] dst 目标字符串存储位置
 * @param[in] src 源字符串
 * @param[in] n src长度
 * @return dst的hash值
 * @note dst必须至少为n存储空间
 */
ntg_uint_t
ntg_hash_strlow(u_char *dst, u_char *src, size_t n)
{
    ntg_uint_t  key;

    key = 0;

    while (n--) {
        *dst = ntg_tolower(*src);
        key = ntg_hash(key, *dst);
        dst++;
        src++;
    }

    return key;
}


ntg_int_t
ntg_hash_keys_array_init(ntg_hash_keys_arrays_t *ha, ntg_uint_t type)
{
    ntg_uint_t  asize;

    if (type == NTG_HASH_SMALL) {
        asize = 4;
        ha->hsize = 107;

    } else {
        asize = NTG_HASH_LARGE_ASIZE;
        ha->hsize = NTG_HASH_LARGE_HSIZE;
    }

    if (ntg_array_init(&ha->keys, ha->temp_pool, asize, sizeof(ntg_hash_key_t))
        != NTG_OK)
    {
        return NTG_ERROR;
    }

    if (ntg_array_init(&ha->dns_wc_head, ha->temp_pool, asize,
                       sizeof(ntg_hash_key_t))
        != NTG_OK)
    {
        return NTG_ERROR;
    }

    if (ntg_array_init(&ha->dns_wc_tail, ha->temp_pool, asize,
                       sizeof(ntg_hash_key_t))
        != NTG_OK)
    {
        return NTG_ERROR;
    }

    ha->keys_hash = ntg_pcalloc(ha->temp_pool, sizeof(ntg_array_t) * ha->hsize);
    if (ha->keys_hash == NULL) {
        return NTG_ERROR;
    }

    ha->dns_wc_head_hash = ntg_pcalloc(ha->temp_pool,
                                       sizeof(ntg_array_t) * ha->hsize);
    if (ha->dns_wc_head_hash == NULL) {
        return NTG_ERROR;
    }

    ha->dns_wc_tail_hash = ntg_pcalloc(ha->temp_pool,
                                       sizeof(ntg_array_t) * ha->hsize);
    if (ha->dns_wc_tail_hash == NULL) {
        return NTG_ERROR;
    }

    return NTG_OK;
}


ntg_int_t
ntg_hash_add_key(ntg_hash_keys_arrays_t *ha, ntg_str_t *key, void *value,
    ntg_uint_t flags)
{
    size_t           len;
    u_char          *p;
    ntg_str_t       *name;
    ntg_uint_t       i, k, n, skip, last;
    ntg_array_t     *keys, *hwc;
    ntg_hash_key_t  *hk;

    last = key->len;

    if (flags & NTG_HASH_WILDCARD_KEY) {

        /*
         * supported wildcards:
         *     "*.example.com", ".example.com", and "www.example.*"
         */

        n = 0;

        for (i = 0; i < key->len; i++) {

            if (key->data[i] == '*') {
                if (++n > 1) {
                    return NTG_DECLINED;
                }
            }

            if (key->data[i] == '.' && key->data[i + 1] == '.') {
                return NTG_DECLINED;
            }
        }

        if (key->len > 1 && key->data[0] == '.') {
            skip = 1;
            goto wildcard;
        }

        if (key->len > 2) {

            if (key->data[0] == '*' && key->data[1] == '.') {
                skip = 2;
                goto wildcard;
            }

            if (key->data[i - 2] == '.' && key->data[i - 1] == '*') {
                skip = 0;
                last -= 2;
                goto wildcard;
            }
        }

        if (n) {
            return NTG_DECLINED;
        }
    }

    /* exact hash */

    k = 0;

    for (i = 0; i < last; i++) {
        if (!(flags & NTG_HASH_READONLY_KEY)) {
            key->data[i] = ntg_tolower(key->data[i]);
        }
        k = ntg_hash(k, key->data[i]);
    }

    k %= ha->hsize;

    /* check conflicts in exact hash */

    name = ha->keys_hash[k].elts;

    if (name) {
        for (i = 0; i < ha->keys_hash[k].nelts; i++) {
            if (last != name[i].len) {
                continue;
            }

            if (ntg_strncmp(key->data, name[i].data, last) == 0) {
                return NTG_BUSY;
            }
        }

    } else {
        if (ntg_array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                           sizeof(ntg_str_t))
            != NTG_OK)
        {
            return NTG_ERROR;
        }
    }

    name = ntg_array_push(&ha->keys_hash[k]);
    if (name == NULL) {
        return NTG_ERROR;
    }

    *name = *key;

    hk = ntg_array_push(&ha->keys);
    if (hk == NULL) {
        return NTG_ERROR;
    }

    hk->key = *key;
    hk->key_hash = ntg_hash_key(key->data, last);
    hk->value = value;

    return NTG_OK;


wildcard:

    /* wildcard hash */

    k = ntg_hash_strlow(&key->data[skip], &key->data[skip], last - skip);

    k %= ha->hsize;

    if (skip == 1) {

        /* check conflicts in exact hash for ".example.com" */

        name = ha->keys_hash[k].elts;

        if (name) {
            len = last - skip;

            for (i = 0; i < ha->keys_hash[k].nelts; i++) {
                if (len != name[i].len) {
                    continue;
                }

                if (ntg_strncmp(&key->data[1], name[i].data, len) == 0) {
                    return NTG_BUSY;
                }
            }

        } else {
            if (ntg_array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                               sizeof(ntg_str_t))
                != NTG_OK)
            {
                return NTG_ERROR;
            }
        }

        name = ntg_array_push(&ha->keys_hash[k]);
        if (name == NULL) {
            return NTG_ERROR;
        }

        name->len = last - 1;
        name->data = ntg_pnalloc(ha->temp_pool, name->len);
        if (name->data == NULL) {
            return NTG_ERROR;
        }

        ntg_memcpy(name->data, &key->data[1], name->len);
    }


    if (skip) {

        /*
         * convert "*.example.com" to "com.example.\0"
         *      and ".example.com" to "com.example\0"
         */

        p = ntg_pnalloc(ha->temp_pool, last);
        if (p == NULL) {
            return NTG_ERROR;
        }

        len = 0;
        n = 0;

        for (i = last - 1; i; i--) {
            if (key->data[i] == '.') {
                ntg_memcpy(&p[n], &key->data[i + 1], len);
                n += len;
                p[n++] = '.';
                len = 0;
                continue;
            }

            len++;
        }

        if (len) {
            ntg_memcpy(&p[n], &key->data[1], len);
            n += len;
        }

        p[n] = '\0';

        hwc = &ha->dns_wc_head;
        keys = &ha->dns_wc_head_hash[k];

    } else {

        /* convert "www.example.*" to "www.example\0" */

        last++;

        p = ntg_pnalloc(ha->temp_pool, last);
        if (p == NULL) {
            return NTG_ERROR;
        }

        ntg_cpystrn(p, key->data, last);

        hwc = &ha->dns_wc_tail;
        keys = &ha->dns_wc_tail_hash[k];
    }


    /* check conflicts in wildcard hash */

    name = keys->elts;

    if (name) {
        len = last - skip;

        for (i = 0; i < keys->nelts; i++) {
            if (len != name[i].len) {
                continue;
            }

            if (ntg_strncmp(key->data + skip, name[i].data, len) == 0) {
                return NTG_BUSY;
            }
        }

    } else {
        if (ntg_array_init(keys, ha->temp_pool, 4, sizeof(ntg_str_t)) != NTG_OK)
        {
            return NTG_ERROR;
        }
    }

    name = ntg_array_push(keys);
    if (name == NULL) {
        return NTG_ERROR;
    }

    name->len = last - skip;
    name->data = ntg_pnalloc(ha->temp_pool, name->len);
    if (name->data == NULL) {
        return NTG_ERROR;
    }

    ntg_memcpy(name->data, key->data + skip, name->len);


    /* add to wildcard hash */

    hk = ntg_array_push(hwc);
    if (hk == NULL) {
        return NTG_ERROR;
    }

    hk->key.len = last - 1;
    hk->key.data = p;
    hk->key_hash = 0;
    hk->value = value;

    return NTG_OK;
}

