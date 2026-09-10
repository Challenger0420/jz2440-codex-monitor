using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.Serialization.Json;
using System.Text;

public sealed class RealCodexProvider : IQuotaProvider
{
    private readonly Process process;
    private readonly StreamWriter input;
    private readonly StreamReader output;
    private readonly DataContractJsonSerializer responseSerializer =
        new DataContractJsonSerializer(typeof(RpcRateLimitsResponse));

    public RealCodexProvider()
    {
        ProcessStartInfo startInfo = new ProcessStartInfo
        {
            FileName = "codex",
            Arguments = "app-server --stdio",
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true
        };

        process = Process.Start(startInfo);
        if (process == null)
            throw new InvalidOperationException("Unable to start codex app-server.");

        input = process.StandardInput;
        input.AutoFlush = true;
        output = process.StandardOutput;

        Send("{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{" +
              "\"clientInfo\":{\"name\":\"jz2440-quota-bridge\",\"version\":\"0.1.0\"}," +
              "\"capabilities\":{\"experimentalApi\":true}}}");
        WaitForResponseId(1, 10000);
        Send("{\"jsonrpc\":\"2.0\",\"method\":\"initialized\",\"params\":{}}");
    }

    public QuotaSnapshot ReadQuota()
    {
        Send("{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"account/rateLimits/read\",\"params\":null}");
        string line = WaitForResponseId(2, 15000);
        if (line.IndexOf("\"error\"", StringComparison.Ordinal) >= 0)
            throw new InvalidOperationException("app-server rate-limit read returned an error: " + line);

        RpcRateLimitsResponse envelope;
        using (MemoryStream stream = new MemoryStream(Encoding.UTF8.GetBytes(line)))
            envelope = (RpcRateLimitsResponse)responseSerializer.ReadObject(stream);

        if (envelope == null || envelope.Result == null)
            throw new InvalidOperationException("app-server returned no rate-limit result.");

        RateLimitSnapshot snapshot = envelope.Result.RateLimits;
        if (envelope.Result.RateLimitsByLimitId != null &&
            envelope.Result.RateLimitsByLimitId.ContainsKey("codex"))
            snapshot = envelope.Result.RateLimitsByLimitId["codex"];
        if (snapshot == null)
            throw new InvalidOperationException("No codex rate-limit snapshot was returned.");

        RateLimitWindow fiveHour = FindWindow(snapshot, 300);
        RateLimitWindow weekly = FindWindow(snapshot, 10080);
        if (fiveHour == null || weekly == null || !fiveHour.UsedPercent.HasValue || !weekly.UsedPercent.HasValue ||
            fiveHour.UsedPercent.Value < 0 || fiveHour.UsedPercent.Value > 100 ||
            weekly.UsedPercent.Value < 0 || weekly.UsedPercent.Value > 100)
            throw new InvalidOperationException("Required 300-minute or 10080-minute window is unavailable.");

        DateTimeOffset now = DateTimeOffset.Now;
        return new QuotaSnapshot
        {
            FiveHourRemaining = Remaining(fiveHour.UsedPercent.Value),
            FiveHourReset = fiveHour.ResetsAt.HasValue ? fiveHour.ResetsAt.Value : 0,
            WeeklyRemaining = Remaining(weekly.UsedPercent.Value),
            WeeklyReset = weekly.ResetsAt.HasValue ? weekly.ResetsAt.Value : 0,
            ResetCards = envelope.Result.ResetCredits != null && envelope.Result.ResetCredits.AvailableCount.HasValue
                ? envelope.Result.ResetCredits.AvailableCount.Value : -1,
            Now = now.ToUnixTimeSeconds(),
            TimeZoneOffsetMinutes = (int)now.Offset.TotalMinutes
        };
    }

    private static int Remaining(int usedPercent)
    {
        if (usedPercent < 0) usedPercent = 0;
        if (usedPercent > 100) usedPercent = 100;
        int remaining = 100 - usedPercent;
        if (remaining < 0) return 0;
        if (remaining > 100) return 100;
        return remaining;
    }

    private static RateLimitWindow FindWindow(RateLimitSnapshot snapshot, long minutes)
    {
        if (snapshot.Primary != null && snapshot.Primary.WindowDurationMins == minutes)
            return snapshot.Primary;
        if (snapshot.Secondary != null && snapshot.Secondary.WindowDurationMins == minutes)
            return snapshot.Secondary;
        return null;
    }

    private void Send(string json)
    {
        if (process.HasExited)
            throw new InvalidOperationException("codex app-server exited unexpectedly.");
        input.WriteLine(json);
    }

    private string WaitForResponseId(int id, int timeoutMilliseconds)
    {
        Stopwatch stopwatch = Stopwatch.StartNew();
        while (stopwatch.ElapsedMilliseconds < timeoutMilliseconds)
        {
            int remaining = timeoutMilliseconds - (int)stopwatch.ElapsedMilliseconds;
            System.Threading.Tasks.Task<string> readTask = output.ReadLineAsync();
            if (!readTask.Wait(Math.Max(1, remaining)))
                throw new TimeoutException("Timed out waiting for app-server response " + id + ".");
            string line = readTask.Result;
            if (line == null)
                throw new InvalidOperationException("codex app-server closed its output.");
            string marker = "\"id\":" + id.ToString();
            if (line.IndexOf(marker, StringComparison.Ordinal) >= 0)
                return line;
        }
        throw new TimeoutException("Timed out waiting for app-server response " + id + ".");
    }

    public void Dispose()
    {
        try { input.Close(); } catch (Exception) { }
        try
        {
            if (!process.HasExited)
                process.Kill();
        }
        catch (Exception) { }
        process.Dispose();
    }
}
