using System;
using System.IO.Ports;
using System.Threading;

var package = new HapticGlovePackage();
package.MotorData.Add(new HapticGloveMotorData() { MotorIndex = 1, Strength = 100, Duration = 2000 });
package.MotorData.Add(new HapticGloveMotorData() { MotorIndex = 0, Strength = 125, Duration = 2000 });

using (var port = new SerialPort("COM5", 9600) {  RtsEnable = true, DtrEnable = true, NewLine = "\n" })
{
    try
    {
        port.Open();
    }
    catch(Exception ex)
    {
        Console.WriteLine($"Unable to open port {port.PortName}: {ex}");
        return;
    }
    Console.Write("Port is open. Writing data...");

    var data = package.Serialize();
    port.Write(data, 0, data.Length);

    Console.WriteLine(" Done.");

    Console.Write("Waiting for response...");
    var response = port.ReadLine();
    Console.WriteLine(" Done.");

    while (true)
    {
        Console.WriteLine(response);

        response = port.ReadLine();
        Thread.Sleep(0);
    }
}