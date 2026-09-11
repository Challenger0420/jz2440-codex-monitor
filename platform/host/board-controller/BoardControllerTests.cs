using System;
using System.Collections.Generic;

public static class BoardControllerTests
{
    private sealed class FakeTransport : ISerialTransport
    {
        private readonly Queue<string> incoming = new Queue<string>();
        public readonly List<string> Writes = new List<string>();
        public bool EmitRequestWithReady;
        public string PortName { get { return "FAKE"; } }
        public void Add(string text) { incoming.Enqueue(text); }
        public string ReadAvailable() { return incoming.Count == 0 ? "" : incoming.Dequeue(); }
        public void Write(string frame)
        {
            Writes.Add(frame);
            if (frame.StartsWith("echo ", StringComparison.Ordinal))
                Add(frame.Substring(5).TrimEnd('\n') + "\n");
            else if (frame == "/opt/jz2440/bin/appctl list\n") Add("codex-monitor\n");
            else if (frame == "/opt/jz2440/bin/appctl start codex-monitor\n")
                Add(EmitRequestWithReady
                    ? "<APPREADY|codex-monitor>\r\n<CQMREQ|V=1>\n"
                    : "<APPREADY|codex-monitor>\n");
            else if (frame == "<CQMQUIT>\n") Add("<APPSTOP|codex-monitor|RC=0>\n");
        }
        public void Dispose() { }
    }

    public static int Run()
    {
        FakeTransport fake = new FakeTransport();
        using (fake)
        {
            fake.Add("login: \n# \n");
            BoardController controller = new BoardController(fake, new CodexMonitorAdapter());
            if (!controller.StartApplication(1000) ||
                controller.Mode != BoardControllerMode.Application)
                throw new InvalidOperationException("controller start flow failed");
            if (fake.Writes.Count != 2 || fake.Writes[1] != "/opt/jz2440/bin/appctl start codex-monitor\n")
                throw new InvalidOperationException("controller console flow failed");
            if (!controller.StopApplication(1000) || controller.Mode != BoardControllerMode.Console)
                throw new InvalidOperationException("controller stop flow failed");
            if (fake.Writes[fake.Writes.Count - 1].IndexOf("echo JZ2440_CTL_SYNC_", StringComparison.Ordinal) != 0)
                throw new InvalidOperationException("controller post-stop sync failed");
        }

        fake = new FakeTransport();
        using (fake)
        {
            fake.Add("# ready\n");
            BoardController controller = new BoardController(fake, new CodexMonitorAdapter());
            IList<string> apps = controller.ListApplications(1000);
            if (apps == null || apps.Count != 1 || apps[0] != "codex-monitor")
                throw new InvalidOperationException("board application list flow failed");
        }

        fake = new FakeTransport();
        using (fake)
        {
            fake.Add("noise\n<CQMREQ|V=1>\n");
            BoardController controller = new BoardController(fake, new CodexMonitorAdapter());
            if (controller.Observe(1000) != BoardControllerMode.Application)
                throw new InvalidOperationException("request mode detection failed");
        }

        fake = new FakeTransport { EmitRequestWithReady = true };
        using (fake)
        {
            fake.Add("# ready\n");
            BoardController controller = new BoardController(fake, new CodexMonitorAdapter());
            if (!controller.StartApplication(1000) ||
                controller.Mode != BoardControllerMode.Application ||
                controller.TakePendingApplicationData() != "<CQMREQ|V=1>\n")
                throw new InvalidOperationException("APPREADY same-chunk request was discarded");
        }

        CodexMonitorAdapter adapter = new CodexMonitorAdapter();
        if (adapter.IsReady("<APPREADY|other-app>\n") ||
            adapter.IsStopped("<APPSTOP|codex-monitor|RC=x>\n"))
            throw new InvalidOperationException("malformed lifecycle marker was accepted");

        fake = new FakeTransport();
        using (fake)
        {
            QuotaSnapshot value = new QuotaSnapshot
            {
                FiveHourRemaining = 78, FiveHourReset = 1788966311,
                WeeklyRemaining = 87, WeeklyReset = 1789448757,
                ResetCards = 3, Now = 1788958655, TimeZoneOffsetMinutes = 480
            };
            BoardApplicationSession session = new BoardApplicationSession(
                fake, delegate { return value; });
            session.Consume("noise\n");
            if (session.Responses != 0 || fake.Writes.Count != 0)
                throw new InvalidOperationException("application mode sent data without request");
            session.Consume("<CQMREQ|");
            session.Consume("V=1>\n<CQMREQ|V=1>\n<APPSTOP|codex-monitor|RC=0>\n");
            if (session.Responses != 2 || fake.Writes.Count != 2 || !session.Stopped)
                throw new InvalidOperationException("application request/response transcript failed");
        }

        fake = new FakeTransport();
        using (fake)
        {
            BoardController controller = new BoardController(fake, new CodexMonitorAdapter());
            if (controller.StartApplication(100) || fake.Writes.Count != 0)
                throw new InvalidOperationException("unknown mode was allowed to start an application");
        }
        Console.WriteLine("board controller simulation: PASS");
        return 0;
    }
}
