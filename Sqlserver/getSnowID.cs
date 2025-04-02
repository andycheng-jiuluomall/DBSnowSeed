using System;
using System.Collections.Generic;
using System.Data;
using System.Data.SqlClient;
using System.Data.SqlTypes;
using Microsoft.SqlServer.Server;

public partial class UserDefinedFunctions
{
    private static readonly Dictionary<string, IdWorker> pools = new Dictionary<string, IdWorker>();

    [Microsoft.SqlServer.Server.SqlFunction]
    public static SqlString getSnowID(SqlInt32 workerId,SqlInt32 datacenterId)
    {
        var key = workerId.Value + "-" + datacenterId.Value;
        if (!pools.ContainsKey(key))
        {
            pools.Add(key, new IdWorker(workerId.Value, datacenterId.Value));
        }
        var result = pools[key].NextId();
        return new SqlString(result.ToString());
    }
}
