#pragma once

enum class Kamstrup {
  List3PhaseShort = 0x19,
  List3PhaseLong  = 0x23,
  List1PhaseShort = 0x11,
  List1PhaseLong  = 0x1B
};

enum class Kamstrup_ListCommon {
  ListSize,
  ListVersionIdentifier,
  MeterID_OBIS,
  MeterID,
  MeterType_OBIS,
  MeterType,
  ActiveImportPower_OBIS,
  ActiveImportPower,
  ActiveExportPower_OBIS,
  ActiveExportPower,
  ReactiveImportPower_OBIS,
  ReactiveImportPower,
  ReactiveExportPower_OBIS,
  ReactiveExportPower,
  EndOfList
};

enum class Kamstrup_List3Phase {
  CurrentL1_OBIS = Kamstrup_ListCommon::EndOfList, 
  CurrentL1,
  CurrentL2_OBIS,
  CurrentL2,
  CurrentL3_OBIS,
  CurrentL3,
  VoltageL1_OBIS,
  VoltageL1,
  VoltageL2_OBIS,
  VoltageL2,
  VoltageL3_OBIS,
  VoltageL3,
  MeterClock_OBIS,
  MeterClock,
  EndOfList
};

enum class Kamstrup_List1Phase {
  CurrentL1_OBIS = Kamstrup_ListCommon::EndOfList, 
  CurrentL1,
  VoltageL1_OBIS,
  VoltageL1,
  MeterClock_OBIS,
  MeterClock,
  EndOfList
};

enum class Kamstrup_ListCumulative {
  ActiveImportEnergy_OBIS,
  ActiveImportEnergy,
  ActiveExportEnergy_OBIS,
  ActiveExportEnergy,
  ReactiveImportEnergy_OBIS,
  ReactiveImportEnergy,
  ReactiveExportEnergy_OBIS,
  ReactiveExportEnergy
};
