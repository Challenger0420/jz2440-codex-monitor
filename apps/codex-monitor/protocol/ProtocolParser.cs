using System;
using System.Collections.Generic;

public static class ProtocolParser
{
    public static bool IsQuit(string frame)
    {
        return frame == "<CQMQUIT>\n" || frame == "<Quit>\n";
    }

    public static bool IsRequest(string frame)
    {
        return frame == "<CQMREQ|V=1>\n";
    }

    public static IList<string> ExtractFrames(string stream)
    {
        List<string> frames = new List<string>();
        if (stream == null) return frames;
        string[] lines = stream.Split(new[] { '\n' }, StringSplitOptions.None);
        foreach (string line in lines)
        {
            string candidate = line + "\n";
            Dictionary<string, string> fields;
            if (TryParse(candidate, out fields) || IsQuit(candidate) || IsRequest(candidate))
                frames.Add(candidate);
        }
        return frames;
    }

    public static bool TryParse(string frame, out Dictionary<string, string> fields)
    {
        fields = new Dictionary<string, string>(StringComparer.Ordinal);
        if (frame == null || frame.Length > 255 || !frame.EndsWith("\n", StringComparison.Ordinal))
            return false;
        string body = frame.Substring(0, frame.Length - 1);
        if (body.Length < 7 || body[0] != '<' || body[body.Length - 1] != '>')
            return false;
        string[] tokens = body.Substring(1, body.Length - 2).Split('|');
        if (tokens.Length == 0 || tokens[0] != "CQM1")
            return false;
        for (int i = 1; i < tokens.Length; i++)
        {
            int equals = tokens[i].IndexOf('=');
            if (equals <= 0 || equals == tokens[i].Length - 1)
                return false;
            if (fields.ContainsKey(tokens[i].Substring(0, equals)))
                return false;
            fields[tokens[i].Substring(0, equals)] = tokens[i].Substring(equals + 1);
        }
        int fiveHour;
        int weekly;
        int resetCards;
        long ignored;
        if (!TryInt(fields, "5H", out fiveHour) || fiveHour < 0 || fiveHour > 100)
            return false;
        if (!TryInt(fields, "W", out weekly) || weekly < 0 || weekly > 100)
            return false;
        if (!TryLong(fields, "5HR", out ignored) || ignored < 0 || ignored > uint.MaxValue)
            return false;
        if (!TryLong(fields, "WR", out ignored) || ignored < 0 || ignored > uint.MaxValue)
            return false;
        if (!TryLong(fields, "NOW", out ignored) || ignored < 0 || ignored > uint.MaxValue)
            return false;
        if (!TryInt(fields, "TZ", out ignored) || ignored < -1440 || ignored > 1440)
            return false;
        if (fields.ContainsKey("RC") && (!TryInt(fields, "RC", out resetCards) || resetCards < -1))
            return false;
        return true;
    }

    private static bool TryInt(Dictionary<string, string> fields, string key, out int value)
    {
        string text;
        if (!fields.TryGetValue(key, out text)) { value = 0; return false; }
        return int.TryParse(text, out value);
    }

    private static bool TryInt(Dictionary<string, string> fields, string key, out long value)
    {
        string text;
        if (!fields.TryGetValue(key, out text)) { value = 0; return false; }
        int parsed;
        if (!int.TryParse(text, out parsed)) { value = 0; return false; }
        value = parsed;
        return true;
    }

    private static bool TryLong(Dictionary<string, string> fields, string key, out long value)
    {
        string text;
        if (!fields.TryGetValue(key, out text)) { value = 0; return false; }
        return long.TryParse(text, out value);
    }
}
