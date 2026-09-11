using System;
using System.IO.Ports;

public interface ISerialTransport : IDisposable
{
    string PortName { get; }
    void Write(string frame);
    string ReadAvailable();
}

public sealed class SerialTransport : ISerialTransport
{
    private readonly SerialPort serial;

    private SerialTransport(SerialPort serialPort)
    {
        serial = serialPort;
    }

    public string PortName { get { return serial.PortName; } }

    public static SerialTransport Open(string portName)
    {
        SerialPort serial = new SerialPort(portName, 115200, Parity.None, 8, StopBits.One)
        {
            Handshake = Handshake.None,
            DtrEnable = false,
            RtsEnable = false,
            ReadTimeout = 500,
            WriteTimeout = 2000,
            NewLine = "\n"
        };
        try
        {
            serial.Open();
            return new SerialTransport(serial);
        }
        catch
        {
            serial.Dispose();
            throw;
        }
    }

    public void Write(string frame)
    {
        serial.Write(frame);
    }

    public string ReadAvailable()
    {
        return serial.ReadExisting();
    }

    public void Dispose()
    {
        serial.Dispose();
    }
}
