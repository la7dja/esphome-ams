#pragma once

#include <vector>
#include "crc16.h" 

#define DLMS_READER_BUFFER_SIZE 512
#define DLMS_READER_MAX_ADDRESS_SIZE 5

class DlmsDecoder
{
 public:
  DlmsDecoder() { };
  bool decode(uint8_t data) {
    // We have completed reading of one package, so clear and be ready for the next
    if (dataLength > 0 && position >= dataLength + 2) clear();
        
    // if we haven't started yet, wait for the start flag (no need to capture any data yet)
    if (position == 0 && data != 0x7E) return false;

    // Check if we're about to run into a buffer overflow
    if (position >= DLMS_READER_BUFFER_SIZE) clear();

    // Check if this is a second start flag, which indicates the previous one was a stop from the last package
    if (position == 1 && data == 0x7E) return false; // just return, we can keep the one byte we had in the buffer
    
    // We have started, so capture every byte
    buffer[position++] = data;

    if (position == 1) return false; // This was the start flag, we're not done yet

    if (position == 2) {
      // Capture the Frame Format Type
      frameFormatType = (uint8_t)(data & 0xF0);
      if (!is_valid_frame_format(frameFormatType)) clear();
      return false;
    }
    
    if (position == 3) {
      // Capture the length of the data package
      dataLength = ((buffer[1] & 0x0F) << 8) | buffer[2];
      return false;
    }

    if (destinationAddressLength == 0) {
      // Capture the destination address
      destinationAddressLength = get_address(3, destinationAddress, 0);
      if (destinationAddressLength > 3) clear();
      return false;
    }
    
    if (sourceAddressLength == 0) {
      // Capture the source address
      sourceAddressLength = get_address(3 + destinationAddressLength, sourceAddress, 0);
      if (sourceAddressLength > 3) clear();
      return false;
    }
     
    if (position == 4 + destinationAddressLength + sourceAddressLength + 2) {
      // Verify the header checksum
      uint16_t headerChecksum = get_checksum(position - 3);
      if (headerChecksum != crc16.compute_checksum(buffer, 1, position - 3)) clear();
      return false;
    }

    if (position == dataLength + 1) {
      // Verify the data package checksum
      uint16_t checksum = this->get_checksum(position - 3);
      if (checksum != crc16.compute_checksum(buffer, 1, position - 3)) clear();
      return false;
    }
    
    if (position == dataLength + 2) {
      // We're done, check the stop flag and signal we're done
      if (data == 0x7E)
        return true;
      else {
        clear();
        return false;
      }
    }
    
    return false;
  };

  std::vector<uint8_t> get_data() {
    int headerLength = 3 + destinationAddressLength + sourceAddressLength + 2;
    uint8_t *start = buffer + 1 + headerLength;
    return {start, start + dataLength};
  }
    
 protected:
  CRC16 crc16;
    
 private:
  uint8_t buffer[DLMS_READER_BUFFER_SIZE];
  int position;
  int dataLength;
  uint8_t frameFormatType;
  uint8_t destinationAddress[DLMS_READER_MAX_ADDRESS_SIZE];
  uint8_t destinationAddressLength;
  uint8_t sourceAddress[DLMS_READER_MAX_ADDRESS_SIZE];
  uint8_t sourceAddressLength;
  
  void clear() {
    position = 0;
    dataLength = 0;
    destinationAddressLength = 0;
    sourceAddressLength = 0;
    frameFormatType = 0;
  };

  int get_address(int addressPosition, uint8_t* addressBuffer, int start) {
    int addressBufferPos = start;
    for (int i = addressPosition; i < position; i++) {
      addressBuffer[addressBufferPos++] = buffer[i];
        
      // LSB=1 means this was the last address byte
      if ((buffer[i] & 0x01) == 0x01) break;

      // See if we've reached last byte, try again when we've got more data
      else if (i == position - 1) return 0;
    }
    return addressBufferPos - start;
  };
  uint16_t get_checksum(int checksumPosition) {
    return (uint16_t)(buffer[checksumPosition + 2] << 8 | buffer[checksumPosition + 1]);
  };
  bool is_valid_frame_format(uint8_t frameFormatType) { return frameFormatType == 0xA0; };
};
