public interface IApplicationAdapter
{
    string Name { get; }
    string StartCommand { get; }
    string StopCommand { get; }
    bool IsApplicationTraffic(string line);
    bool IsReady(string line);
    bool IsStopped(string line);
}
