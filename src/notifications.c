/*
 * Copyright (c) 2006-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2); or (b) the Server Side Public License v1 (SSPLv1); or (c) the
 * GNU Affero General Public License v3 (AGPLv3).
*/
#include "config.h"
#include "notifications.h"
#include "spec.h"
#include "doc_types.h"
#include "redismodule.h"
#include "rdb.h"
#include "module.h"
#include "util/workers.h"

#define JSON_LEN 5 // length of string "json."

RedisModuleString *global_RenameFromKey = NULL;
extern RedisModuleCtx *RSDummyContext;
RedisModuleString **hashFields = NULL;

typedef enum {
  _null_cmd,
  hset_cmd,
  hmset_cmd,
  hsetnx_cmd,
  hincrby_cmd,
  hincrbyfloat_cmd,
  hdel_cmd,
  del_cmd,
  set_cmd,
  rename_from_cmd,
  rename_to_cmd,
  trimmed_cmd,
  restore_cmd,
  expire_cmd,
  persist_cmd,
  expired_cmd,
  hexpire_cmd,
  hpersist_cmd,
  hexpired_cmd,
  evicted_cmd,
  change_cmd,
  loaded_cmd,
  copy_to_cmd,
} RedisCmd;

static void freeHashFields() {
  if (hashFields != NULL) {
    for (size_t i = 0; hashFields[i] != NULL; ++i) {
      RedisModule_FreeString(RSDummyContext, hashFields[i]);
    }
    rm_free(hashFields);
    hashFields = NULL;
  }
}

int HashNotificationCallback(RedisModuleCtx *ctx, int type, const char *event,
                             RedisModuleString *key) {

#define CHECK_CACHED_EVENT(E) \
  if (event == E##_event) {   \
    redisCommand = E##_cmd;   \
  }

#define CHECK_AND_CACHE_EVENT(E) \
  if (!strcmp(event, #E)) {      \
    redisCommand = E##_cmd;      \
    E##_event = event;           \
  }

  int redisCommand = 0;
  RedisModuleKey *kp;
  DocumentType kType;

  static const char *hset_event = 0, *hmset_event = 0, *hsetnx_event = 0, *hincrby_event = 0,
                    *hincrbyfloat_event = 0, *hdel_event = 0, *del_event = 0, *set_event = 0,
                    *rename_from_event = 0, *rename_to_event = 0, *trimmed_event = 0,
                    *restore_event = 0, *expired_event = 0, *evicted_event = 0, *change_event = 0,
                    *loaded_event = 0, *copy_to_event = 0, *hexpire_event = 0, *hexpired_event = 0,
                    *expire_event = 0, *hpersist_event = 0, *persist_event = 0;

  // clang-format off

       CHECK_CACHED_EVENT(hset)
  else CHECK_CACHED_EVENT(hmset)
  else CHECK_CACHED_EVENT(hsetnx)
  else CHECK_CACHED_EVENT(hincrby)
  else CHECK_CACHED_EVENT(hincrbyfloat)
  else CHECK_CACHED_EVENT(hdel)
  else CHECK_CACHED_EVENT(del)
  else CHECK_CACHED_EVENT(set)
  else CHECK_CACHED_EVENT(rename_from)
  else CHECK_CACHED_EVENT(rename_to)
  else CHECK_CACHED_EVENT(trimmed)
  else CHECK_CACHED_EVENT(restore)
  else CHECK_CACHED_EVENT(hexpired)
  else CHECK_CACHED_EVENT(expire)
  else CHECK_CACHED_EVENT(expired)
  else CHECK_CACHED_EVENT(persist)
  else CHECK_CACHED_EVENT(hpersist)
  else CHECK_CACHED_EVENT(evicted)
  else CHECK_CACHED_EVENT(change)
  else CHECK_CACHED_EVENT(del)
  else CHECK_CACHED_EVENT(set)
  else CHECK_CACHED_EVENT(rename_from)
  else CHECK_CACHED_EVENT(rename_to)
  else CHECK_CACHED_EVENT(loaded)
  else CHECK_CACHED_EVENT(copy_to)

  else {
         CHECK_AND_CACHE_EVENT(hset)
    else CHECK_AND_CACHE_EVENT(hmset)
    else CHECK_AND_CACHE_EVENT(hsetnx)
    else CHECK_AND_CACHE_EVENT(hincrby)
    else CHECK_AND_CACHE_EVENT(hincrbyfloat)
    else CHECK_AND_CACHE_EVENT(hdel)
    else CHECK_AND_CACHE_EVENT(del)
    else CHECK_AND_CACHE_EVENT(set)
    else CHECK_AND_CACHE_EVENT(rename_from)
    else CHECK_AND_CACHE_EVENT(rename_to)
    else CHECK_AND_CACHE_EVENT(trimmed)
    else CHECK_AND_CACHE_EVENT(restore)
    else CHECK_AND_CACHE_EVENT(hexpire)
    else CHECK_AND_CACHE_EVENT(hexpired)
    else CHECK_AND_CACHE_EVENT(expire)
    else CHECK_AND_CACHE_EVENT(expired)
    else CHECK_AND_CACHE_EVENT(evicted)
    else CHECK_AND_CACHE_EVENT(change)
    else CHECK_AND_CACHE_EVENT(del)
    else CHECK_AND_CACHE_EVENT(set)
    else CHECK_AND_CACHE_EVENT(rename_from)
    else CHECK_AND_CACHE_EVENT(rename_to)
    else CHECK_AND_CACHE_EVENT(loaded)
    else CHECK_AND_CACHE_EVENT(copy_to)
    else redisCommand = _null_cmd;
  }

  switch (redisCommand) {
    case loaded_cmd:
      // on loaded event the key is stack allocated so to use it to load the
      // document we must copy it
      key = RedisModule_CreateStringFromString(ctx, key);
      Indexes_UpdateMatchingWithSchemaRules(ctx, key, getDocTypeFromString(key), hashFields); //TODO: avoid getDocTypeFromString ?
      RedisModule_FreeString(ctx, key);
      break;

    case hset_cmd:
    case hmset_cmd:
    case hsetnx_cmd:
    case hincrby_cmd:
    case hincrbyfloat_cmd:
    case hdel_cmd:
    case hexpired_cmd:
      Indexes_UpdateMatchingWithSchemaRules(ctx, key, DocumentType_Hash, hashFields);
      break;

/********************************************************
 *              Handling Redis commands                 *
 ********************************************************/
    case expire_cmd:
    case persist_cmd:
    case hexpire_cmd:
    case hpersist_cmd:
    case restore_cmd:
    case copy_to_cmd:
      Indexes_UpdateMatchingWithSchemaRules(ctx, key, getDocTypeFromString(key), hashFields);
      break;

    case del_cmd:
    case set_cmd:
    case trimmed_cmd:
    case expired_cmd:
    case evicted_cmd:
      Indexes_DeleteMatchingWithSchemaRules(ctx, key, hashFields);
      break;

    case change_cmd:
    // TODO: hash/json
      kp = RedisModule_OpenKey(ctx, key, REDISMODULE_READ);
      kType = DocumentType_Unsupported;
      if (kp) {
        kType = getDocType(kp);
        RedisModule_CloseKey(kp);
      }
      if (kType == DocumentType_Unsupported) {
        // in crdt empty key means that key was deleted
        // TODO:FIX
        Indexes_DeleteMatchingWithSchemaRules(ctx, key, hashFields);
      } else {
        // todo: here we will open the key again, we can optimize it by
        //       somehow passing the key pointer
        Indexes_UpdateMatchingWithSchemaRules(ctx, key, kType, hashFields);
      }
      break;

    case rename_from_cmd:
      // Notification rename_to is called right after rename_from so this is safe.
      global_RenameFromKey = key;
      break;

    case rename_to_cmd:
      Indexes_ReplaceMatchingWithSchemaRules(ctx, global_RenameFromKey, key);
      break;
  }


/********************************************************
 *              Handling RedisJSON commands             *
 ********************************************************/
  if (!strncmp(event, "json.", JSON_LEN)) {
    if (!strcmp(event + JSON_LEN, "set") ||
        !strcmp(event + JSON_LEN, "merge") ||
        !strcmp(event + JSON_LEN, "mset") ||
        !strcmp(event + JSON_LEN, "del") ||
        !strcmp(event + JSON_LEN, "numincrby") ||
        !strcmp(event + JSON_LEN, "nummultby") ||
        !strcmp(event + JSON_LEN, "strappend") ||
        !strcmp(event + JSON_LEN, "arrappend") ||
        !strcmp(event + JSON_LEN, "arrinsert") ||
        !strcmp(event + JSON_LEN, "arrpop") ||
        !strcmp(event + JSON_LEN, "arrtrim") ||
        !strcmp(event + JSON_LEN, "toggle")) {
      // update index
      Indexes_UpdateMatchingWithSchemaRules(ctx, key, DocumentType_Json, hashFields);
    }
  }

  freeHashFields();

  return REDISMODULE_OK;
}

/*****************************************************************************/

void CommandFilterCallback(RedisModuleCommandFilterCtx *filter) {
  size_t len;
  const RedisModuleString *cmd = RedisModule_CommandFilterArgGet(filter, 0);
  const char *cmdStr = RedisModule_StringPtrLen(cmd, &len);
  if (*cmdStr != 'H' && *cmdStr != 'h') {
    return;
  }

  int numArgs = RedisModule_CommandFilterArgsCount(filter);
  if (numArgs < 3) {
    return;
  }
  int cmdFactor = 1;

  // HSETNX does not fire keyspace event if hash exists. No need to keep fields
  if (!strcasecmp("HSET", cmdStr) || !strcasecmp("HMSET", cmdStr) || !strcasecmp("HSETNX", cmdStr) ||
      !strcasecmp("HINCRBY", cmdStr) || !strcasecmp("HINCRBYFLOAT", cmdStr)) {
    if (numArgs % 2 != 0) return;
    // HSET receives field&value, HDEL receives field
    cmdFactor = 2;
  } else if (!strcasecmp("HDEL", cmdStr)) {
    // Nothing to do
  } else {
    return;
  }

  freeHashFields();

  const RedisModuleString *keyStr = RedisModule_CommandFilterArgGet(filter, 1);
  RedisModuleString *copyKeyStr = RedisModule_CreateStringFromString(RSDummyContext, keyStr);

  RedisModuleKey *k = RedisModule_OpenKey(RSDummyContext, copyKeyStr, REDISMODULE_READ);
  if (!k || RedisModule_KeyType(k) != REDISMODULE_KEYTYPE_HASH) {
    // key does not exist or is not a hash, nothing to do
    goto done;
  }

  int fieldsNum = (numArgs - 2) / cmdFactor;
  hashFields = (RedisModuleString **)rm_calloc(fieldsNum + 1, sizeof(*hashFields));

  for (size_t i = 0; i < fieldsNum; ++i) {
    RedisModuleString *field = (RedisModuleString *)RedisModule_CommandFilterArgGet(filter, 2 + i * cmdFactor);
    RedisModule_RetainString(RSDummyContext, field);
    hashFields[i] = field;
  }

done:
  RedisModule_FreeString(RSDummyContext, copyKeyStr);
  RedisModule_CloseKey(k);
}

void ShardingEvent(RedisModuleCtx *ctx, RedisModuleEvent eid, uint64_t subevent, void *data) {
  /**
   * On sharding event we need to do couple of things depends on the subevent given:
   *
   * 1. REDISMODULE_SUBEVENT_SHARDING_SLOT_RANGE_CHANGED
   *    On this event we know that the slot range changed and we might have data
   *    which no longer belongs to this shard, we must ignore it on searches
   *
   * 2. REDISMODULE_SUBEVENT_SHARDING_TRIMMING_STARTED
   *    This event tells us that the trimming process has started and keys will start to be
   *    deleted, we do not need to do anything on this event
   *
   * 3. REDISMODULE_SUBEVENT_SHARDING_TRIMMING_ENDED
   *    This event tells us that the trimming process has finished, we are not longer
   *    have data that are not belong to us and its safe to stop checking this on searches.
   */
  if (eid.id != REDISMODULE_EVENT_SHARDING) {
    RedisModule_Log(RSDummyContext, "warning", "Bad event given, ignored.");
    return;
  }

  switch (subevent) {
    case REDISMODULE_SUBEVENT_SHARDING_SLOT_RANGE_CHANGED:
      RedisModule_Log(ctx, "notice", "%s", "Got slot range change event, enter trimming phase.");
      isTrimming = true;
      break;
    case REDISMODULE_SUBEVENT_SHARDING_TRIMMING_STARTED:
      RedisModule_Log(ctx, "notice", "%s", "Got trimming started event, enter trimming phase.");
      isTrimming = true;
      workersThreadPool_OnEventStart();
      break;
    case REDISMODULE_SUBEVENT_SHARDING_TRIMMING_ENDED:
      RedisModule_Log(ctx, "notice", "%s", "Got trimming ended event, exit trimming phase.");
      isTrimming = false;
      // Since trimming is done in a part-time job while redis is running other commands, we notify
      // the thread pool to no longer receive new jobs (in RCE mode), and terminate the threads
      // ONCE ALL PENDING JOBS ARE DONE.
      workersThreadPool_OnEventEnd(false);
      break;
    default:
      RedisModule_Log(RSDummyContext, "warning", "Bad subevent given, ignored.");
  }
}

void ShutdownEvent(RedisModuleCtx *ctx, RedisModuleEvent eid, uint64_t subevent, void *data) {
  RedisModule_Log(ctx, "notice", "%s", "Begin releasing RediSearch resources on shutdown");
  RediSearch_CleanupModule();
  RedisModule_Log(ctx, "notice", "%s", "End releasing RediSearch resources");
}

#define HIDE_USER_DATA_FROM_LOGS "hide-user-data-from-log"

bool getHideUserDataFromLogs() {
  char *value = getRedisConfigValue(RSDummyContext, HIDE_USER_DATA_FROM_LOGS);
  RedisModule_Assert(value);
  const bool hideUserData = !strcasecmp(value, "yes");
  rm_free(value);
  return hideUserData;
}

void onUpdatedHideUserDataFromLogs(RedisModuleCtx *ctx) {
  RSGlobalConfig.hideUserDataFromLog = getHideUserDataFromLogs();
  if (RSGlobalConfig.hideUserDataFromLog) {
    RedisModule_Log(ctx, "notice", "Hide user data from search logs is now enabled, "
                   "search entity names (such as indexes and fields) in the logs will now be obfuscated");
  } else {
    RedisModule_Log(ctx, "notice", "Hide user data from search logs is now disabled, "
                   "search entity names (such as indexes and fields) in the logs will now be visible");
  }
}

void ConfigChangedCallback(RedisModuleCtx *ctx, RedisModuleEvent eid, uint64_t event, void *data) {
  if (eid.id != REDISMODULE_EVENT_CONFIG ||
      event != REDISMODULE_SUBEVENT_CONFIG_CHANGE) {
    return;
  }
  RedisModuleConfigChangeV1 *ei = data;
  for (unsigned int i = 0; i < ei->num_changes; i++) {
    const char *conf = ei->config_names[i];
    if (!strcmp(conf, HIDE_USER_DATA_FROM_LOGS)) {
      onUpdatedHideUserDataFromLogs(ctx);
    }
  }
}

void Initialize_KeyspaceNotifications() {
  static bool RS_KeyspaceEvents_Initialized = false;
  if (!RS_KeyspaceEvents_Initialized) {
    RedisModule_SubscribeToKeyspaceEvents(RSDummyContext,
      REDISMODULE_NOTIFY_GENERIC | REDISMODULE_NOTIFY_HASH |
      REDISMODULE_NOTIFY_TRIMMED | REDISMODULE_NOTIFY_STRING |
      REDISMODULE_NOTIFY_EXPIRED | REDISMODULE_NOTIFY_EVICTED |
      REDISMODULE_NOTIFY_LOADED | REDISMODULE_NOTIFY_MODULE,
      HashNotificationCallback);
    RS_KeyspaceEvents_Initialized = true;
  }
}

void Initialize_ServerEventNotifications(RedisModuleCtx *ctx) {
  // RedisModule_SubscribeToServerEvent should exist since redis 6.0
  // We can assume it is always present

  // we do not need to scan after rdb load, i.e, there is not danger of losing results
  // after resharding, its safe to filter keys which are not in our slot range.
  if (RedisModule_ShardingGetKeySlot) {
    // we have server events support, lets subscribe to relevant events.
    RedisModule_Log(ctx, "notice", "%s", "Subscribe to sharding events");
    RedisModule_SubscribeToServerEvent(ctx, RedisModuleEvent_Sharding, ShardingEvent);
  }

  if (getenv("RS_GLOBAL_DTORS")) {
    // clear resources when the server exits
    // used only with sanitizer or valgrind
    RedisModule_Log(ctx, "notice", "%s", "Subscribe to clear resources on shutdown");
    RedisModule_SubscribeToServerEvent(ctx, RedisModuleEvent_Shutdown, ShutdownEvent);
  }

  RedisModule_Log(ctx, "notice", "%s", "Subscribe to config changes");
  RedisModule_SubscribeToServerEvent(ctx, RedisModuleEvent_Config, ConfigChangedCallback);
}

void Initialize_CommandFilter(RedisModuleCtx *ctx) {
  if (RSGlobalConfig.filterCommands) {
    RedisModule_RegisterCommandFilter(ctx, CommandFilterCallback, 0);
  }
}


void ReplicaBackupCallback(RedisModuleCtx *ctx, RedisModuleEvent eid, uint64_t subevent, void *data) {

  REDISMODULE_NOT_USED(eid);
  switch(subevent) {
  case REDISMODULE_SUBEVENT_REPL_BACKUP_CREATE:
    Backup_Globals();
    break;
  case REDISMODULE_SUBEVENT_REPL_BACKUP_RESTORE:
    Restore_Globals();
    break;
  case REDISMODULE_SUBEVENT_REPL_BACKUP_DISCARD:
    Discard_Globals_Backup();
    break;
  }
}

void ReplicaAsyncLoad(RedisModuleCtx *ctx, RedisModuleEvent eid, uint64_t subevent, void *data) {
	REDISMODULE_NOT_USED(eid);
	// Todo: implement callbacks to support async read requests during diskless rdb replication
	//  in "swapdb" mode.
}


int CheckVersionForShortRead() {
  // Minimal versions: 6.2.5
  // (6.0.15 is not supporting the required event notification for modules)
  if (redisVersion.majorVersion >= 7 || (redisVersion.majorVersion == 6 && redisVersion.minorVersion >= 2)) {
	  return REDISMODULE_OK;
  }
  if (redisVersion.majorVersion == 6 && redisVersion.minorVersion == 2) {
      return redisVersion.patchVersion >= 5 ? REDISMODULE_OK : REDISMODULE_ERR;
  } else if (redisVersion.majorVersion == 255 &&
           redisVersion.minorVersion == 255 &&
           redisVersion.patchVersion == 255) {
    // Also supported on master (version=255.255.255)
    return REDISMODULE_OK;
  }
  return REDISMODULE_ERR;
}

void Initialize_RdbNotifications(RedisModuleCtx *ctx) {
  if (CheckVersionForShortRead() == REDISMODULE_OK) {
    int success = RedisModule_SubscribeToServerEvent(ctx, RedisModuleEvent_ReplBackup, ReplicaBackupCallback);
    RS_ASSERT_ALWAYS(success != REDISMODULE_ERR); // should be supported in this redis version/release
	RedisModule_SetModuleOptions(ctx, REDISMODULE_OPTIONS_HANDLE_IO_ERRORS);
    if (redisVersion.majorVersion < 7 || IsEnterprise()) {
	    RedisModule_Log(ctx, "notice", "Enabled diskless replication");
	    // TODO: in OSS, in redis >= 7, we must set REDISMODULE_OPTIONS_HANDLE_REPL_ASYNC_LOAD as well to allow
	    //  diskless replication, as diskless replication occurs only in 'swapdb' mode.
    }
  }
}

void RoleChangeCallback(RedisModuleCtx *ctx, RedisModuleEvent eid, uint64_t subevent, void *data) {
  REDISMODULE_NOT_USED(eid);
  switch(subevent) {
  case REDISMODULE_EVENT_REPLROLECHANGED_NOW_MASTER:
    Indexes_SetTempSpecsTimers(TimerOp_Add);
    break;
  case REDISMODULE_EVENT_REPLROLECHANGED_NOW_REPLICA:
    Indexes_SetTempSpecsTimers(TimerOp_Del);
    break;
  }
}

void Initialize_RoleChangeNotifications(RedisModuleCtx *ctx) {
  int success = RedisModule_SubscribeToServerEvent(ctx, RedisModuleEvent_ReplicationRoleChanged, RoleChangeCallback);
  RS_ASSERT(success != REDISMODULE_ERR); // should be supported in this redis version/release
  RedisModule_Log(ctx, "notice", "Enabled role change notification");
}
