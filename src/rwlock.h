/*
 * Copyright (c) 2006-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2); or (b) the Server Side Public License v1 (SSPLv1); or (c) the
 * GNU Affero General Public License v3 (AGPLv3).
*/
#ifndef SRC_RWLOCK_H_
#define SRC_RWLOCK_H_

#include <pthread.h>
#include "redismodule.h"

extern pthread_rwlock_t RWLock;
extern pthread_key_t _lockKey;

int RediSearch_LockInit(RedisModuleCtx *ctx);

void RediSearch_LockRead();
void RediSearch_LockWrite();
void RediSearch_LockRelease();

void RediSearch_LockDestory();

#define RWLOCK_ACQUIRE_READ() RediSearch_LockRead()
#define RWLOCK_ACQUIRE_WRITE() RediSearch_LockWrite()
#define RWLOCK_RELEASE() RediSearch_LockRelease()

#endif /* SRC_RWLOCK_H_ */
