#include <stdint.h>
typedef struct {
    CRITICAL_SECTION lock;
    int32_t worker_id;
    int32_t datacenter_id;
    int32_t sequence;
    int64_t last_timestamp;
} IdWorker;

// 配置常量
enum {
    WorkerIdBits = 5,
    DatacenterIdBits = 5,
    SequenceBits = 12,

    MaxWorkerId = -1 ^ (-1 << WorkerIdBits),
    MaxDatacenterId = -1 ^ (-1 << DatacenterIdBits),
    SequenceMask = -1 ^ (-1 << SequenceBits),

    WorkerIdShift = SequenceBits,
    DatacenterIdShift = SequenceBits + WorkerIdBits,
    TimestampLeftShift = SequenceBits + WorkerIdBits + DatacenterIdBits,

    Twepoch = 1288834974657LL,
};

// 私有函数声明
static int64_t time_gen();
static int64_t til_next_millis(int64_t last_timestamp);

// 创建IdWorker实例
IdWorker* IdWorker_create(int32_t worker_id, int32_t datacenter_id, int* error) {
    if (worker_id < 0 || worker_id > MaxWorkerId) {
        *error = 1;
        return NULL;
    }

    if (datacenter_id < 0 || datacenter_id > MaxDatacenterId) {
        *error = 2;
        return NULL;
    }

    IdWorker* worker = (IdWorker*)malloc(sizeof(IdWorker));
    if (!worker) {
        *error = 3;
        return NULL;
    }

    InitializeCriticalSection(&worker->lock);
    worker->worker_id = worker_id;
    worker->datacenter_id = datacenter_id;
    worker->sequence = 0;
    worker->last_timestamp = -1LL;
    *error = 0;
    return worker;
}

// 销毁IdWorker实例
void IdWorker_destroy(IdWorker* worker) {
    if (worker) {
        DeleteCriticalSection(&worker->lock);
        free(worker);
    }
}

// 生成下一个ID
int64_t IdWorker_next_id(IdWorker* worker, int* error) {
    EnterCriticalSection(&worker->lock);

    int64_t timestamp = time_gen();
    *error = 0;

    if (timestamp < worker->last_timestamp) {
        *error = 1;  // 时钟回拨错误
        LeaveCriticalSection(&worker->lock);
        return 0;
    }

    if (worker->last_timestamp == timestamp) {
        worker->sequence = (worker->sequence + 1) & SequenceMask;
        if (worker->sequence == 0) {
            timestamp = til_next_millis(worker->last_timestamp);
        }
    }
    else {
        worker->sequence = 0;
    }

    worker->last_timestamp = timestamp;

    int64_t result =
        ((timestamp - Twepoch) << TimestampLeftShift) |
        (worker->datacenter_id << DatacenterIdShift) |
        (worker->worker_id << WorkerIdShift) |
        worker->sequence;

    LeaveCriticalSection(&worker->lock);
    return result;
}

// 获取当前时间（毫秒）
static int64_t time_gen() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    const int64_t UNIX_EPOCH = 116444736000000000LL;  // 1970-01-01的FILETIME值
    ULARGE_INTEGER uli;
    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    // 转换为Unix时间（毫秒）
    return (uli.QuadPart - UNIX_EPOCH) / 10000;
}

// 等待下一毫秒
static int64_t til_next_millis(int64_t last_timestamp) {
    int64_t timestamp = time_gen();
    while (timestamp <= last_timestamp) {
        Sleep(1);
        timestamp = time_gen();
    }
    return timestamp;
}