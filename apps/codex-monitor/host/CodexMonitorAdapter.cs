public sealed class CodexMonitorAdapter : IApplicationAdapter
{
    public string Name { get { return "codex-monitor"; } }
    public string StartCommand { get { return "/opt/jz2440/bin/appctl start codex-monitor\n"; } }
    public string StopCommand { get { return "<CQMQUIT>\n"; } }
    public bool IsApplicationTraffic(string line) { return ProtocolParser.IsRequest(line); }
    public bool IsReady(string line) { return BoardProtocol.IsAppReady(line); }
    public bool IsStopped(string line) { return BoardProtocol.IsAppStop(line); }
}
