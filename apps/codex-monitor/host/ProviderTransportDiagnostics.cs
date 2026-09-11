using System;
using System.Diagnostics;
using System.IO;
using System.Text;

public static class ProviderTransportDiagnostics
{
    private const string InitializeRequest = "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"clientInfo\":{\"name\":\"jz2440-provider-diagnostic\",\"version\":\"0.1.0\"},\"capabilities\":{\"experimentalApi\":true}}}";

    public static int Run()
    {
        string executable = RealCodexProvider.ResolveCodexExecutable();
        Console.WriteLine("PROVIDER_TRACE executable={0}", executable);
        Console.WriteLine("PROVIDER_TRACE arguments=app-server --stdio");
        Console.WriteLine("PROVIDER_TRACE cwd={0}", Environment.CurrentDirectory);
        Console.WriteLine("PROVIDER_TRACE process_architecture={0}", Environment.Is64BitProcess ? "x64" : "x86");
        Console.WriteLine("PROVIDER_TRACE runtime={0}", Environment.Version);
        Console.WriteLine("PROVIDER_TRACE framing=newline-delimited-json; encoding=utf-8-no-bom; stderr=separate-drained-stream");

        ProcessStartInfo info = new ProcessStartInfo
        {
            FileName = executable,
            Arguments = "app-server --stdio",
            WorkingDirectory = Environment.CurrentDirectory,
            UseShellExecute = false,
            CreateNoWindow = true,
            RedirectStandardInput = true,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            StandardOutputEncoding = new UTF8Encoding(false),
            StandardErrorEncoding = new UTF8Encoding(false)
        };

        using (Process process = Process.Start(info))
        {
            if (process == null) throw new InvalidOperationException("unable to start app-server");
            int stderrLines = 0;
            process.ErrorDataReceived += delegate(object sender, DataReceivedEventArgs args)
            {
                if (args.Data != null) stderrLines++;
            };
            process.BeginErrorReadLine();

            byte[] request = Encoding.UTF8.GetBytes(InitializeRequest + "\n");
            process.StandardInput.BaseStream.Write(request, 0, request.Length);
            process.StandardInput.BaseStream.Flush();
            string line = null;
            try
            {
                System.Threading.Tasks.Task<string> read = process.StandardOutput.ReadLineAsync();
                if (!read.Wait(10000))
                {
                    Console.WriteLine("PROVIDER_TRACE handshake=TIMEOUT stdout=none stderr_lines={0}", stderrLines);
                    return 1;
                }
                line = read.Result;
            }
            catch (Exception error)
            {
                Console.WriteLine("PROVIDER_TRACE handshake=READ_ERROR type={0}", error.GetType().FullName);
                return 1;
            }

            if (line == null)
            {
                Console.WriteLine("PROVIDER_TRACE handshake=EOF stderr_lines={0}", stderrLines);
                return 1;
            }
            string normalized = line.TrimStart('\uFEFF');
            bool jsonObject = normalized.StartsWith("{", StringComparison.Ordinal);
            bool responseId1 = normalized.IndexOf("\"id\":1", StringComparison.Ordinal) >= 0;
            Console.WriteLine("PROVIDER_TRACE stdout_frame=1 byte_length={0} json_object={1} response_id_1={2} stderr_lines={3}",
                Encoding.UTF8.GetByteCount(line), jsonObject ? "yes" : "no", responseId1 ? "yes" : "no", stderrLines);
            Console.WriteLine("PROVIDER_TRACE handshake={0}", jsonObject && responseId1 ? "PASS" : "FAIL");
            try { if (!process.HasExited) process.Kill(); } catch (Exception) { }
            return jsonObject && responseId1 ? 0 : 1;
        }
    }
}
