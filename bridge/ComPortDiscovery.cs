using System;
using System.Collections.Generic;
using System.Linq;
using System.Management;
using System.Text.RegularExpressions;

public sealed class PnpSerialDevice
{
    public string Name;
    public string PnpDeviceId;
    public string[] HardwareIds;
}

public sealed class ComPortCandidate
{
    public string PortName;
    public string Name;
    public string PnpDeviceId;
    public string HardwareId;

    public override string ToString()
    {
        return PortName + " - " + Name + " [" + PnpDeviceId + "]";
    }
}

public interface IComPortDiscovery
{
    IList<ComPortCandidate> Find();
}

public sealed class ProlificComPortDiscovery : IComPortDiscovery
{
    private const string VidPid = "USB\\VID_067B&PID_2303";
    private static readonly Regex PortPattern = new Regex("\\((COM[0-9]+)\\)", RegexOptions.IgnoreCase);

    public IList<ComPortCandidate> Find()
    {
        List<PnpSerialDevice> devices = new List<PnpSerialDevice>();
        using (ManagementObjectSearcher searcher = new ManagementObjectSearcher(
            "SELECT Name, PNPDeviceID, HardwareID FROM Win32_PnPEntity"))
        using (ManagementObjectCollection results = searcher.Get())
        {
            foreach (ManagementObject item in results)
            {
                devices.Add(new PnpSerialDevice
                {
                    Name = item["Name"] as string,
                    PnpDeviceId = item["PNPDeviceID"] as string,
                    HardwareIds = ToStringArray(item["HardwareID"])
                });
            }
        }
        return ParseCandidates(devices);
    }

    internal static IList<ComPortCandidate> ParseCandidates(IEnumerable<PnpSerialDevice> devices)
    {
        List<ComPortCandidate> candidates = new List<ComPortCandidate>();
        foreach (PnpSerialDevice device in devices)
        {
            string deviceId = device.PnpDeviceId ?? string.Empty;
            bool matches = deviceId.StartsWith(VidPid, StringComparison.OrdinalIgnoreCase);
            string hardwareId = (device.HardwareIds ?? new string[0])
                .FirstOrDefault(value => value != null && value.StartsWith(VidPid, StringComparison.OrdinalIgnoreCase));
            if (!matches && hardwareId == null) continue;

            Match portMatch = PortPattern.Match(device.Name ?? string.Empty);
            if (!portMatch.Success) continue;
            string port = portMatch.Groups[1].Value.ToUpperInvariant();
            if (candidates.Any(candidate => candidate.PortName == port && candidate.PnpDeviceId == deviceId)) continue;
            candidates.Add(new ComPortCandidate
            {
                PortName = port,
                Name = device.Name ?? "",
                PnpDeviceId = deviceId,
                HardwareId = hardwareId ?? VidPid
            });
        }
        return candidates.OrderBy(candidate => ComNumber(candidate.PortName)).ThenBy(candidate => candidate.PortName).ToList();
    }

    private static string[] ToStringArray(object value)
    {
        string[] values = value as string[];
        return values ?? new string[0];
    }

    private static int ComNumber(string port)
    {
        int value;
        return int.TryParse(port.Substring(3), out value) ? value : int.MaxValue;
    }
}
