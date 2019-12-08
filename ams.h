#include "esphome.h"

#include "dlms-decoder.h"
#include "han-decoder.h"
#include "Aidon.h"
#include "Kaifa.h"
#include "Kamstrup.h"

#define get_ams_component(constructor) static_cast<AMS *> (const_cast<custom_component::CustomComponentConstructor *>(&constructor)->get_component(0))

class AMSSensor : public Sensor {
 public:
  AMSSensor(float _scale = 1.0) : Sensor(), scale(_scale) { };
  void publish(float value) {
    publish_state(value * scale);
    yield();
  };
  
 private:
  float scale;
};

class AMSTextSensor : public TextSensor {
 public:
  AMSTextSensor() : TextSensor() { };
  void publish(std::string text) {
    publish_state(text);
    yield();
  };
};


class AMS : public Component, public UARTDevice {
 public:
  AMS(UARTComponent *parent) : Component(), UARTDevice(parent) { };
  void setup() override { };
    
  void loop() override {
    while (available()) {
      uint8_t c = read();

      yield();
      if (dlms.decode(c) && han.init(dlms.get_data()))
        update();
    }
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
  
  AMSSensor active_power{0.001};
  AMSSensor reactive_power{0.001};
  AMSSensor current_L1{0.1};
  AMSSensor current_L2{0.1};
  AMSSensor current_L3{0.1};
  AMSSensor voltage_L1{0.1};
  AMSSensor voltage_L2{0.1};
  AMSSensor voltage_L3{0.1};
  AMSSensor active_import_energy{0.01};
  AMSSensor active_export_energy{0.01};
  AMSSensor reactive_import_energy{0.01};
  AMSSensor reactive_export_energy{0.01};
  AMSTextSensor list_version_id;
  AMSTextSensor meter_id;
  AMSTextSensor meter_type;

  void update() {
    if (han.is_list_version_id("Kamstrup_V0001"))
      update_kamstrup();
    else if (han.is_list_version_id("AIDON_V0001"))
      update_aidon();
    else
      ESP_LOGW("ams", "Unknown list version ID: %s", han.get_list_version_id().c_str());
  };

  void update_aidon() {
    int listSize = han.get_list_size();
    switch (listSize) {
      case (int)Aidon::List1:
        active_power.publish(han.get_int((int)Aidon_List1::ActiveImportPower));
        return;
      case (int)Aidon::List1PhaseLong:
	// TODO 
      case (int)Aidon::List1PhaseShort:
        current_L1.publish(han.get_int((int)Aidon_List1Phase::CurrentL1));
        voltage_L1.publish(han.get_int((int)Aidon_List1Phase::VoltageL1));
        break;
      case (int)Aidon::List3PhaseLong:
        active_import_energy.publish(han.get_int((int)Aidon_List3Phase::CumulativeActiveImportEnergy));
        active_export_energy.publish(han.get_int((int)Aidon_List3Phase::CumulativeActiveExportEnergy));
        reactive_import_energy.publish(han.get_int((int)Aidon_List3Phase::CumulativeReactiveImportEnergy));
        reactive_export_energy.publish(han.get_int((int)Aidon_List3Phase::CumulativeReactiveExportEnergy));
      case (int)Aidon::List3PhaseShort:
        current_L1.publish(han.get_int((int)Aidon_List3Phase::CurrentL1));
        current_L2.publish(han.get_int((int)Aidon_List3Phase::CurrentL2));
        current_L3.publish(han.get_int((int)Aidon_List3Phase::CurrentL3));
        voltage_L1.publish(han.get_int((int)Aidon_List3Phase::VoltageL1));
        voltage_L2.publish(han.get_int((int)Aidon_List3Phase::VoltageL2));
        voltage_L3.publish(han.get_int((int)Aidon_List3Phase::VoltageL3));
        break;
      default:
        ESP_LOGW("ams", "Warning: Unknown listSize %d", listSize);
        return;
    }

    active_power.publish(han.get_int((int)Aidon_ListCommon::ActiveImportPower));
    reactive_power.publish(han.get_int((int)Aidon_ListCommon::ReactiveImportPower));    
    list_version_id.publish(han.get_string((int)Aidon_ListCommon::ListVersionIdentifier));
    meter_id.publish(han.get_string((int)Aidon_ListCommon::MeterID));
    meter_type.publish(han.get_string((int)Aidon_ListCommon::MeterType));
  };
  
  void update_kaifa() {
    int listSize = han.get_list_size();
    switch (listSize) {
      case (int)Kaifa::List1:
        active_power.publish(han.get_int((int)Kaifa_List1::ActiveImportPower));
        return;
      case (int)Kaifa::List1PhaseShort:
      case (int)Kaifa::List1PhaseLong:
        current_L1.publish(han.get_int((int)Kaifa_List1Phase::CurrentL1));
        voltage_L1.publish(han.get_int((int)Kaifa_List1Phase::VoltageL1));
        break;
      case (int)Kaifa::List3PhaseShort:
      case (int)Kaifa::List3PhaseLong:
        current_L1.publish(han.get_int((int)Kaifa_List3Phase::CurrentL1));
        current_L2.publish(han.get_int((int)Kaifa_List3Phase::CurrentL2));
        current_L3.publish(han.get_int((int)Kaifa_List3Phase::CurrentL3));
        voltage_L1.publish(han.get_int((int)Kaifa_List3Phase::VoltageL1));
        voltage_L2.publish(han.get_int((int)Kaifa_List3Phase::VoltageL2));
        voltage_L3.publish(han.get_int((int)Kaifa_List3Phase::VoltageL3));
        break;
      default:
        ESP_LOGW("ams", "Warning: Unknown listSize %d", listSize);
        return;
    }

    active_power.publish(han.get_int((int)Kaifa_ListCommon::ActiveImportPower));
    reactive_power.publish(han.get_int((int)Kaifa_ListCommon::ReactiveImportPower));    
    list_version_id.publish(han.get_string((int)Kaifa_ListCommon::ListVersionIdentifier));
    meter_id.publish(han.get_string((int)Kaifa_ListCommon::MeterID));
    meter_type.publish(han.get_string((int)Kaifa_ListCommon::MeterType));

    int offset;
    switch (listSize) {
      case (int)Kaifa::List1PhaseLong: offset = (int)Kaifa_List1Phase::EndOfList; break;
      case (int)Kaifa::List3PhaseLong: offset = (int)Kaifa_List3Phase::EndOfList; break;
      default: return;
    }

    active_import_energy.publish(han.get_int(offset + (int)Kaifa_ListCumulative::ActiveImportEnergy) /  10.0);
    active_export_energy.publish(han.get_int(offset + (int)Kaifa_ListCumulative::ActiveExportEnergy) / 10.0);
    reactive_import_energy.publish(han.get_int(offset + (int)Kaifa_ListCumulative::ReactiveImportEnergy) / 10.0);
    reactive_export_energy.publish(han.get_int(offset + (int)Kaifa_ListCumulative::ReactiveExportEnergy) / 10.0);
  };

  void update_kamstrup() {
    int listSize = han.get_list_size();
    switch (listSize) {
      case (int)Kamstrup::List1PhaseShort:
      case (int)Kamstrup::List1PhaseLong:
        current_L1.publish(han.get_int((int)Kamstrup_List1Phase::CurrentL1) / 10.0);
        voltage_L1.publish(han.get_int((int)Kamstrup_List1Phase::VoltageL1) * 10);
        break;
      case (int)Kamstrup::List3PhaseShort:
      case (int)Kamstrup::List3PhaseLong:
        current_L1.publish(han.get_int((int)Kamstrup_List3Phase::CurrentL1) / 10.0);
        current_L2.publish(han.get_int((int)Kamstrup_List3Phase::CurrentL2) / 10.0);
        current_L3.publish(han.get_int((int)Kamstrup_List3Phase::CurrentL3) / 10.0);
        voltage_L1.publish(han.get_int((int)Kamstrup_List3Phase::VoltageL1) * 10);
        voltage_L2.publish(han.get_int((int)Kamstrup_List3Phase::VoltageL2) * 10);
        voltage_L3.publish(han.get_int((int)Kamstrup_List3Phase::VoltageL3) * 10);
        break;
      default:
        ESP_LOGW("ams", "Warning: Unknown listSize %d", listSize);
        return;
    }

    active_power.publish(han.get_int((int)Kamstrup_ListCommon::ActiveImportPower));
    reactive_power.publish(han.get_int((int)Kamstrup_ListCommon::ReactiveImportPower));    
    list_version_id.publish(han.get_string((int)Kamstrup_ListCommon::ListVersionIdentifier));
    meter_id.publish(han.get_string((int)Kamstrup_ListCommon::MeterID));
    meter_type.publish(han.get_string((int)Kamstrup_ListCommon::MeterType));

    int offset;
    switch (listSize) {
      case (int)Kamstrup::List1PhaseLong: offset = (int)Kamstrup_List1Phase::EndOfList; break;
      case (int)Kamstrup::List3PhaseLong: offset = (int)Kamstrup_List3Phase::EndOfList; break;
      default: return;
    }

    active_import_energy.publish(han.get_int(offset + (int)Kamstrup_ListCumulative::ActiveImportEnergy));
    active_export_energy.publish(han.get_int(offset + (int)Kamstrup_ListCumulative::ActiveExportEnergy));
    reactive_import_energy.publish(han.get_int(offset + (int)Kamstrup_ListCumulative::ReactiveImportEnergy));
    reactive_export_energy.publish(han.get_int(offset + (int)Kamstrup_ListCumulative::ReactiveExportEnergy));
  };
};


