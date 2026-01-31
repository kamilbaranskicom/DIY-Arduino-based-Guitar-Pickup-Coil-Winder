#include "taskqueue.h"

// --- QUEUE HELPERS ---

bool enqueueTask(MachineState s, char m, long target, bool isRelative, int rpm,
                 float ramp) {
  if (taskCount < QUEUE_SIZE) {
    Task &t = taskQueue[tail];
    t.state = s;
    t.motor = m;
    if (isRelative) {
      t.dir = (target > 0) ? 1 : -1;
      t.targetSteps = abs(target);
    } else {
      t.targetPosition = target;
      t.dir = 0;
      // just for initialization; we will calculate the direction when starting
      // the task as we shouldn't assume where the absPos will be
    }
    t.currentSteps = 0;
    t.accelDistance = 0;
    t.startRPM =
        (t.motor == 'W')
            ? cfg.startRPM_W
            : cfg.startRPM_T; // or should we ask active preset for this, as
                              // wire diameter may affect startRPM?
    t.targetRPM = (float)rpm;
    t.currentRPM = t.startRPM;
    t.accelRate = ramp;
    t.isStarted = false;
    t.isDecelerating = false;
    t.isComplete = false;

    tail = (tail + 1) % QUEUE_SIZE;
    taskCount++;
    return true;
  }
  Serial.println(F("ERROR: Task Queue Full!"));
  return false;
}

Task *getCurrentTask() {
  if (taskCount > 0)
    return &taskQueue[head];
  return NULL;
}

void dequeueTask() {
  if (taskCount > 0) {
    head = (head + 1) % QUEUE_SIZE;
    taskCount--;
  }
}

void clearQueue() {
  head = 0;
  tail = 0;
  taskCount = 0;
}