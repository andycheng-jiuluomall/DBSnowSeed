#pragma once
#pragma warning(disable: 4996)

#define UDF_DLL __declspec(dllexport) 
#include "mysql.h"  
#include "mysql/udf_registration_types.h"
#include <codecvt>
#include <wchar.h>


extern "C" {
	UDF_DLL bool SnowSeed_init(UDF_INIT* initid, UDF_ARGS* args, char* message);
	UDF_DLL long long SnowSeed(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* error);
	UDF_DLL void SnowSeed_deinit(UDF_INIT* initid);
}