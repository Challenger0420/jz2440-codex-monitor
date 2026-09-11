using System;

public static class BoardProtocol
{
    public static bool IsAppReady(string line)
    {
        return line != null && line.TrimEnd('\r', '\n') == "<APPREADY|codex-monitor>";
    }

    public static bool IsAppStop(string line)
    {
        const string prefix = "<APPSTOP|codex-monitor|RC=";
        if (line == null) return false;
        string normalized = line.TrimEnd('\r', '\n');
        if (!normalized.StartsWith(prefix, StringComparison.Ordinal) ||
            !normalized.EndsWith(">", StringComparison.Ordinal)) return false;
        string rc = normalized.Substring(prefix.Length, normalized.Length - prefix.Length - 1);
        int value;
        return int.TryParse(rc, out value);
    }

    public static bool IsLifecycleLine(string line)
    {
        return IsAppReady(line) || IsAppStop(line);
    }
}
