#pragma once

#include <vector>
#include "esphome.h"
#include "Aidon.h"
#include "Kaifa.h"
#include "Kamstrup.h"


class HanDecoder
{
 public:
  HanDecoder() { };
  bool init(std::vector<uint8_t> data) {
    message = data;
    
    if (message.size() < 9)
      ESP_LOGD("ams", "Invalid HAN message: Less than 9 bytes received");
    else if (message[0] != 0xE6 || message[1] != 0xE7 || message[2] != 0x00 || message[3] != 0x0F)
      ESP_LOGD("ams", "Invalid HAN message: Start should be E6 E7 00 0F");
    else {
      listSize = get_int(0);
      ESP_LOGD("ams", "Got han HAN message. List size: %d", listSize);

      if (listVersionID.empty()) {
        // Try to autodetect list version identification, only tested with Kamstrup.
        // Id for Aidon and Kaifa taken from https://github.com/roarfred/AmsToMqttBridge/blob/master/Documentation/NVE_Info_kunder_HANgrensesnitt.pdf, but not verified
        compensateFor09HeaderBug = false;
        listVersionID = get_string((int)Kamstrup_ListCommon::ListVersionIdentifier);
        if (!is_list_version_id("Kamstrup_V0001")) {
          listVersionID = get_string((int)Aidon_ListCommon::ListVersionIdentifier);
          if (!is_list_version_id("AIDON_V0001")) {
            compensateFor09HeaderBug = true;
            listVersionID = get_string((int)Kaifa_ListCommon::ListVersionIdentifier);
            if (!is_list_version_id("KAIFA_V0001")) {
              ESP_LOGD("ams", "Unknown list version ID: %s", listVersionID.c_str());
              listVersionID = "";
              return false;
            }
          }
        }
        ESP_LOGD("ams", "Detected list version ID: %s", listVersionID.c_str());
      }
      return true;
    }    
    return false;
  };

  int get_list_size() { return listSize; };
  std::string get_list_version_id() { return listVersionID; };
  bool is_list_version_id(std::string list) { return list == listVersionID; };
  int get_int(int objectId) {
    int valuePosition = find_value_position(objectId);

    if (valuePosition > 0) {
      int value = 0;
      int bytes = 0;
      switch (message[valuePosition++]) {
      case 0x10: bytes = 2; break;
      case 0x12: bytes = 2; break;
      case 0x06: bytes = 4; break;
      case 0x02: bytes = 1; break;
      case 0x01: bytes = 1; break;
      case 0x0F: bytes = 1; break;
      case 0x16: bytes = 1; break;
      default:
        ESP_LOGW("ams", "Object %d (at position %d) not an int", objectId, valuePosition);
      }

      for (int i = valuePosition; i < valuePosition + bytes; i++)
        value = value << 8 | message[i];      
            
      return value;
    } else
      ESP_LOGW("ams", "Object %d not found", objectId);
      return 0;
  }

  std::string get_string(int objectId) {
    int valuePosition = find_value_position(objectId);
    if (valuePosition > 0) {
      size_t size = (size_t)message[valuePosition + 1];
      return std::string((char*)message.data() + valuePosition + 2, size);
    } else
      return std::string();
  }

 private:
  const int dataHeader = 8;
  bool compensateFor09HeaderBug = false;

  std::vector<uint8_t> message;
  int listSize;
  std::string listVersionID;

  int find_value_position(int dataPosition) {
    // The first byte after the header gives the length 
    // of the extended header information (variable)
    int headerSize = dataHeader + (compensateFor09HeaderBug ? 1 : 0);
    int firstData = headerSize + message[headerSize] + 1;

    for (int i = firstData; i < message.size(); i++) {
      if (dataPosition-- == 0) 
        return i;
      else if (message[i] == 0x00) // null
        i += 0;
      else if (message[i] == 0x0A) // String
        i += message[i + 1] + 1;
      else if (message[i] == 0x09) // byte array
        i += message[i + 1] + 1;
      else if (message[i] == 0x01) // array (1 byte for reading size)
        i += 1;
      else if (message[i] == 0x02) // struct (1 byte for reading size)
        i += 1;
      else if (message[i] == 0x10) // int16 value (2 bytes)
        i += 2;
      else if (message[i] == 0x12) // uint16 value (2 bytes)
        i += 2;
      else if (message[i] == 0x06) // uint32 value (4 bytes)
        i += 4;
      else if (message[i] == 0x0F) // int8 value (1 bytes)
        i += 1;
      else if (message[i] == 0x16) // enum (1 bytes)
        i += 1;
      else {
        ESP_LOGD("ams", "Unknown data type found: 0x%x", message[i]);
        return 0; // unknown data type found
      }
    }
    
    ESP_LOGD("asm", "Passed the end of the data. Length was: %d", message.size());
    return 0;
  }
};
