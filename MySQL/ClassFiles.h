#include <chrono>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <string>

class IdWorker {
public:
    static constexpr int64_t Twepoch = 1288834974657LL; // ��׼ʱ�� 2010-11-04 09:42:54 UTC

    IdWorker(int32_t worker_id, int32_t datacenter_id, int64_t sequence = 0LL)
        : worker_id_(worker_id),
        datacenter_id_(datacenter_id),
        sequence_(sequence) {
        if (worker_id > MaxWorkerId || worker_id < 0) throw std::invalid_argument("workerId �������0���Ҳ��ܴ��� MaxWorkerId�� " + std::to_string(MaxWorkerId));
        if (datacenter_id > MaxDatacenterId || datacenter_id < 0) throw std::invalid_argument("datacenterId �������0���Ҳ��ܴ��� MaxDatacenterId�� " + std::to_string(MaxDatacenterId));
    }
    int64_t NextId() {
        std::lock_guard<std::mutex> lock(mutex_);
        auto timestamp = TimeGen();
        // ����ʱ�ӻز�
        if (timestamp < last_timestamp_) throw std::runtime_error("ʱ������������һ������ID��ʱ���.  �ܾ�Ϊ" + std::to_string(last_timestamp_ - timestamp) + "��������id");
        // ͬһ����������
        if (last_timestamp_ == timestamp) {
            sequence_ = (sequence_ + 1) & SequenceMask;
            if (sequence_ == 0) timestamp = TilNextMillis(last_timestamp_);
        }
        // ��ʱ�䴰���������к�
        else sequence_ = 0;
        last_timestamp_ = timestamp;
        // ���ID������
        return ((timestamp - Twepoch) << TimestampLeftShift)| (datacenter_id_ << DatacenterIdShift) | (worker_id_ << WorkerIdShift)| sequence_;
    }
private:
    // λ������
    static constexpr int WorkerIdBits = 5;
    static constexpr int DatacenterIdBits = 5;
    static constexpr int SequenceBits = 12;
    // ���ֵ����
    static constexpr int32_t MaxWorkerId = -1LL ^ (-1LL << WorkerIdBits);          // 31
    static constexpr int32_t MaxDatacenterId = -1LL ^ (-1LL << DatacenterIdBits);  // 31
    static constexpr int32_t SequenceMask = -1LL ^ (-1LL << SequenceBits);         // 4095
    // λ����
    static constexpr int WorkerIdShift = SequenceBits;                            // 12
    static constexpr int DatacenterIdShift = SequenceBits + WorkerIdBits;         // 17
    static constexpr int TimestampLeftShift = SequenceBits + WorkerIdBits + DatacenterIdBits; // 22
    // ��Ա����
    int32_t worker_id_;
    int32_t datacenter_id_;
    int32_t sequence_ = 0LL;
    int64_t last_timestamp_ = -1LL;
    std::mutex mutex_;
    // �ȴ���һ����
    int64_t TilNextMillis(int64_t last_timestamp) {
        auto timestamp = TimeGen();
        while (timestamp <= last_timestamp) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            timestamp = TimeGen();
        }
        return timestamp;
    }
    // ��ȡ��ǰʱ���
    static int64_t TimeGen() {
        using namespace std::chrono;
        auto now = system_clock::now();
        return duration_cast<milliseconds>(now.time_since_epoch()).count();
    }
};