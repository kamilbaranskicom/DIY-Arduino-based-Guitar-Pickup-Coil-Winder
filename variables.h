// --- STRUCTURES & ENUMS ---

enum VarType { T_INT, T_FLOAT, T_LONG, T_BOOL };
enum VarCategory { C_MACHINE, C_PRESET }; // Distinction for EEPROM logic

struct VarMap {
  const char *label;
  void *ptr;
  VarType type;
  VarCategory category;
};

// --- VARIABLE TABLE ---
// Categorizing variables to handle auto-save for machine config
VarMap varTable[] = {{"SCREW PITCH", &cfg.screwPitch, T_FLOAT, C_MACHINE},
                     {"WINDER SPEED", &cfg.maxRPM_W, T_INT, C_MACHINE},
                     {"TRAVERSE SPEED", &cfg.maxRPM_T, T_INT, C_MACHINE},
                     {"WINDER DIRECTION", &cfg.dirW, T_BOOL, C_MACHINE},
                     {"TRAVERSE DIRECTION", &cfg.dirT, T_BOOL, C_MACHINE},
                     {"LIMIT SWITCH", &cfg.useLimitSwitch, T_BOOL, C_MACHINE},

                     {"WIRE", &active.wireDia, T_FLOAT, C_PRESET},
                     {"COIL LENGTH", &active.coilWidth, T_FLOAT, C_PRESET},
                     {"TURNS", &active.totalTurns, T_LONG, C_PRESET},
                     {"TARGET RPM", &active.targetRPM, T_INT, C_PRESET},
                     {"RAMP", &active.rampRPM, T_INT, C_PRESET},
                     {"START OFFSET", &active.startOffset, T_LONG, C_PRESET}};
const int varCount = sizeof(varTable) / sizeof(VarMap);