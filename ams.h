#include <list>
#include "esphome.h"

#include "dlms-decoder.h"
#include "han-decoder.h"
#include "Aidon.h"
#include "Kaifa.h"
#include "Kamstrup.h"

#define get_ams_component(constructor) static_cast<AMS *> (const_cast<custom_component::CustomComponentConstructor *>(&constructor)->get_component(0))

class AMS;

class AMSSensor : public Sensor {
 public:
  AMSSensor(AMS* _ams, float _scale = 1.0) : Sensor(), ams(_ams), scale(_scale) { };
  void publish(int objectId, float meter_scale = 1.0);
  
 private:
  AMS* ams;
  float scale;
};

class AMSTextSensor : public TextSensor {
 public:
  AMSTextSensor(AMS* _ams) : TextSensor(), ams(_ams) { };
  void publish(int objectId);

 private:
    AMS* ams;
};


class AMS : public Component, public UARTDevice {
 public:
  AMS(UARTComponent *parent) : Component(), UARTDevice(parent) { };
  void setup() override { };
    
  void loop() override {
#ifdef ARDUINO_ARCH_ESP8266
    if (!publish_queue.empty()) {
      publish_queue.front()();
      publish_queue.pop_front();
      return;
    }
#endif

    while (available()) {
      uint8_t c = read();

      if (dlms.decode(c) && han.init(dlms.get_data()))
        update();
    }
  };
  
  int get_int(int objectId) { return han.get_int(objectId); };
  std::string get_string(int objectId) { return han.get_string(objectId); };
  void queue(std::function<void()> fun) {
#ifdef ARDUINO_ARCH_ESP8266
    publish_queue.push_back(fun);
#else
    fun();
#endif
  };

  std::vector<Sensor*> sensors() {
    return {&active_power, &reactive_power,
            &current_L1, &current_L2, &current_L3,
            &voltage_L1, &voltage_L2, &voltage_L3,
            &active_import_energy, &active_export_energy,
            &reactive_import_energy, &reactive_export_energy};
  };
  std::vector<TextSensor*> text_sensors() {
    return {&list_version_id, &meter_id, &meter_type};
  };

 private:
  DlmsDecoder dlms;
  HanDecoder han;

#ifdef ARDUINO_ARCH_ESP8266
  // List used to distribute publishing of sensor values across multiple
  // loop invocations to prevent blocking too long
  std::list<std::function<void()>> publish_queue;
#endif
  
  AMSSensor active_power{this, 0.001};
  AMSSensor reactive_power{this, 0.001};
  AMSSensor current_L1{this, 0.1};
  AMSSensor current_L2{this, 0.1};
  AMSSensor current_L3{this, 0.1};
  AMSSensor voltage_L1{this, 0.1};
  AMSSensor voltage_L2{this, 0.1};
  AMSSensor voltage_L3{this, 0.1};
  AMSSensor active_import_energy{this, 0.01};
  AMSSensor active_export_energy{this, 0.01};
  AMSSensor reactive_import_energy{this, 0.01};
  AMSSensor reactive_export_energy{this, 0.01};
  AMSTextSensor list_version_id{this};
  AMSTextSensor meter_id{this};
  AMSTextSensor meter_type{this};

  void update() {
    if (han.is_list_version_id("Kamstrup_V0001"))
      update_kamstrup();
    else if (han.is_list_version_id("KAIFA_V0001"))
      update_kaifa();
    else if (han.is_list_version_id("AIDON_V0001"))
      update_aidon();
    else
      ESP_LOGW("ams", "Unknown list version ID: %s", han.get_list_version_id().c_str());
  };

  void update_aidon() {
    int listSize = han.get_list_size();
    switch (listSize) {
      case (int)Aidon::List1:
        active_power.publish((int)Aidon_List1::ActiveImportPower);
        return;
      case (int)Aidon::List1PhaseLong:
	// TODO 
      case (int)Aidon::List1PhaseShort:
        current_L1.publish((int)Aidon_List1Phase::CurrentL1);
        voltage_L1.publish((int)Aidon_List1Phase::VoltageL1);
        break;
      case (int)Aidon::List3PhaseLong:
        active_import_energy.publish((int)Aidon_List3Phase::CumulativeActiveImportEnergy);
        active_export_energy.publish((int)Aidon_List3Phase::CumulativeActiveExportEnergy);
        reactive_import_energy.publish((int)Aidon_List3Phase::CumulativeReactiveImportEnergy);
        reactive_export_energy.publish((int)Aidon_List3Phase::CumulativeReactiveExportEnergy);
      case (int)Aidon::List3PhaseShort:
        current_L1.publish((int)Aidon_List3Phase::CurrentL1);
        current_L2.publish((int)Aidon_List3Phase::CurrentL2);
        current_L3.publish((int)Aidon_List3Phase::CurrentL3);
        voltage_L1.publish((int)Aidon_List3Phase::VoltageL1);
        voltage_L2.publish((int)Aidon_List3Phase::VoltageL2);
        voltage_L3.publish((int)Aidon_List3Phase::VoltageL3);
        break;
      default:
        ESP_LOGW("ams", "Warning: Unknown listSize %d", listSize);
        return;
    }

    active_power.publish((int)Aidon_ListCommon::ActiveImportPower);
    reactive_power.publish((int)Aidon_ListCommon::ReactiveImportPower);
    list_version_id.publish((int)Aidon_ListCommon::ListVersionIdentifier);
    meter_id.publish((int)Aidon_ListCommon::MeterID);
    meter_type.publish((int)Aidon_ListCommon::MeterType);
  };
  
  void update_kaifa() {
    int listSize = han.get_list_size();
    switch (listSize) {
      case (int)Kaifa::List1:
        active_power.publish((int)Kaifa_List1::ActiveImportPower);
        return;
      case (int)Kaifa::List1PhaseShort:
      case (int)Kaifa::List1PhaseLong:
        current_L1.publish((int)Kaifa_List1Phase::CurrentL1);
        voltage_L1.publish((int)Kaifa_List1Phase::VoltageL1);
        break;
      case (int)Kaifa::List3PhaseShort:
      case (int)Kaifa::List3PhaseLong:
        current_L1.publish((int)Kaifa_List3Phase::CurrentL1);
        current_L2.publish((int)Kaifa_List3Phase::CurrentL2);
        current_L3.publish((int)Kaifa_List3Phase::CurrentL3);
        voltage_L1.publish((int)Kaifa_List3Phase::VoltageL1);
        voltage_L2.publish((int)Kaifa_List3Phase::VoltageL2);
        voltage_L3.publish((int)Kaifa_List3Phase::VoltageL3);
        break;
      default:
        ESP_LOGW("ams", "Warning: Unknown listSize %d", listSize);
        return;
    }

    active_power.publish((int)Kaifa_ListCommon::ActiveImportPower);
    reactive_power.publish((int)Kaifa_ListCommon::ReactiveImportPower);
    list_version_id.publish((int)Kaifa_ListCommon::ListVersionIdentifier);
    meter_id.publish((int)Kaifa_ListCommon::MeterID);
    meter_type.publish((int)Kaifa_ListCommon::MeterType);

    int offset;
    switch (listSize) {
      case (int)Kaifa::List1PhaseLong: offset = (int)Kaifa_List1Phase::EndOfList; break;
      case (int)Kaifa::List3PhaseLong: offset = (int)Kaifa_List3Phase::EndOfList; break;
      default: return;
    }

    active_import_energy.publish(offset + (int)Kaifa_ListCumulative::ActiveImportEnergy, 0.1);
    active_export_energy.publish(offset + (int)Kaifa_ListCumulative::ActiveExportEnergy, 0.1);
    reactive_import_energy.publish(offset + (int)Kaifa_ListCumulative::ReactiveImportEnergy, 0.1);
    reactive_export_energy.publish(offset + (int)Kaifa_ListCumulative::ReactiveExportEnergy, 0.1);
  };

  void update_kamstrup() {
    int listSize = han.get_list_size();
    switch (listSize) {
      case (int)Kamstrup::List1PhaseShort:
      case (int)Kamstrup::List1PhaseLong:
        current_L1.publish((int)Kamstrup_List1Phase::CurrentL1, 0.1);
        voltage_L1.publish((int)Kamstrup_List1Phase::VoltageL1, 10.0);
        break;
      case (int)Kamstrup::List3PhaseShort:
      case (int)Kamstrup::List3PhaseLong:
        current_L1.publish((int)Kamstrup_List3Phase::CurrentL1, 0.1);
        current_L2.publish((int)Kamstrup_List3Phase::CurrentL2, 0.1);
        current_L3.publish((int)Kamstrup_List3Phase::CurrentL3, 0.1);
        voltage_L1.publish((int)Kamstrup_List3Phase::VoltageL1, 10.0);
        voltage_L2.publish((int)Kamstrup_List3Phase::VoltageL2, 10.0);
        voltage_L3.publish((int)Kamstrup_List3Phase::VoltageL3, 10.0);
        break;
      default:
        ESP_LOGW("ams", "Warning: Unknown listSize %d", listSize);
        return;
    }

    active_power.publish((int)Kamstrup_ListCommon::ActiveImportPower);
    reactive_power.publish((int)Kamstrup_ListCommon::ReactiveImportPower);

    list_version_id.publish((int)Kamstrup_ListCommon::ListVersionIdentifier);
    meter_id.publish((int)Kamstrup_ListCommon::MeterID);
    meter_type.publish((int)Kamstrup_ListCommon::MeterType);

    int offset;
    switch (listSize) {
      case (int)Kamstrup::List1PhaseLong: offset = (int)Kamstrup_List1Phase::EndOfList; break;
      case (int)Kamstrup::List3PhaseLong: offset = (int)Kamstrup_List3Phase::EndOfList; break;
      default: return;
    }

    active_import_energy.publish(offset + (int)Kamstrup_ListCumulative::ActiveImportEnergy);
    active_export_energy.publish(offset + (int)Kamstrup_ListCumulative::ActiveExportEnergy);
    reactive_import_energy.publish(offset + (int)Kamstrup_ListCumulative::ReactiveImportEnergy);
    reactive_export_energy.publish(offset + (int)Kamstrup_ListCumulative::ReactiveExportEnergy);
  };
};


void AMSSensor::publish(int objectId, float meter_scale)
{
  ams->queue([=]() {
    float value = ams->get_int(objectId) * scale * meter_scale;
    publish_state(value);
  });
};

void AMSTextSensor::publish(int objectId) {
  ams->queue([=]() { publish_state(ams->get_string(objectId)); });
};
