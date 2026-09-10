using System;
using System.Collections.Generic;
using System.Runtime.Serialization;

[DataContract]
public sealed class QuotaSnapshot
{
    public int FiveHourRemaining;
    public long FiveHourReset;
    public int WeeklyRemaining;
    public long WeeklyReset;
    public int ResetCards;
    public long Now;
    public int TimeZoneOffsetMinutes;
}

[DataContract]
public sealed class RpcRateLimitsResponse
{
    [DataMember(Name = "result")]
    public RateLimitResponse Result;
}

[DataContract]
public sealed class RateLimitResponse
{
    [DataMember(Name = "rateLimits")]
    public RateLimitSnapshot RateLimits;

    [DataMember(Name = "rateLimitsByLimitId")]
    public Dictionary<string, RateLimitSnapshot> RateLimitsByLimitId;

    [DataMember(Name = "rateLimitResetCredits")]
    public ResetCreditsSummary ResetCredits;
}

[DataContract]
public sealed class RateLimitSnapshot
{
    [DataMember(Name = "primary")]
    public RateLimitWindow Primary;

    [DataMember(Name = "secondary")]
    public RateLimitWindow Secondary;
}

[DataContract]
public sealed class RateLimitWindow
{
    [DataMember(Name = "usedPercent")]
    public int? UsedPercent;

    [DataMember(Name = "windowDurationMins")]
    public long? WindowDurationMins;

    [DataMember(Name = "resetsAt")]
    public long? ResetsAt;
}

[DataContract]
public sealed class ResetCreditsSummary
{
    [DataMember(Name = "availableCount")]
    public int? AvailableCount;
}
