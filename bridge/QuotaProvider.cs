using System;

public interface IQuotaProvider : IDisposable
{
    QuotaSnapshot ReadQuota();
}

public sealed class MockCodexProvider : IQuotaProvider
{
    private readonly QuotaSnapshot snapshot;

    public MockCodexProvider(QuotaSnapshot value)
    {
        if (value == null) throw new ArgumentNullException("value");
        snapshot = value;
    }

    public QuotaSnapshot ReadQuota()
    {
        return new QuotaSnapshot
        {
            FiveHourRemaining = snapshot.FiveHourRemaining,
            FiveHourReset = snapshot.FiveHourReset,
            WeeklyRemaining = snapshot.WeeklyRemaining,
            WeeklyReset = snapshot.WeeklyReset,
            ResetCards = snapshot.ResetCards,
            Now = snapshot.Now,
            TimeZoneOffsetMinutes = snapshot.TimeZoneOffsetMinutes
        };
    }

    public void Dispose() { }
}
