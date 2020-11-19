/*
 * Copyright (c) 2019 TAOS Data, Inc. <jhtao@taosdata.com>
 *
 * This program is free software: you can use, redistribute, and/or modify
 * it under the terms of the GNU Affero General Public License, version 3
 * or later ("AGPL"), as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TDENGINE_SYNC_H
#define TDENGINE_SYNC_H

#ifdef __cplusplus
extern "C" {
#endif

#define TAOS_SYNC_MAX_REPLICA 5
#define TAOS_SYNC_MAX_INDEX   0x7FFFFFFF

typedef enum _TAOS_SYNC_ROLE {
  TAOS_SYNC_ROLE_OFFLINE,
  TAOS_SYNC_ROLE_UNSYNCED,
  TAOS_SYNC_ROLE_SYNCING,
  TAOS_SYNC_ROLE_SLAVE,
  TAOS_SYNC_ROLE_MASTER,
} ESyncRole;

typedef enum _TAOS_SYNC_STATUS {
  TAOS_SYNC_STATUS_INIT,
  TAOS_SYNC_STATUS_START,
  TAOS_SYNC_STATUS_FILE,
  TAOS_SYNC_STATUS_CACHE,
} ESyncStatus;

typedef struct {
  uint32_t  nodeId;    // node ID assigned by TDengine
  uint16_t  nodePort;  // node sync Port
  char      nodeFqdn[TSDB_FQDN_LEN]; // node FQDN  
} SNodeInfo;

typedef struct {
  int8_t     quorum;    // number of confirms required, >=1 
  int8_t     replica;   // number of replications, >=1
  SNodeInfo  nodeInfo[TAOS_SYNC_MAX_REPLICA];
} SSyncCfg;

typedef struct {
  int32_t  selfIndex;
  uint32_t nodeId[TAOS_SYNC_MAX_REPLICA];
  int32_t  role[TAOS_SYNC_MAX_REPLICA];
} SNodesRole;

/* 
  if name is empty(name[0] is zero), get the file from index or after, but not larger than eindex. If a file
  is found between index and eindex, index shall be updated, name shall be set, size shall be set to 
  file size, and file magic number shall be returned. 

  if name is provided(name[0] is not zero), get the named file at the specified index. If not there, return
  zero. If it is there, set the size to file size, and return file magic number. Index shall not be updated.
*/
typedef uint32_t (*FGetFileInfo)(void *ahandle, char *name, uint32_t *index, uint32_t eindex, int64_t *size, uint64_t *fversion); 

// get the wal file from index or after
// return value, -1: error, 1:more wal files, 0:last WAL. if name[0]==0, no WAL file
typedef int32_t  (*FGetWalInfo)(void *ahandle, char *fileName, int64_t *fileId); 
 
// when a forward pkt is received, call this to handle data
typedef int32_t  (*FWriteToCache)(void *ahandle, void *pHead, int32_t qtype, void *pMsg);

// when forward is confirmed by peer, master call this API to notify app
typedef void     (*FConfirmForward)(void *ahandle, void *mhandle, int32_t code);

// when role is changed, call this to notify app
typedef void     (*FNotifyRole)(void *ahandle, int8_t role);

// if a number of retrieving data failed, call this to start flow control 
typedef void     (*FNotifyFlowCtrl)(void *ahandle, int32_t mseconds);

// when data file is synced successfully, notity app
typedef int32_t  (*FNotifyFileSynced)(void *ahandle, uint64_t fversion);

typedef struct {
  int32_t  vgId;       // vgroup ID
  uint64_t version;    // initial version
  SSyncCfg syncCfg;    // configuration from mgmt
  char     path[128];  // path to the file
  void *   ahandle;    // handle provided by APP
  FGetFileInfo      getFileInfo;
  FGetWalInfo       getWalInfo;
  FWriteToCache     writeToCache;
  FConfirmForward   confirmForward;
  FNotifyRole       notifyRole;
  FNotifyFlowCtrl   notifyFlowCtrl;
  FNotifyFileSynced notifyFileSynced;
} SSyncInfo;

typedef void *tsync_h;

int32_t syncInit();
void    syncCleanUp();

int64_t syncStart(const SSyncInfo *);
void    syncStop(int64_t rid);
int32_t syncReconfig(int64_t rid, const SSyncCfg *);
int32_t syncForwardToPeer(int64_t rid, void *pHead, void *mhandle, int32_t qtype);
void    syncConfirmForward(int64_t rid, uint64_t version, int32_t code);
void    syncRecover(int64_t rid);  // recover from other nodes:
int32_t syncGetNodesRole(int64_t rid, SNodesRole *);

extern char *syncRole[];

//global configurable parameters
extern int32_t  tsMaxSyncNum;
extern int32_t  tsSyncTcpThreads;
extern int32_t  tsMaxWatchFiles;
extern int32_t  tsSyncTimer;
extern int32_t  tsMaxFwdInfo;
extern int32_t  sDebugFlag;
extern char     tsArbitrator[];
extern uint16_t tsSyncPort;

#ifdef __cplusplus
}
#endif

#endif  // TDENGINE_SYNC_H
