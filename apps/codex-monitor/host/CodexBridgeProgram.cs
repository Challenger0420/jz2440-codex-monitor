using System;
using System.Collections.Generic;
using System.Text;
using System.Threading;

public static class Program
{
    public static int Main(string[] args)
    {
        if (args.Length > 0 && args[0] == "board")
            return RunBoardCommand(args);

        string portOverride = null;
        int interval = 60;
        int count = 0;
        bool dryRun = false;
        bool providerTest = false;
        bool providerTrace = false;
        bool selfTest = false;
        bool quit = false;

        for (int i = 0; i < args.Length; i++)
        {
            if (args[i] == "--port" && i + 1 < args.Length) portOverride = args[++i];
            else if (args[i] == "--interval" && i + 1 < args.Length) interval = ParsePositive(args[++i], "interval");
            else if (args[i] == "--count" && i + 1 < args.Length) count = ParseNonNegative(args[++i], "count");
            else if (args[i] == "--dry-run") dryRun = true;
            else if (args[i] == "--provider-test") providerTest = true;
            else if (args[i] == "--provider-trace") providerTrace = true;
            else if (args[i] == "--self-test") selfTest = true;
            else if (args[i] == "--quit") quit = true;
            else if (args[i] == "--help") { PrintUsage(); return 0; }
            else throw new ArgumentException("Unknown argument: " + args[i]);
        }
        if (selfTest)
        {
            ProtocolParserTests.Run();
            RequestDrivenSessionTests.Run();
            BoardControllerTests.Run();
            ProviderTests.Run();
            ProviderTransportContractTests.Run();
            ComPortDiscoveryTests.Run();
            return 0;
        }

        if (providerTest)
            return RunProviderTest();
        if (providerTrace)
            return ProviderTransportDiagnostics.Run();

        if (dryRun)
        {
            try
            {
                using (IQuotaProvider provider = new RealCodexProvider())
                {
                    QuotaSnapshot snapshot = provider.ReadQuota();
                    Console.Write(ProtocolEncoder.Encode(snapshot));
                    return 0;
                }
            }
            catch (Exception error)
            {
                Console.Error.WriteLine("dry-run failed: {0}: {1}", error.GetType().FullName, error.Message);
                return 1;
            }
        }

        if (quit)
            return SendQuit(portOverride, new ProlificComPortDiscovery());

        IQuotaProvider liveProvider = null;
        ISerialTransport liveSerial = null;
        IComPortDiscovery discovery = new ProlificComPortDiscovery();
        RequestDrivenSession requestSession = null;
        int sent = 0;
        DateTime nextConnectAttempt = DateTime.MinValue;
        try
        {
            while (count == 0 || sent < count)
            {
                try
                {
                    if (liveSerial == null)
                    {
                        if (DateTime.UtcNow < nextConnectAttempt)
                        {
                            Thread.Sleep(100);
                            continue;
                        }
                        string portName = portOverride ?? FindSinglePort(discovery);
                        if (portName == null)
                        {
                            nextConnectAttempt = DateTime.UtcNow.AddSeconds(interval);
                            Thread.Sleep(100);
                            continue;
                        }
                        liveSerial = SerialTransport.Open(portName);
                        requestSession = new RequestDrivenSession();
                        requestSession.RequestReceived += delegate
                        {
                            QuotaSnapshot snapshot;
                            try
                            {
                                if (liveProvider == null)
                                    liveProvider = new RealCodexProvider();
                                snapshot = liveProvider.ReadQuota();
                            }
                            catch (Exception error)
                            {
                                Console.Error.WriteLine("request failed; waiting for the next request: {0}", error.Message);
                                if (liveProvider != null) { liveProvider.Dispose(); liveProvider = null; }
                                return;
                            }
                            string response = ProtocolEncoder.Encode(snapshot);
                            liveSerial.Write(response);
                            sent++;
                            Console.WriteLine("request -> sent #{0}: {1}", sent, response.TrimEnd('\n'));
                        };
                        Console.WriteLine("serial connected: {0}; waiting for CQMREQ", portName);
                    }

                    string incoming = liveSerial.ReadAvailable();
                    requestSession.Consume(incoming);
                    if (count != 0 && sent >= count) break;
                    Thread.Sleep(100);
                }
                catch (Exception error)
                {
                    Console.Error.WriteLine("serial cycle failed; waiting {0}s before reconnect: {1}", interval, error.Message);
                    if (liveSerial != null) { liveSerial.Dispose(); liveSerial = null; }
                    if (liveProvider != null) { liveProvider.Dispose(); liveProvider = null; }
                    requestSession = null;
                    nextConnectAttempt = DateTime.UtcNow.AddSeconds(interval);
                }
            }
        }
        finally
        {
            if (liveSerial != null) liveSerial.Dispose();
            if (liveProvider != null) liveProvider.Dispose();
        }
        return 0;
    }

    private static int SendQuit(string portOverride, IComPortDiscovery discovery)
    {
        string portName = portOverride ?? FindSinglePort(discovery);
        if (portName == null)
            throw new InvalidOperationException("No unique authentic PL2303 COM port is available for --quit.");
        using (ISerialTransport serial = SerialTransport.Open(portName))
        {
            serial.Write("<CQMQUIT>\n");
            Console.WriteLine("sent CQMQUIT to {0}", portName);
        }
        return 0;
    }

    private static int RunProviderTest()
    {
        Console.WriteLine("PROVIDER_TEST transport=direct-exe-provider-only");
        for (int attempt = 1; attempt <= 5; attempt++)
        {
            try
            {
                using (IQuotaProvider provider = new RealCodexProvider())
                {
                    QuotaSnapshot snapshot = provider.ReadQuota();
                    Console.WriteLine("PROVIDER_TEST attempt={0} handshake=PASS quota=PASS 5H={1} W={2} RC={3}",
                        attempt, snapshot.FiveHourRemaining, snapshot.WeeklyRemaining, snapshot.ResetCards);
                }
            }
            catch (Exception error)
            {
                Console.Error.WriteLine("PROVIDER_TEST attempt={0} handshake_or_quota=FAIL type={1} message={2}",
                    attempt, error.GetType().FullName, error.Message);
                return 1;
            }
        }
        Console.WriteLine("PROVIDER_TEST=PASS attempts=5");
        return 0;
    }

    private static int RunBoardCommand(string[] args)
    {
        string command = args.Length > 1 ? args[1] : "";
        string app = args.Length > 2 ? args[2] : "";
        string portOverride = null;
        bool consoleConfirmed = false;
        bool applicationConfirmed = false;
        bool serialTrace = false;
        for (int i = 2; i < args.Length; i++)
        {
            if (args[i] == "--port" && i + 1 < args.Length) portOverride = args[++i];
            else if (args[i] == "--console") consoleConfirmed = true;
            else if (args[i] == "--application") applicationConfirmed = true;
            else if (args[i] == "--serial-trace") serialTrace = true;
        }
        if (command == "list")
        {
            string listPort = portOverride ?? FindSinglePort(new ProlificComPortDiscovery());
            if (listPort == null) return 1;
            using (ISerialTransport listSerial = SerialTransport.Open(listPort))
            {
                BoardController listController = new BoardController(listSerial, new CodexMonitorAdapter());
                IList<string> apps = listController.ListApplications(5000);
                if (apps == null)
                    throw new InvalidOperationException("board is not in a confirmed console mode or appctl list did not complete");
                foreach (string entry in apps) Console.WriteLine(entry);
                return 0;
            }
        }
        if (command != "status" && command != "start" && command != "stop")
        {
            PrintBoardUsage();
            return 2;
        }
        if (command == "start" && app != "codex-monitor")
        {
            Console.Error.WriteLine("only codex-monitor is registered in board controller v1");
            return 2;
        }

        string portName = portOverride ?? FindSinglePort(new ProlificComPortDiscovery());
        if (portName == null) return 1;
            using (ISerialTransport openedSerial = SerialTransport.Open(portName))
            {
                BoardController controller = null;
                ISerialTransport serial = openedSerial;
                if (serialTrace)
                    serial = new SerialTraceTransport(openedSerial, delegate
                    {
                        return controller == null ? "Unknown" : controller.Mode.ToString();
                    });
                controller = new BoardController(serial, new CodexMonitorAdapter());
                if (consoleConfirmed) controller.ConfirmConsoleMode();
                else if (applicationConfirmed) controller.ConfirmApplicationMode();
            if (command == "status")
            {
                BoardControllerMode mode = controller.Observe(1500);
                Console.WriteLine("port={0} mode={1}", portName, mode);
                return 0;
            }
            if (command == "start")
            {
                if (!controller.StartApplication(15000))
                    throw new InvalidOperationException("did not receive APPREADY from appctl");
                Console.WriteLine("APPREADY observed; entering APPLICATION_MODE_CODEX");
                return ServeCodexApplication(serial, controller.TakePendingApplicationData());
            }
            if (!controller.StopApplication(5000))
                throw new InvalidOperationException("board is not in a confirmed codex application mode or APPSTOP was not received");
            Console.WriteLine("APPSTOP observed; console mode restored");
            return 0;
        }
    }

    private static int ServeCodexApplication(ISerialTransport serial, string initialIncoming)
    {
        IQuotaProvider provider = null;
        BoardApplicationSession session = new BoardApplicationSession(serial, delegate
        {
            if (provider == null) provider = new RealCodexProvider();
            return provider.ReadQuota();
        });
        try
        {
            while (true)
            {
                string incoming = initialIncoming;
                initialIncoming = null;
                if (String.IsNullOrEmpty(incoming)) incoming = serial.ReadAvailable();
                try
                {
                    session.Consume(incoming);
                    if (session.Stopped) return 0;
                }
                catch (Exception error)
                {
                    Console.Error.WriteLine("request failed; waiting for the next request: {0}", error.Message);
                    if (provider != null) { provider.Dispose(); provider = null; }
                }
                Thread.Sleep(100);
            }
        }
        finally
        {
            if (provider != null) provider.Dispose();
        }
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
        Console.WriteLine("  --interval 60     reconnect retry interval in seconds (default 60)");
        Console.WriteLine("  --dry-run         fetch and print one real frame");
        Console.WriteLine("  --provider-test   run five real provider-only checks without serial");
        Console.WriteLine("  --provider-trace  inspect one provider stdout/stderr handshake without serial");
        Console.WriteLine("  --count N         respond to N board requests, useful for hardware test");
        Console.WriteLine("  --quit            send one explicit CQMQUIT and exit");
        Console.WriteLine("  --self-test       run host-side protocol parser tests");
        PrintBoardUsage();
    }

    private static void PrintBoardUsage()
    {
        Console.WriteLine("  board list");
        Console.WriteLine("  board status [--port COMx]");
        Console.WriteLine("  board start codex-monitor [--console] [--port COMx]");
        Console.WriteLine("  board stop [--application] [--port COMx]");
        Console.WriteLine("  add --serial-trace to board commands for raw RX diagnostics");
    }
}
