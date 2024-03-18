using System;
using System.Threading.Tasks;
using InTheHand.Bluetooth;
using Windows.Devices.Input;
using Windows.Security.Authentication.OnlineId;

var package = new HapticGlovePackage();
package.MotorData.Add(new HapticGloveMotorData());
package.MotorData.Add(new HapticGloveMotorData());

Console.Write("Looking for devices");
BluetoothDevice? gloveDevice = null;
var requestOptions = new RequestDeviceOptions();
requestOptions.Filters.Add(new BluetoothLEScanFilter() { NamePrefix = "TUC" });

for (int attempt = 0; attempt < 3; ++attempt)
{
    Console.Write(".");
    var devices = await Bluetooth.ScanForDevicesAsync(requestOptions);

    if (devices.Count > 0)
    {
        foreach (var device in devices)
        {
            if (device.Name == "TUC Haptic Glove")
            {
                gloveDevice = device;
                break;
            }
        }

        if (gloveDevice != null)
        {
            Console.WriteLine(" Done.");
            break;
        }
    }
}

if (gloveDevice == null)
{
    Console.WriteLine(" Failed.");
    Console.WriteLine("No glove found.");
    return;
}

Console.Write($"Found glove with ID: {gloveDevice.Id}. Connecting...");
await gloveDevice.Gatt.ConnectAsync();
Console.WriteLine(" Done.");

try
{
    var services = await gloveDevice.Gatt.GetPrimaryServicesAsync();

    if (services.Count == 0)
    {
        Console.WriteLine("No services found on device.");
        return;
    }
    else
    {
        foreach (var svc in services)
            Console.WriteLine($"Service: {svc.Uuid}");
    }

    Console.Write("Establishing service connection...");
    var service = await gloveDevice.Gatt.GetPrimaryServiceAsync(BluetoothUuid.FromGuid(new Guid("141806C2-081D-4197-FFFF-98D46AC994BE")));
    var characteristic = await service.GetCharacteristicAsync(BluetoothUuid.FromGuid(new Guid("141806C2-081D-4197-0001-98D46AC994BE")));
    Console.WriteLine(" Done.");

    var rng = new Random();

    while (!Console.KeyAvailable || Console.ReadKey(true).Key != ConsoleKey.Escape)
    {
        // Update packages.
        package.MotorData[0] = new HapticGloveMotorData() { MotorIndex = 1, Strength = Convert.ToByte(rng.Next(0, 255)), Duration = Convert.ToUInt16(rng.Next(0, 3000)) };
        package.MotorData[1] = new HapticGloveMotorData() { MotorIndex = 0, Strength = Convert.ToByte(rng.Next(0, 255)), Duration = Convert.ToUInt16(rng.Next(0, 3000)) };

        // Set updates.
        Console.Write("Sending package...");
        await characteristic.WriteValueWithResponseAsync(package.Serialize());
        Console.WriteLine(" Done.");

        await Task.Delay(3000);
    }
}
catch(Exception ex)
{
    Console.WriteLine($"Unable to write to package stream: {ex}");
}
finally
{
    gloveDevice.Gatt.Disconnect();
}
