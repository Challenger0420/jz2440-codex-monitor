using System;
using System.Text;

public sealed class BoardApplicationSession
{
    private readonly ISerialTransport serial;
    private readonly Func<QuotaSnapshot> snapshotFactory;
    private readonly StringBuilder pending = new StringBuilder();

    public int Responses { get; private set; }
    public bool Stopped { get; private set; }

    public BoardApplicationSession(ISerialTransport transport, Func<QuotaSnapshot> factory)
    {
        serial = transport;
        snapshotFactory = factory;
    }

    public void Consume(string incoming)
    {
        if (!string.IsNullOrEmpty(incoming)) pending.Append(incoming);
        while (true)
        {
            int newline = pending.ToString().IndexOf('\n');
            if (newline < 0) break;
            string line = pending.ToString(0, newline + 1);
            pending.Remove(0, newline + 1);
            if (ProtocolParser.IsRequest(line))
            {
                QuotaSnapshot snapshot = snapshotFactory();
                serial.Write(ProtocolEncoder.Encode(snapshot));
                Responses++;
            }
            else if (BoardProtocol.IsAppStop(line))
            {
                Stopped = true;
            }
        }
        if (pending.Length > 1024) pending.Remove(0, pending.Length - 255);
    }
}
