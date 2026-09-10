using System;
using System.Collections.Generic;
using System.Threading;

public static class Program
{
    public static int Main(string[] args)
    {
        string portOverride = null;
        int interval = 60;
        int count = 0;
        bool dryRun = false;
        bool selfTest = false;

        for (int i = 0; i < args.Length; i++)
        {
            if (args[i] == "--port" && i + 1 < args.Length) portOverride = args[++i];
            else if (args[i] == "--interval" && i + 1 < args.Length) interval = ParsePositive(args[++i], "interval");
            else if (args[i] == "--count" && i + 1 < args.Length) count = ParseNonNegative(args[++i], "count");
            else if (args[i] == "--dry-run") dryRun = true;
            else if (args[i] == "--self-test") selfTest = true;
            else if (args[i] == "--help") { PrintUsage(); return 0; }
            else throw new ArgumentException("Unknown argument: " + args[i]);
        }
        if (selfTest)
        {
            ProtocolParserTests.Run();
            ProviderTests.Run();
            ComPortDiscoveryTests.Run();
            return 0;
        }

        if (dryRun)
        {
            using (IQuotaProvider provider = new RealCodexProvider())
            {
                QuotaSnapshot snapshot = provider.ReadQuota();
                Console.Write(ProtocolEncoder.Encode(snapshot));
                return 0;
            }
        }

        IQuotaProvider liveProvider = null;
        ISerialTransport liveSerial = null;
        IComPortDiscovery discovery = new ProlificComPortDiscovery();
        int sent = 0;
        try
        {
            while (count == 0 || sent < count)
            {
                try
                {
                    if (liveSerial == null)
                    {
                        string portName = portOverride ?? FindSinglePort(discovery);
                        if (portName == null)
                            throw new InvalidOperationException("No unique authentic PL2303 COM port is available.");
                        liveSerial = SerialTransport.Open(portName);
                        Console.WriteLine("serial connected: {0}", portName);
                    }
                    if (liveProvider == null)
                        liveProvider = new RealCodexProvider();
                    QuotaSnapshot snapshot = liveProvider.ReadQuota();
                    string frame = ProtocolEncoder.Encode(snapshot);
                    liveSerial.Write(frame);
                    sent++;
                    Console.WriteLine("sent #{0}: {1}", sent, frame.TrimEnd('\n'));
                }
                catch (Exception error)
                {
                    Console.Error.WriteLine("cycle failed; retrying in {0}s: {1}", interval, error.Message);
                    if (liveSerial != null) { liveSerial.Dispose(); liveSerial = null; }
                    if (liveProvider != null) { liveProvider.Dispose(); liveProvider = null; }
                }
                if (count != 0 && sent >= count) break;
                Thread.Sleep(interval * 1000);
            }
        }
        finally
        {
            if (liveSerial != null) liveSerial.Dispose();
            if (liveProvider != null) liveProvider.Dispose();
        }
        return 0;
    }

    private static string FindSinglePort(IComPortDiscovery discovery)
    {
        IList<ComPortCandidate> candidates = discovery.Find();
        if (candidates.Count == 1) return candidates[0].PortName;
        if (candidates.Count == 0)
        {
            Console.Error.WriteLine("no matching PL2303 device; waiting for the next interval");
            return null;
        }
        Console.Error.WriteLine("multiple matching PL2303 devices; refusing to choose automatically:");
        foreach (ComPortCandidate candidate in candidates)
            Console.Error.WriteLine("  {0}", candidate);
        return null;
    }

    private static int ParsePositive(string text, string name)
    {
        int value = ParseNonNegative(text, name);
        if (value <= 0) throw new ArgumentException(name + " must be positive.");
        return value;
    }

    private static int ParseNonNegative(string text, string name)
    {
        int value;
        if (!int.TryParse(text, out value) || value < 0)
            throw new ArgumentException(name + " must be a non-negative integer.");
        return value;
    }

    private static void PrintUsage()
    {
        Console.WriteLine("Codex quota bridge");
        Console.WriteLine("  --port COMx       serial port override (default: auto-discover VID_067B/PID_2303)");
        Console.WriteLine("  --interval 60     seconds between reads (default 60)");
        Console.WriteLine("  --dry-run         fetch and print one real frame");
        Console.WriteLine("  --count N         send N frames, useful for hardware test");
        Console.WriteLine("  --self-test       run host-side protocol parser tests");
    }
}
