#include <Stepper.h>
#include <Arduino.h>
#include "BasicStepperDriver.h"
#include "MultiDriver.h"
#include "SyncDriver.h"
#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3); 

int A, B, C = 0; 
int L = 40; 
int state = 0; 
int EN = 12; 
String message;
int QTY, numMessages, endBytes;
byte inByte;

#define DIR_X 14 
#define STEP_X 15 
#define DIR_Y 16 
#define STEP_Y 17 
#define MICROSTEPS 16 
#define MOTOR_STEPS 200 

int count = 0; 

BasicStepperDriver stepperX(MOTOR_STEPS, DIR_X, STEP_X);
BasicStepperDriver stepperY(MOTOR_STEPS, DIR_Y, STEP_Y);
SyncDriver controller(stepperX, stepperY);

void setup()
{ 
  pinMode(EN, OUTPUT);
  digitalWrite(EN, HIGH); 
  
  // Fixed initialization
  numMessages = 0;
  endBytes = 0; 
  
  Serial.begin(9600); 
  mySerial.begin(9600); 
  
  stepperX.begin(250, MICROSTEPS);
  stepperY.begin(250, MICROSTEPS);
  delay(500);
  Serial.println("System Ready - Waiting for HMI data...");
}

void data() {
  if (mySerial.available()) {
    inByte = mySerial.read();

    // Logic: Collect only digits, ignore 'p' and other characters
    if (inByte >= '0' && inByte <= '9') {
      message.concat(char(inByte));
    }
    else if (inByte == 255) {
      endBytes++;
    }

    // When 3 end-bytes are received, convert message to integer
    if (endBytes == 3) {
      QTY = message.toInt();
      message = "";
      endBytes = 0;
      numMessages = 1; // Flag that a new number is ready to be assigned
    }
  }

  // State machine to assign values to A, B, and C sequentially
  if (numMessages == 1) {
    if (state == 0) {
      A = QTY;
      Serial.print("Assigned A (Wire): "); Serial.println(A);
      state = 1;
    } 
    else if (state == 1) {
      B = QTY;
      Serial.print("Assigned B (Width): "); Serial.println(B);
      state = 2;
    } 
    else if (state == 2) {
      C = QTY;
      Serial.print("Assigned C (Turns): "); Serial.println(C);
      state = 0; 
    }
    numMessages = 0; 
  }
}

void loop()
{
  data();
  
  if (A > 0 && B > 0 && C > 0) {
    digitalWrite(EN, LOW); 

    if (count <= C) {
      // Winding logic using hardcoded L and rotation steps
      for (int i = 0; i <= L; i++) {
        if (count > C) break;
        controller.rotate(360, 40);
        
        // Update display counter
        mySerial.print("n2.val=");
        mySerial.print(count);
        mySerial.write(0xff); mySerial.write(0xff); mySerial.write(0xff);
        count++;  
      }

      for (int i = 0; i <= L; i++) {
        if (count > C) break;
        controller.rotate(360, -40);  
        
        mySerial.print("n2.val=");
        mySerial.print(count);
        mySerial.write(0xff); mySerial.write(0xff); mySerial.write(0xff);
        count++;
      }
    }   
    
    // Stop after finishing turns
    if (count > C) {
       digitalWrite(EN, HIGH);
       // Reset values to wait for next job
       A = 0; B = 0; C = 0; count = 0;
       Serial.println("Winding Complete!");
    }
  }
}