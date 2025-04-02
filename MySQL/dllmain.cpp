// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "SnowSeed.h"
#include <unordered_map>
#include <string>
#include "ClassFiles.h"
using std::string;
bool SnowSeed_init(UDF_INIT* initid, UDF_ARGS* args, char* message) {
	bool error = false;
	int type = 0;
	if (args->arg_count != 2)  error = true;
	if (args->arg_type[0] != INT_RESULT)  error = true;
	else {
		try {
			int num = atoi(args->args[0]);
			if (num > 31)  error = true; 
		}
		catch (const std::invalid_argument& e) {
			error = true;
		}
		catch (const std::out_of_range& e) {
			error = true;
			
		}
	}
	if (args->arg_type[1] != INT_RESULT)  error = true; 
	else {
		try {
			int num = atoi(args->args[1]);
			if (num > 31)  error = true;
		}
		catch (const std::invalid_argument& e) {
			error = true;
			
		}
		catch (const std::out_of_range& e) {
			error = true;
		}
	}
	if (error) strcpy(message, (u8"参数必须是两位小于32的整数" + std::to_string(type)).c_str());
	return error;
}
static std::unordered_map<std::string, IdWorker*> pool;
long long SnowSeed(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* error) {
	int workerId = atoi(args->args[0]);
	int centerId = atoi(args->args[1]);
std:string key = std::string(args->args[0]) + "-" + std::string(args->args[1]);
	if (pool.count(key) == 0) pool.emplace(std::make_pair(key, new IdWorker(workerId, centerId)));
	return pool[key]->NextId();
}

void SnowSeed_deinit(UDF_INIT* initid) {}



