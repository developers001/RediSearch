/*
 * Copyright (c) 2006-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2); or (b) the Server Side Public License v1 (SSPLv1); or (c) the
 * GNU Affero General Public License v3 (AGPLv3).
*/
#ifndef RS_RESP3_H_
#define RS_RESP3_H_

#include "redismodule.h"
#include "reply.h"


static inline bool is_resp3(RedisModuleCtx *ctx) {
    return RedisModule_GetContextFlags(ctx) & REDISMODULE_CTX_FLAGS_RESP3;
}

#define _ReplyMap(ctx) (RedisModule_ReplyWithMap != NULL && is_resp3(ctx))
#define _ReplySet(ctx) (RedisModule_ReplyWithSet != NULL && is_resp3(ctx))

static inline void RedisModule_ReplySetMapOrArrayLength(RedisModuleCtx *ctx, long len, bool devide_by_two) {
    if (_ReplyMap(ctx)) {
        RedisModule_ReplySetMapLength(ctx, devide_by_two ? len / 2 : len);
    } else {
        RedisModule_ReplySetArrayLength(ctx, len);
    }
}

static inline int RedisModule_ReplyWithMapOrArray(RedisModuleCtx *ctx, long len, bool devide_by_two) {
    if (_ReplyMap(ctx)) {
        return RedisModule_ReplyWithMap(ctx, devide_by_two ? len / 2 : len);
    } else {
        return RedisModule_ReplyWithArray(ctx, len);
    }
}

static inline void RedisModule_ReplySetSetOrArrayLength(RedisModuleCtx *ctx, long len) {
    if (_ReplySet(ctx)) {
        RedisModule_ReplySetSetLength(ctx, len);
    } else {
        RedisModule_ReplySetArrayLength(ctx, len);
    }
}

static inline void RedisModule_ReplyWithSetOrArray(RedisModuleCtx *ctx, long len) {
    if (_ReplySet(ctx)) {
        RedisModule_ReplyWithSet(ctx, len);
    } else {
        RedisModule_ReplyWithArray(ctx, len);
    }
}

#endif
