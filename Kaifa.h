#pragma once

enum class Kaifa {
  List1           = 0x01,
  List1PhaseShort = 0x09,
  List3PhaseShort = 0x0D,
  List1PhaseLong = 0x0E,
  List3PhaseLong = 0x12
};

enum class Kaifa_List1 {
  ListSize,
  ActiveImportPower
};

enum class Kaifa_ListCommon {
  ListSize,
  ListVersionIdentifier,
  MeterID,
  MeterType,
  ActiveImportPower,
  ActiveExportPower,
  ReactiveImportPower,
  ReactiveExportPower,
  EndOfList
};

enum class Kaifa_List3Phase {
  CurrentL1 = Kaifa_ListCommon::EndOfList,
  CurrentL2,
  CurrentL3,
  VoltageL1,
  VoltageL2,
  VoltageL3,
  MeterClock,
  EndOfList
};

enum class Kaifa_List1Phase {
  CurrentL1 = Kaifa_ListCommon::EndOfList,
  VoltageL1,
  MeterClock,
  EndOfList
};

enum class Kaifa_ListCumulative {
  ActiveImportEnergy,
  ActiveExportEnergy,
  ReactiveImportEnergy,
  ReactiveExportEnergy
};
