using System;

public static class RequestDrivenSessionTests
{
    public static int Run()
    {
        int requests = 0;
        RequestDrivenSession session = new RequestDrivenSession();
        session.RequestReceived += delegate { requests++; };

        session.Consume("noise\n");
        if (requests != 0)
            throw new InvalidOperationException("bridge sent a response without CQMREQ");
        session.Consume("<CQMREQ|V=1>\n");
        if (requests != 1)
            throw new InvalidOperationException("single CQMREQ was not handled once");
        session.Consume("<CQMREQ|V=1>\n<CQMREQ|V=1>\n");
        if (requests != 3)
            throw new InvalidOperationException("repeated CQMREQ handling failed");

        session.Reset();
        session.Consume("<CQMREQ|");
        if (requests != 3)
            throw new InvalidOperationException("partial request was handled too early");
        session.Consume("V=1>\n");
        if (requests != 4)
            throw new InvalidOperationException("split CQMREQ was not reassembled");
        Console.WriteLine("request-driven session self-test: PASS");
        return 0;
    }
}
