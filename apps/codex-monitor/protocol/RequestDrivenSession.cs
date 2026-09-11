using System;
using System.Text;

public sealed class RequestDrivenSession
{
    private readonly StringBuilder pending = new StringBuilder();

    public event Action RequestReceived;

    public void Consume(string incoming)
    {
        if (string.IsNullOrEmpty(incoming)) return;
        pending.Append(incoming);
        if (pending.Length > 1024)
            pending.Remove(0, pending.Length - 255);

        while (true)
        {
            int newline = pending.ToString().IndexOf('\n');
            if (newline < 0) return;
            string frame = pending.ToString(0, newline + 1);
            pending.Remove(0, newline + 1);
            if (ProtocolParser.IsRequest(frame) && RequestReceived != null)
                RequestReceived();
        }
    }

    public void Reset()
    {
        pending.Length = 0;
    }
}
