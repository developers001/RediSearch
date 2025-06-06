/*
 * Copyright (c) 2006-Present, Redis Ltd.
 * All rights reserved.
 *
 * Licensed under your choice of the Redis Source Available License 2.0
 * (RSALv2); or (b) the Server Side Public License v1 (SSPLv1); or (c) the
 * GNU Affero General Public License v3 (AGPLv3).
*/
#include "spec.h"
#include "alias.h"
#include "rdb.h"

dict *specDict_g_bkup;
TrieMap *ScemaPrefixes_g_bkup;
AliasTable *AliasTable_g_bkup;

void Backup_Globals() {
  specDict_g_bkup = specDict_g;
  specDict_g = dictCreate(&dictTypeHeapHiddenStrings, NULL);

  ScemaPrefixes_g_bkup = SchemaPrefixes_g;
  SchemaPrefixes_Create();

  AliasTable_g_bkup = AliasTable_g;
  IndexAlias_InitGlobal();
}

void Restore_Globals() {
  Indexes_Free(specDict_g);
  dictRelease(specDict_g);
  specDict_g = specDict_g_bkup;
  specDict_g_bkup = NULL;

  SchemaPrefixes_Free(SchemaPrefixes_g);
  SchemaPrefixes_g = ScemaPrefixes_g_bkup;
  ScemaPrefixes_g_bkup = NULL;

  IndexAlias_DestroyGlobal(&AliasTable_g);
  AliasTable_g = AliasTable_g_bkup;
  AliasTable_g_bkup = NULL;
}



void Discard_Globals_Backup() {
  // This is a temporary fix until we change functions to get pointer to lists
  // save global to temp
  dict *specDict_g_temp = specDict_g;
  TrieMap *ScemaPrefixes_g_temp = SchemaPrefixes_g;
  AliasTable *AliasTable_g_temp = AliasTable_g;
  // set backup as globals
  specDict_g = specDict_g_bkup;
  SchemaPrefixes_g = ScemaPrefixes_g_bkup;
  AliasTable_g = AliasTable_g_bkup;
  // clear data
  Indexes_Free(specDict_g);
  SchemaPrefixes_Free(SchemaPrefixes_g);
  IndexAlias_DestroyGlobal(&AliasTable_g);
  // restore global from temp
  specDict_g = specDict_g_temp;
  SchemaPrefixes_g = ScemaPrefixes_g_temp;
  AliasTable_g = AliasTable_g_temp;
  // nullify backup
  specDict_g_bkup = NULL;
  ScemaPrefixes_g_bkup = NULL;
  AliasTable_g_bkup = NULL;
}