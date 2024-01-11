// Takes Value and widens it by one bit at the location of the bit
// in the mask.  A one is inserted in the space.  OneBitMask must
// have one and only one bit set.
uint insertonebit(uint value, uint onebitmask)
{
    uint mask = onebitmask - 1;
    return (value & ~mask) << 1 | (value & mask) | onebitmask;
}