using System;
using System.Collections.Generic;
using System.Text;

public static class ProviderTransportContractTests
{
    public static int Run()
    {
        ProviderLineBuffer buffer = new ProviderLineBuffer();
        if (buffer.Consume("{\"id\":1").Count != 0)
            throw new InvalidOperationException("partial JSON was emitted too early");
        IList<string> frames = buffer.Consume(",\"result\":{}}\n{\"id\":2}\n");
        if (frames.Count != 2 || !ProviderFrameInspector.IsResponseId(frames[0], 1) ||
            !ProviderFrameInspector.IsResponseId(frames[1], 2))
            throw new InvalidOperationException("multiple newline-delimited frames were not preserved");

        string bomFrame = "\uFEFF{\"id\":1,\"result\":{}}";
        if (!ProviderFrameInspector.IsJsonObject(bomFrame) ||
            !ProviderFrameInspector.IsResponseId(bomFrame, 1) ||
            ProviderFrameInspector.Utf8Length(bomFrame) != Encoding.UTF8.GetByteCount(bomFrame))
            throw new InvalidOperationException("UTF-8/BOM frame inspection failed");
        if (ProviderFrameInspector.IsJsonObject("not-json") ||
            ProviderFrameInspector.IsResponseId("{\"id\":2}", 1))
            throw new InvalidOperationException("malformed or mismatched frame was accepted");

        // stderr is intentionally a separate stream in RealCodexProvider;
        // this test ensures the line inspector has no path for merged logs.
        if (ProviderFrameInspector.IsJsonObject("stderr: warning"))
            throw new InvalidOperationException("stderr text was classified as JSON");
        Console.WriteLine("provider transport contract tests: PASS");
        return 0;
    }
}

public sealed class ProviderLineBuffer
{
    private readonly StringBuilder pending = new StringBuilder();

    public IList<string> Consume(string chunk)
    {
        List<string> result = new List<string>();
        if (!String.IsNullOrEmpty(chunk)) pending.Append(chunk);
        while (true)
        {
            int newline = pending.ToString().IndexOf('\n');
            if (newline < 0) break;
            result.Add(pending.ToString(0, newline).TrimEnd('\r'));
            pending.Remove(0, newline + 1);
        }
        return result;
    }
}

public static class ProviderFrameInspector
{
    public static string Normalize(string line)
    {
        return line == null ? null : line.TrimStart('\uFEFF');
    }

    public static bool IsJsonObject(string line)
    {
        string normalized = Normalize(line);
        return normalized != null && normalized.StartsWith("{", StringComparison.Ordinal) &&
            normalized.EndsWith("}", StringComparison.Ordinal);
    }

    public static bool IsResponseId(string line, int id)
    {
        string normalized = Normalize(line);
        return normalized != null && normalized.IndexOf("\"id\":" + id.ToString(), StringComparison.Ordinal) >= 0;
    }

    public static int Utf8Length(string line)
    {
        return line == null ? 0 : Encoding.UTF8.GetByteCount(line);
    }
}
