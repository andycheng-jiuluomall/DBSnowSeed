using System;
using System.Collections.Generic;
using System.Data;
using System.Data.SqlClient;
using System.Data.SqlTypes;
using Microsoft.SqlServer.Server;

public partial class UserDefinedFunctions
{
    private static readonly Dictionary<string, IdWorker> pools = new();

    [SqlFunction]
    public static SqlInt64 getSnowID(SqlInt32 workerId,SqlInt32 datacenterId)
    {
        var key = workerId.Value + "-" + datacenterId.Value;
        if (!pools.ContainsKey(key)) pools.Add(key, new(workerId.Value, datacenterId.Value));
        return new SqlInt64(pools[key].NextId());
    }
}
