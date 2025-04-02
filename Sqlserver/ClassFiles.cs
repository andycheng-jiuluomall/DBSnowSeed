using System;
public class DisposableAction : IDisposable
{
    readonly Action _action;

    public DisposableAction(Action action)
    {
        if (action == null)
            throw new ArgumentNullException("action");
        _action = action;
    }

    public void Dispose()
    {
        _action();
    }
}
public class IdWorker
{
    //��׼ʱ��
    public const long Twepoch = 1288834974657L;
    //������ʶλ��
    const int WorkerIdBits = 5;
    //���ݱ�־λ��
    const int DatacenterIdBits = 5;
    //���к�ʶλ��
    const int SequenceBits = 12;
    //����ID���ֵ
    const long MaxWorkerId = -1L ^ (-1L << WorkerIdBits);
    //���ݱ�־ID���ֵ
    const long MaxDatacenterId = -1L ^ (-1L << DatacenterIdBits);
    //���к�ID���ֵ
    private const long SequenceMask = -1L ^ (-1L << SequenceBits);
    //����IDƫ����12λ
    private const int WorkerIdShift = SequenceBits;
    //����IDƫ����17λ
    private const int DatacenterIdShift = SequenceBits + WorkerIdBits;
    //ʱ���������22λ
    public const int TimestampLeftShift = SequenceBits + WorkerIdBits + DatacenterIdBits;

    private long _sequence = 0L;
    private long _lastTimestamp = -1L;

    public long WorkerId { get; protected set; }
    public long DatacenterId { get; protected set; }
    public long Sequence
    {
        get { return _sequence; }
        internal set { _sequence = value; }
    }
    public IdWorker(long workerId, long datacenterId, long sequence = 0L)
    {
        // ���������Χ���׳��쳣
        if (workerId > MaxWorkerId || workerId < 0)
        {
            throw new ArgumentException(string.Format("worker Id �������0���Ҳ��ܴ���MaxWorkerId�� {0}", MaxWorkerId));
        }

        if (datacenterId > MaxDatacenterId || datacenterId < 0)
        {
            throw new ArgumentException(string.Format("region Id �������0���Ҳ��ܴ���MaxWorkerId�� {0}", MaxDatacenterId));
        }

        //�ȼ����ٸ�ֵ
        WorkerId = workerId;
        DatacenterId = datacenterId;
        _sequence = sequence;
    }

    readonly object _lock = new Object();
    public virtual long NextId()
    {
        lock (_lock)
        {
            var timestamp = TimeGen();
            if (timestamp < _lastTimestamp)
            {
                throw new Exception(string.Format("ʱ������������һ������ID��ʱ���.  �ܾ�Ϊ{0}��������id", _lastTimestamp - timestamp));
            }

            //����ϴ�����ʱ��͵�ǰʱ����ͬ,��ͬһ������
            if (_lastTimestamp == timestamp)
            {
                //sequence��������sequenceMask����һ�£�ȥ����λ
                _sequence = (_sequence + 1) & SequenceMask;
                //�ж��Ƿ����,Ҳ����ÿ�����ڳ���1024����Ϊ1024ʱ����sequenceMask���룬sequence�͵���0
                if (_sequence == 0)
                {
                    //�ȴ�����һ����
                    timestamp = TilNextMillis(_lastTimestamp);
                }
            }
            else
            {
                //������ϴ�����ʱ�䲻ͬ,����sequence��������һ���뿪ʼ��sequence�������´�0��ʼ�ۼ�,
                //Ϊ�˱�֤β������Ը���һЩ,���һλ��������һ�������
                _sequence = 0;//new Random().Next(10);
            }
            _lastTimestamp = timestamp;
            return ((timestamp - Twepoch) << TimestampLeftShift) | (DatacenterId << DatacenterIdShift) | (WorkerId << WorkerIdShift) | _sequence;
        }
    }

    // ��ֹ������ʱ���֮ǰ��ʱ�仹ҪС������NTP�ز������⣩,��������������.
    protected virtual long TilNextMillis(long lastTimestamp)
    {
        var timestamp = TimeGen();
        while (timestamp <= lastTimestamp)
        {
            timestamp = TimeGen();
        }
        return timestamp;
    }

    // ��ȡ��ǰ��ʱ���
    protected virtual long TimeGen()
    {
        var Jan1st1970 = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc);
        return (long)(DateTime.UtcNow - Jan1st1970).TotalMilliseconds;
    }
}
public class SnowSeed
{
    private readonly static SnowSeed Singleton = new SnowSeed();
    private IdWorker snowFlake;
    SnowSeed()
    {
        //����������� 
        var workerId = 0;
        //�������ı��
        var datacenterId = 0;
        snowFlake = new IdWorker(workerId, datacenterId);
    }
    IdWorker getInstance()
    {
        return snowFlake;
    }

    public static long NewID
    {
        get
        {
            return Singleton.getInstance().NextId();
        }
    }
}