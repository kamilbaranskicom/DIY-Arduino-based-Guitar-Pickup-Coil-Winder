#include "variables.h"

// --- COMMAND HANDLERS ---

void handleSet(String line) {
  line.remove(0, 4); // Remove "SET "
  line.trim();

  for (int i = 0; i < varCount; i++) {
    String label = String(varTable[i].label);
    if (line.startsWith(label)) {
      String valStr = line.substring(label.length());
      valStr.trim();

      // If no value provided, do nothing
      if (valStr.length() == 0)
        return;

      switch (varTable[i].type) {
      case T_FLOAT:
        *(float *)varTable[i].ptr = valStr.toFloat();
        break;
      case T_INT:
        *(int *)varTable[i].ptr = valStr.toInt();
        break;
      case T_LONG:
        *(long *)varTable[i].ptr = valStr.toInt();
        break;
      case T_BOOL:
        *(bool *)varTable[i].ptr = parseBool(valStr, label);
        break;
      }

      Serial.print(F("CONFIRMED: "));
      Serial.print(label);
      Serial.print(F(" = "));
      handleGet(F("GET ") + label);
      return;
    }
  }
  Serial.println(F("ERROR: Unknown parameter"));
}

void handleGet(String line) {
  line.remove(0, 4); // Remove "GET "
  line.trim();

  bool found = false;
  for (int i = 0; i < varCount; i++) {
    String label = String(varTable[i].label);

    // If line is empty, print ALL. If label matches, print ONE.
    if (line.length() == 0 || line == label) {
      found = true;
      Serial.print(label);
      Serial.print(F(": "));

      switch (varTable[i].type) {
      case T_FLOAT:
        Serial.println(*(float *)varTable[i].ptr, 3);
        break;
      case T_INT:
        Serial.println(*(int *)varTable[i].ptr);
        break;
      case T_LONG:
        Serial.println(*(long *)varTable[i].ptr);
        break;
      case T_BOOL:
        bool val = *(bool *)varTable[i].ptr;
        if (label.indexOf(F("DIRECTION")) >= 0) {
          Serial.println(val ? F("FORWARD") : F("BACKWARD"));
        } else {
          Serial.println(val ? F("ON") : F("OFF"));
        }
        break;
      }
    }
  }
  if (!found)
    Serial.println(F("ERROR: Parameter not found"));
}

// Helper function to parse human-friendly boolean values
bool parseBool(String val, String label) {
  val.toUpperCase();
  val.trim();

  // Directions logic
  if (label.indexOf(F("DIRECTION")) >= 0) {
    if (val == F("FORWARD"))
      return true;
    if (val == F("BACKWARD"))
      return false;
  }

  // Standard logic
  if (val == F("ON") || val == F("1") || val == F("TRUE") || val == F("YES"))
    return true;
  return false;
}