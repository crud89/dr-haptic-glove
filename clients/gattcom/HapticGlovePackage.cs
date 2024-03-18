using System;
using System.Runtime.InteropServices;

struct HapticGloveMotorData
{
    public Byte MotorIndex;
    public Byte Strength;
    public UInt16 Duration;
}

class HapticGlovePackage
{
    public List<HapticGloveMotorData> MotorData { get; private set; } = new List<HapticGloveMotorData>();

    public unsafe Byte[] Serialize()
    {
        var data = new Byte[4 + 4 * MotorData.Count];

        fixed (Byte* ptr = &data[0])
            *(UInt32*)ptr = Convert.ToUInt32(MotorData.Count);

        for (int i = 0; i < MotorData.Count; i++)
        {
            fixed (Byte* ptr = &data[4 + i * 4])
            {
                *(Byte*)(ptr + 3) = this.MotorData[i].MotorIndex;
                *(Byte*)(ptr + 2) = this.MotorData[i].Strength;
                *(UInt16*)(ptr + 0) = this.MotorData[i].Duration;
            }
        }

        return data;
    }
}