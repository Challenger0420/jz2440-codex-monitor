using System;
using System.Collections.Generic;

public static class ComPortDiscoveryTests
{
    public static int Run()
    {
        IList<ComPortCandidate> candidates = ProlificComPortDiscovery.ParseCandidates(
            new[]
            {
                new PnpSerialDevice
                {
                    Name = "Prolific USB-to-Serial Comm Port (COM6)",
                    PnpDeviceId = "USB\\VID_067B&PID_2303\\INSTANCE1",
                    HardwareIds = new[] { "USB\\VID_067B&PID_2303" }
                },
                new PnpSerialDevice
                {
                    Name = "Bluetooth Standard Serial Port (COM3)",
                    PnpDeviceId = "BTH\\OTHER",
                    HardwareIds = new[] { "BTH\\OTHER" }
                },
                new PnpSerialDevice
                {
                    Name = "PL2303 (COM5)",
                    PnpDeviceId = "USB\\VID_067B&PID_2303\\INSTANCE2",
                    HardwareIds = new[] { "USB\\VID_067B&PID_2303" }
                }
            });
        if (candidates.Count != 2 || candidates[0].PortName != "COM5" || candidates[1].PortName != "COM6")
            throw new InvalidOperationException("COM discovery candidate parsing failed");
        Console.WriteLine("COM discovery self-test: PASS");
        return 0;
    }
}
