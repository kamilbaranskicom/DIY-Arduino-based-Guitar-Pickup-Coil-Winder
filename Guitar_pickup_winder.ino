#include <Arduino.h>
#include "BasicStepperDriver.h"
#include "SyncDriver.h"

// --- PINS ---
#define W_STEP 17
#define W_DIR  16
#define T_STEP 15
#define T_DIR  14
#define EN 12

// --- SETTINGS ---
#define MOTOR_STEPS 200
#define MICROSTEPS 8      
#define MAX_RPM 400       
#define START_RPM 100     
#define CHUNK_SIZE 10     // Number of turns processed in one go

int wireDiaRaw = 0, coilWidth = 0, targetTurns = 0;
int currentTurns = 0, turnsInLayer = 0, turnsPerLayer = 0;
int layerDir = -1;
bool isWinding = false;

BasicStepperDriver winder(MOTOR_STEPS, W_DIR, W_STEP);
BasicStepperDriver traverse(MOTOR_STEPS, T_DIR, T_STEP);
SyncDriver controller(winder, traverse);

void setup() {
  pinMode(EN, OUTPUT);
  digitalWrite(EN, HIGH);
  
  // High baud rate for faster communication
  Serial.begin(115200); 
  
  winder.begin(START_RPM, MICROSTEPS);
  traverse.begin(START_RPM, MICROSTEPS);
  
  Serial.println("--- READY ---");
  Serial.println("Format: 'wire width turns' (e.g. '1 10 1000')");
  Serial.println("Empty line (Enter) to STOP");
}

void stopMachine() {
  isWinding = false;
  digitalWrite(EN, HIGH);
  Serial.println(">>> STOPPED");
}

void startMachine() {
  if (wireDiaRaw > 0 && coilWidth > 0 && targetTurns > 0) {
    currentTurns = 0;
    turnsInLayer = 0;
    turnsPerLayer = (coilWidth * 10) / wireDiaRaw;
    isWinding = true;
    digitalWrite(EN, LOW);
    Serial.println(">>> WINDING...");
  }
}

void handleSerial() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Empty line = STOP
    if (input.length() == 0) {
      stopMachine();
      return;
    }

    // Parse "1 10 1000"
    int space1 = input.indexOf(' ');
    int space2 = input.lastIndexOf(' ');

    if (space1 != -1 && space2 != -1 && space1 != space2) {
      wireDiaRaw = input.substring(0, space1).toInt();
      coilWidth = input.substring(space1 + 1, space2).toInt();
      targetTurns = input.substring(space2 + 1).toInt();
      
      Serial.print("Params: Wire="); Serial.print(wireDiaRaw);
      Serial.print(" Width="); Serial.print(coilWidth);
      Serial.print(" Turns="); Serial.println(targetTurns);
      
      startMachine();
    }
  }
}

void loop() {
  handleSerial();

  if (isWinding) {
    if (currentTurns < targetTurns) {
      // Calculate remaining counts for layer and total
      int remInLayer = turnsPerLayer - turnsInLayer;
      int remTotal = targetTurns - currentTurns;
      int currentChunk = min(CHUNK_SIZE, min(remInLayer, remTotal));

      // Simple acceleration
      if (currentTurns <= 100) {
        float rpm = map(currentTurns, 0, 100, START_RPM, MAX_RPM);
        winder.begin(rpm, MICROSTEPS);
        traverse.begin(rpm, MICROSTEPS);
      }

      // 180 degrees per 1mm. wireDiaRaw 1 = 0.1mm.
      double degWinder = 360.0 * currentChunk;
      double degTraverse = (wireDiaRaw * 18.0) * currentChunk;

      // Executing synchronized chunk
      controller.rotate(degWinder, (double)(layerDir * degTraverse));

      currentTurns += currentChunk;
      turnsInLayer += currentChunk;

      if (turnsInLayer >= turnsPerLayer) {
        layerDir *= -1;
        turnsInLayer = 0;
        Serial.println("Layer flip");
      }

      Serial.print("T: "); Serial.println(currentTurns);
    } else {
      stopMachine();
    }
  }
}