using System;

public static class ProtocolEncoder
{
    public static string Encode(QuotaSnapshot snapshot)
    {
        if (snapshot == null)
            throw new ArgumentNullException("snapshot");
        if (snapshot.FiveHourRemaining < 0 || snapshot.FiveHourRemaining > 100 ||
            snapshot.WeeklyRemaining < 0 || snapshot.WeeklyRemaining > 100)
            throw new InvalidOperationException("Quota percentage is outside 0..100.");
        if (snapshot.ResetCards < -1)
            throw new InvalidOperationException("Reset card count is below -1.");
        if (snapshot.FiveHourReset < 0 || snapshot.FiveHourReset > uint.MaxValue ||
            snapshot.WeeklyReset < 0 || snapshot.WeeklyReset > uint.MaxValue ||
            snapshot.Now < 0 || snapshot.Now > uint.MaxValue)
            throw new InvalidOperationException("Timestamp is outside the target uint32 range.");
        if (snapshot.TimeZoneOffsetMinutes < -1440 || snapshot.TimeZoneOffsetMinutes > 1440)
            throw new InvalidOperationException("Time zone offset is outside the supported range.");

        return string.Format(
            "<CQM1|5H={0}|5HR={1}|W={2}|WR={3}|RC={4}|NOW={5}|TZ={6}>\n",
            snapshot.FiveHourRemaining, snapshot.FiveHourReset,
            snapshot.WeeklyRemaining, snapshot.WeeklyReset,
            snapshot.ResetCards, snapshot.Now, snapshot.TimeZoneOffsetMinutes);
    }
}
