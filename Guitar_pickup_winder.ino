#include "BasicStepperDriver.h"
#include "SyncDriver.h"
#include <Arduino.h>
#include <SoftwareSerial.h>

// --- HARDWARE CONFIGURATION ---
#define WINDER_STEP 17
#define WINDER_DIR 16
#define TRAVERSE_STEP 15
#define TRAVERSE_DIR 14
#define ENABLE_PIN 12

SoftwareSerial mySerial(2, 3);

// --- PARAMETERS ---
int wireDiameterRaw = 0; // e.g., 1 for 0.1mm
int coilWidthMM = 0;
int targetTurns = 0;

int currentTurns = 0;
int turnsInCurrentLayer = 0;
int turnsPerLayer = 0;
int layerDirection = -1; // Starts "away" from home based on your test
bool isWinding = false;

// --- STEPPER CONFIGURATION ---
// Changed both to 8 for speed and sync reliability
#define MOTOR_STEPS 200
#define MICROSTEPS 8

// Target speeds for pickup winding
#define MAX_RPM 600
#define START_RPM 100

BasicStepperDriver winder(MOTOR_STEPS, WINDER_DIR, WINDER_STEP);
BasicStepperDriver traverse(MOTOR_STEPS, TRAVERSE_DIR, TRAVERSE_STEP);
SyncDriver controller(winder, traverse);

String message = "";
int endBytes = 0;
int inputCounter = 0;

void setup() {
  pinMode(ENABLE_PIN, OUTPUT);
  digitalWrite(ENABLE_PIN, HIGH);

  Serial.begin(9600);
  mySerial.begin(9600);

  winder.begin(START_RPM, MICROSTEPS);
  traverse.begin(START_RPM, MICROSTEPS);

  Serial.println(">>> System calibrated for 1600 steps/rev <<<");
}

void startMachine() {
  if (targetTurns > 0 && wireDiameterRaw > 0) {
    currentTurns = 0;
    turnsInCurrentLayer = 0;
    isWinding = true;
    digitalWrite(ENABLE_PIN, LOW);
    winder.begin(START_RPM, MICROSTEPS);
    Serial.println("WINDING_START");
  }
}

void stopMachine() {
  isWinding = false;
  digitalWrite(ENABLE_PIN, HIGH);
  Serial.println("WINDING_STOP");
  inputCounter = 0;
}

void assignParam(int val) {
  if (inputCounter == 0) {
    wireDiameterRaw = val;
    inputCounter++;
  } else if (inputCounter == 1) {
    coilWidthMM = val;
    inputCounter++;
  } else if (inputCounter == 2) {
    targetTurns = val;
    if (wireDiameterRaw > 0) {
      // Logic: (Width * 10) / Wire_Raw_Units
      turnsPerLayer = (coilWidthMM * 10) / wireDiameterRaw;
      Serial.print("Layer Size: ");
      Serial.println(turnsPerLayer);
    }
    inputCounter = 0;
    startMachine();
  }
}

void handleHMI() {
  if (mySerial.available()) {
    byte inByte = mySerial.read();
    if ((inByte >= '0' && inByte <= '9') || (inByte >= 'A' && inByte <= 'Z')) {
      message += (char)inByte;
    } else if (inByte == 255) {
      endBytes++;
    }

    if (endBytes == 3) {
      message.trim();
      if (message == "START")
        startMachine();
      else if (message == "STOP")
        stopMachine();
      else if (message.length() > 0)
        assignParam(message.toInt());
      message = "";
      endBytes = 0;
    }
  }
}

void loop() {
  handleHMI();

  if (isWinding) {
    if (currentTurns < targetTurns) {
      // Soft Start ramp over first 50 turns
      if (currentTurns <= 50) {
        float currentRPM = map(currentTurns, 0, 50, START_RPM, MAX_RPM);
        winder.begin(currentRPM, MICROSTEPS);
        traverse.begin(currentRPM, MICROSTEPS);
      }

      // 2mm pitch lead screw: 180 degrees rotate = 1mm move
      // Traverse degrees = wireDiameter(mm) * 180
      float degreesToMove = (float)wireDiameterRaw * 18.0;

      // This command moves BOTH motors simultaneously
      controller.rotate((double)360, (double)(layerDirection * degreesToMove));

      currentTurns++;
      turnsInCurrentLayer++;

      if (turnsInCurrentLayer >= turnsPerLayer) {
        layerDirection *= -1;
        turnsInCurrentLayer = 0;
        Serial.println("Layer_Flip");
      }

      mySerial.print("n2.val=");
      mySerial.print(currentTurns);
      mySerial.write(0xff);
      mySerial.write(0xff);
      mySerial.write(0xff);
    } else {
      stopMachine();
    }
  }
}