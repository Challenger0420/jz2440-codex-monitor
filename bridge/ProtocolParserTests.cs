using System;
using System.Collections.Generic;

public static class ProtocolParserTests
{
    public static int Run()
    {
        string valid = "<CQM1|5H=72|5HR=1788945780|W=64|WR=1789138800|RC=3|NOW=1788938123|TZ=480>\n";
        string[] accepted =
        {
            valid,
            "<CQM1|TZ=-480|W=64|UNKNOWN=x|WR=1789138800|5H=72|NOW=1788938123|RC=-1|5HR=1788945780>\n",
            "<CQM1|5H=72|5HR=1788945780|W=64|WR=1789138800|NOW=1788938123|TZ=480>\n",
            "<CQM1|5H=0|5HR=0|W=100|WR=4294967295|NOW=0|TZ=0|RC=-1>\n"
        };
        string[] rejected =
        {
            "<CQM1|5H=72|W=64|WR=1|NOW=1|TZ=480>\n",
            "<CQM1|5H=x|5HR=1|W=64|WR=1|NOW=1|TZ=480>\n",
            "<CQM1|5H=101|5HR=1|W=64|WR=1|NOW=1|TZ=480>\n",
            "<CQM1|5H=-1|5HR=1|W=64|WR=1|NOW=1|TZ=480>\n",
            "<CQM1|5H=72|5HR=1|W=64|WR=1|NOW=1|TZ=480",
            "<CQM1|5H=72|5HR=1|W=64|WR=1|NOW=1|TZ=480|" + new string('x', 260) + ">\n",
            "<CQM1|5H=72|5HR=1|W=64|WR=1|NOW=1|TZ=1500>\n",
            "<CQM1|5H=72|5HR=4294967296|W=64|WR=1|NOW=1|TZ=480>\n",
            "<CQM1|5H=72|5HR=1|W=64|WR=1|NOW=1|TZ=480|RC=-2>\n",
            "<CQM1|5H=72|5HR=1|W=64|WR=1|NOW=1|TZ=480|5H=73>\n",
            "<CQM2|5H=72|5HR=1|W=64|WR=1|NOW=1|TZ=480>\n",
            "garbage<CQM1|5H=72|5HR=1|W=64|WR=1|NOW=1|TZ=480>\n"
        };
        int passed = 0;
        for (int i = 0; i < accepted.Length; i++)
        {
            Dictionary<string, string> fields;
            bool okay = ProtocolParser.TryParse(accepted[i], out fields);
            if (!okay) throw new InvalidOperationException("accepted case failed: " + i);
            passed++;
        }
        for (int i = 0; i < rejected.Length; i++)
        {
            Dictionary<string, string> fields;
            if (ProtocolParser.TryParse(rejected[i], out fields))
                throw new InvalidOperationException("rejected case accepted: " + i);
            passed++;
        }
        if (!ProtocolParser.IsQuit("<CQMQUIT>\n") || !ProtocolParser.IsQuit("<Quit>\n") ||
            ProtocolParser.IsQuit("<CQMQUIT>"))
            throw new InvalidOperationException("quit frame test failed");
        IList<string> extracted = ProtocolParser.ExtractFrames("noise\n" + valid + "junk\n<CQMQUIT>\n" + valid);
        if (extracted.Count != 3 || extracted[0] != valid || extracted[1] != "<CQMQUIT>\n")
            throw new InvalidOperationException("stream extraction test failed");
        passed += 4;
        Console.WriteLine("protocol parser self-test: PASS ({0} cases)", passed);
        return 0;
    }
}
