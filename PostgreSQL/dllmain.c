// dllmain.cpp : 定义 DLL 应用程序的入口点。


#include <postgres.h>
#include <fmgr.h>
#include <utils/hsearch.h>
#include <storage/lwlock.h>
#include "ClassFiles.h"
#include <storage/spin.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(SnowSeed);

Datum SnowSeed(PG_FUNCTION_ARGS);

Datum SnowSeed(PG_FUNCTION_ARGS) {
	bool	 found;
	Oid relid;
	int64 time, millisecondOffset;
	struct SeHashKey searchKey;
	struct SeHashEntry* entry = NULL;
	struct timeval tp;
	int32 machineId, timeOffset, machineBinLen, snBinLen, sn;



	if (NULL == pgState || NULL == pgHash)
		ereport(ERROR, (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
			errmsg("pg_kmb must be loaded via shared_preload_libraries")));

	relid = PG_GETARG_OID(0);
	machineId = PG_GETARG_INT32(1);
	machineBinLen = PG_GETARG_INT32(2);
	snBinLen = PG_GETARG_INT32(3);
	millisecondOffset = PG_GETARG_INT64(4);

	if (machineId < 1)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
			errmsg("The parameter iMachineId must be greater than 1.")));
	if (machineBinLen < 1)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
			errmsg("The parameter iMachineBinLen must be greater than 1.")));
	if (snBinLen < 1)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
			errmsg("The parameter iSnBinLen must be greater than 1.")));
	if (millisecondOffset < 0)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
			errmsg("The parameter iMillisecondOffset must be greater than 0.")));

	if (machineId > ((1 << machineBinLen) - 1))
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
			errmsg("The parameter iMachineId out of range.")));
	timeOffset = (machineBinLen + snBinLen);

	if (timeOffset != 22)
		ereport(ERROR, (errcode(ERRCODE_DATA_EXCEPTION),
			errmsg("iMachineBinLen and  iSnBinLen out of range(iMachineBinLen + iSnBinLen=22).")));


	//获取时间
	gettimeofday(&tp, NULL);
	time = (
		(((int64)tp.tv_sec) * INT64CONST(1000))
		+ (((int64)tp.tv_usec) / INT64CONST(1000))
		) - millisecondOffset;

	//hash操作
	searchKey.relid = relid;
	searchKey.machineId = machineId;
	LWLockAcquire(pgState->lock, LW_SHARED);
	entry = (struct SeHashEntry*)hash_search(pgHash, &searchKey, HASH_FIND, NULL);
	if (NULL == entry) {
		LWLockRelease(pgState->lock);
		LWLockAcquire(pgState->lock, LW_EXCLUSIVE);
		entry = (struct SeHashEntry*)hash_search(pgHash, &searchKey, HASH_ENTER_NULL, &found);
		if (!found) {
			SpinLockInit(&entry->mutex);
			SpinLockAcquire(&entry->mutex);
			entry->millisecond = time;
			entry->sn = 1;
			SpinLockRelease(&entry->mutex);
			sn = 1;
		}
		else { /*永远不会执行*/
			SpinLockAcquire(&entry->mutex);
			sn = entry->sn;
			SpinLockRelease(&entry->mutex);
		}
	}
	else {
		SpinLockAcquire(&entry->mutex);
		if (time == entry->millisecond) {
			++entry->sn;
		}
		else {
			entry->millisecond = time;
			entry->sn = 1;
		}
		sn = entry->sn;
		SpinLockRelease(&entry->mutex);
	}
	LWLockRelease(pgState->lock);
	PG_RETURN_INT64(
		(time << timeOffset) |
		(((int64)machineId) << snBinLen) |
		(sn)
	);
}