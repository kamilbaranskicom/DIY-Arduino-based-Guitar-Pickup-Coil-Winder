#include <Arduino.h>
#include <SoftwareSerial.h>

// --- PINS ---
#define W_STEP 17
#define W_DIR 16
#define T_STEP 15
#define T_DIR 14
#define EN 12

SoftwareSerial nextionSerial(2, 3); // HMI on pins 2 (RX) and 3 (TX)

// --- MECHANICAL SETTINGS (1600 steps/rev) ---
#define STEPS_PER_REV 1600
#define STEPS_PER_MM 800 // 1600 steps / 2mm pitch

// --- WINDING PARAMETERS ---
long wireDiaRaw = 0;  // input '1' = 0.1mm
long coilWidthMM = 0; // input '10' = 10mm
long targetTurns = 0;
long currentStepsW = 0;
long totalTargetSteps = 0;
long stepsPerLayer = 0;
long currentLayerSteps = 0;
long traverseAccumulator = 0;
int layerDir = -1;
bool isWinding = false;

// --- SPEED & ACCELERATION ---
long currentDelay = 400;
long minDelay = 70; // Approx 500-600 RPM limit for Nano
long startDelay = 400;
long rampSteps = 8000; // Soft start over 5 turns

void setup() {
  pinMode(EN, OUTPUT);
  pinMode(W_STEP, OUTPUT);
  pinMode(W_DIR, OUTPUT);
  pinMode(T_STEP, OUTPUT);
  pinMode(T_DIR, OUTPUT);
  digitalWrite(EN, HIGH);

  // Adjusted to 57600 as requested
  Serial.begin(57600);
  nextionSerial.begin(9600);

  Serial.println("--- MANUAL SYNC MODE READY ---");
  Serial.println("try: \"1 10 1000\" for 0.1 mm wire diameter / 10 mm coil "
                 "width / 1000 turns.");
  Serial.println("use empty line (Enter) to stop.");
}

void stopMachine() {
  isWinding = false;
  digitalWrite(EN, HIGH);
  Serial.println(">>> STOPPED");
}

void updateHMI(long turns) {
  nextionSerial.print("n2.val=");
  nextionSerial.print(turns);
  nextionSerial.write(0xff);
  nextionSerial.write(0xff);
  nextionSerial.write(0xff);

  Serial.print("Turn: ");
  Serial.println(turns);
}

void startMachine() {
  if (wireDiaRaw > 0 && coilWidthMM > 0 && targetTurns > 0) {
    totalTargetSteps = targetTurns * STEPS_PER_REV;

    // Calculate winder steps for one full layer
    // turns_per_layer = width / (wireDia/10)
    stepsPerLayer = (coilWidthMM * 10 * STEPS_PER_REV) / wireDiaRaw;

    currentStepsW = 0;
    currentLayerSteps = 0;
    traverseAccumulator = 0;
    currentDelay = startDelay;
    isWinding = true;

    digitalWrite(EN, LOW);
    digitalWrite(W_DIR, HIGH);
    digitalWrite(T_DIR, (layerDir == 1) ? HIGH : LOW);
    Serial.println(">>> WINDING STARTED");
  }
}

void handleInput() {
  // Handle Serial Monitor input
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() == 0)
      stopMachine();
    else {
      int s1 = input.indexOf(' ');
      int s2 = input.lastIndexOf(' ');
      if (s1 != -1 && s2 != -1) {
        wireDiaRaw = input.substring(0, s1).toInt();
        coilWidthMM = input.substring(s1 + 1, s2).toInt();
        targetTurns = input.substring(s2 + 1).toInt();
        startMachine();
      }
    }
  }

  // Handle Nextion input
  while (nextionSerial.available()) {
    // Basic Nextion parsing could be added here if needed,
    // but Serial is primary for this diagnostic phase.
    nextionSerial.read();
  }
}

void loop() {
  handleInput();

  if (isWinding) {
    if (currentStepsW < totalTargetSteps) {

      // 1. STEP WINDER
      digitalWrite(W_STEP, HIGH);

      // 2. STEP TRAVERSE (Bresenham Sync)
      // Traverse needs 80 steps per 0.1mm (one wire diameter raw unit)
      // Per turn (1600 steps), it needs (wireDiaRaw * 80) steps.
      traverseAccumulator += (wireDiaRaw * 80);
      if (traverseAccumulator >= STEPS_PER_REV) {
        digitalWrite(T_STEP, HIGH);
        traverseAccumulator -= STEPS_PER_REV;
      }

      delayMicroseconds(currentDelay);
      digitalWrite(W_STEP, LOW);
      digitalWrite(T_STEP, LOW);
      delayMicroseconds(currentDelay);

      // 3. SMOOTH ACCELERATION
      if (currentStepsW < rampSteps && currentDelay > minDelay) {
        if (currentStepsW % 40 == 0)
          currentDelay--;
      }

      currentStepsW++;
      currentLayerSteps++;

      // 4. LAYER DIRECTION FLIP
      if (currentLayerSteps >= stepsPerLayer) {
        layerDir *= -1;
        digitalWrite(T_DIR, (layerDir == 1) ? HIGH : LOW);
        currentLayerSteps = 0;
        Serial.println("Layer Change");
      }

      // 5. UPDATE PROGRESS (Every full turn = 1600 steps)
      if (currentStepsW % STEPS_PER_REV == 0) {
        updateHMI(currentStepsW / STEPS_PER_REV);
      }

    } else {
      stopMachine();
    }
  }
}