#pragma once

#include <cstdint>

class CRC16
{
 public:
  CRC16() {
    uint16_t value;
    uint16_t temp;
    for (int i = 0; i < 256; ++i) {
      value = 0;
      temp = i;
      for (int j = 0; j < 8; ++j) {
        if (((value ^ temp) & 0x0001) != 0)
          value = (uint16_t)((value >> 1) ^ polynomial);
        else
          value >>= 1;
        temp >>= 1;
      }
      table[i] = value;
    }
  };

  uint16_t compute_checksum(uint8_t *data, int start, int length) {
    uint16_t fcs = 0xffff;
    for (int i = start; i < (start + length); i++)
    {
        int index = (fcs ^ data[i]) & 0xff;
        fcs = (uint16_t)((fcs >> 8) ^ table[index]);
    }
    fcs ^= 0xffff;
    return fcs;
}

  private:
    const uint16_t polynomial = 0x8408;
    uint16_t table[256];
};


