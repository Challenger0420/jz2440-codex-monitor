using System;
using System.Text;

public sealed class SerialTraceTransport : ISerialTransport
{
    private readonly ISerialTransport inner;
    private readonly Func<string> mode;

    public SerialTraceTransport(ISerialTransport transport, Func<string> modeProvider)
    {
        inner = transport;
        mode = modeProvider;
    }

    public string PortName { get { return inner.PortName; } }

    public void Write(string frame)
    {
        Console.WriteLine("[{0}] TX len={1}",
            DateTime.Now.ToString("HH:mm:ss.fff"),
            Encoding.UTF8.GetByteCount(frame == null ? "" : frame));
        Console.WriteLine("  ascii=\"{0}\"", Escape(frame == null ? "" : frame));
        Console.WriteLine("  hex={0}", Hex(frame == null ? "" : frame));
        inner.Write(frame);
    }

    public string ReadAvailable()
    {
        string chunk = inner.ReadAvailable();
        if (!String.IsNullOrEmpty(chunk))
        {
            Console.WriteLine("[{0}] RX mode={1} len={2}",
                DateTime.Now.ToString("HH:mm:ss.fff"),
                mode == null ? "Unknown" : mode(),
                Encoding.UTF8.GetByteCount(chunk));
            Console.WriteLine("  ascii=\"{0}\"", Escape(chunk));
            Console.WriteLine("  hex={0}", Hex(chunk));
        }
        return chunk;
    }

    public void Dispose()
    {
        inner.Dispose();
    }

    private static string Escape(string value)
    {
        StringBuilder result = new StringBuilder();
        foreach (char c in value)
        {
            if (c == '\\') result.Append("\\\\");
            else if (c == '\r') result.Append("\\r");
            else if (c == '\n') result.Append("\\n");
            else if (c == '\t') result.Append("\\t");
            else if (c >= 32 && c <= 126) result.Append(c);
            else result.Append("\\x").Append(((int)c & 0xff).ToString("X2"));
        }
        return result.ToString();
    }

    private static string Hex(string value)
    {
        byte[] bytes = Encoding.UTF8.GetBytes(value);
        StringBuilder result = new StringBuilder(bytes.Length * 3);
        for (int i = 0; i < bytes.Length; i++)
        {
            if (i != 0) result.Append(' ');
            result.Append(bytes[i].ToString("X2"));
        }
        return result.ToString();
    }
}
