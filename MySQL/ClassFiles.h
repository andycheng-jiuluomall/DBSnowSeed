#include <chrono>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <string>

class IdWorker {
public:
    static constexpr int64_t Twepoch = 1288834974657LL; // 基准时间 2010-11-04 09:42:54 UTC

    IdWorker(int32_t worker_id, int32_t datacenter_id, int64_t sequence = 0LL)
        : worker_id_(worker_id),
        datacenter_id_(datacenter_id),
        sequence_(sequence) {
        if (worker_id > MaxWorkerId || worker_id < 0) throw std::invalid_argument("workerId 必须大于0，且不能大于 MaxWorkerId： " + std::to_string(MaxWorkerId));
        if (datacenter_id > MaxDatacenterId || datacenter_id < 0) throw std::invalid_argument("datacenterId 必须大于0，且不能大于 MaxDatacenterId： " + std::to_string(MaxDatacenterId));
    }
    int64_t NextId() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto timestamp = TimeGen();
        // 处理时钟回拨
        if (timestamp < last_timestamp_) throw std::runtime_error("时间戳必须大于上一次生成ID的时间戳.  拒绝为" + std::to_string(last_timestamp_ - timestamp) + "毫秒生成id");
        // 同一毫秒内生成
        if (last_timestamp_ == timestamp) {
            sequence_ = (sequence_ + 1) & SequenceMask;
            if (sequence_ == 0) timestamp = TilNextMillis(last_timestamp_);
        }
        // 新时间窗口重置序列号
        else sequence_ = 0;
        last_timestamp_ = timestamp;
        // 组合ID各部分
        return ((timestamp - Twepoch) << TimestampLeftShift)| (datacenter_id_ << DatacenterIdShift) | (worker_id_ << WorkerIdShift)| sequence_;
    }
private:
    // 位移配置
    static constexpr int WorkerIdBits = 5;
    static constexpr int DatacenterIdBits = 5;
    static constexpr int SequenceBits = 12;
    // 最大值计算
    static constexpr int32_t MaxWorkerId = -1LL ^ (-1LL << WorkerIdBits);          // 31
    static constexpr int32_t MaxDatacenterId = -1LL ^ (-1LL << DatacenterIdBits);  // 31
    static constexpr int32_t SequenceMask = -1LL ^ (-1LL << SequenceBits);         // 4095
    // 位移量
    static constexpr int WorkerIdShift = SequenceBits;                            // 12
    static constexpr int DatacenterIdShift = SequenceBits + WorkerIdBits;         // 17
    static constexpr int TimestampLeftShift = SequenceBits + WorkerIdBits + DatacenterIdBits; // 22
    // 成员变量
    int32_t worker_id_;
    int32_t datacenter_id_;
    int32_t sequence_ = 0LL;
    int64_t last_timestamp_ = -1LL;
    std::mutex mutex_;
    // 等待下一毫秒
    int64_t TilNextMillis(int64_t last_timestamp) {
        auto timestamp = TimeGen();
        while (timestamp <= last_timestamp) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            timestamp = TimeGen();
        }
        return timestamp;
    }
    // 获取当前时间戳
    static int64_t TimeGen() {
        using namespace std::chrono;
        auto now = system_clock::now();
        return duration_cast<milliseconds>(now.time_since_epoch()).count();
    }
};