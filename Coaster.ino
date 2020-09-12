#include <Eventually.h>

/*
 * K'nex roller coaster dual-train control system, by Stephen Morawski
 * Controls 3 motors: block brake engage/disengage, station gates open/closed, and station tire drives
 * Has 4 sensors: block brake limit switch, station gates limit switch, brake run reed switch, and station reed switch
 */

// Sensors
#define BRAKE_SWITCH 2
#define GATES_SWITCH 3
#define BRAKE_REED 4
#define STATION_REED 5

// Motors
#define BRAKE_1 6
#define BRAKE_2 7
#define GATES_1 8
#define GATES_2 9
#define TIRES 10

// LEDs
#define RED 11
#define YELLOW 12
#define GREEN 13

EvtManager mgr;

void setup() {
  pinMode(BRAKE_SWITCH, INPUT);
  pinMode(GATES_SWITCH, INPUT);
  pinMode(BRAKE_REED, INPUT);
  pinMode(STATION_REED, INPUT);
  
  pinMode(BRAKE_1, OUTPUT);
  pinMode(BRAKE_2, OUTPUT);
  pinMode(GATES_1, OUTPUT);
  pinMode(GATES_2, OUTPUT);
  pinMode(TIRES, OUTPUT);

  pinMode(RED, OUTPUT);
  pinMode(YELLOW, OUTPUT);
  pinMode(GREEN, OUTPUT);

  // Turn everything off until we determine where to start
  for (int i = 2; i <= 13; i += 1) {
    digitalWrite(i, LOW);
  }

  // Check sensors to see what state we start in
  // Most of the time we will start with both trains stopped at the block brake and station
  if (digitalRead(STATION_REED) == HIGH) {
    // One train is in the station, need to find the second train
    if (digitalRead(BRAKE_SWITCH) == HIGH) {
      // Block brake is engaged, so the second train is on the block brake
      // This should be the starting state most of the time
      brakeReed();
    } else {
      // Block brake is disengaged, so the second train must be on the lift hill
      stationReed();
    }
  } else {
    // No train in the station, so one must be on the lift hill and the other on the block brake
    if (digitalRead(BRAKE_SWITCH) == HIGH) {
      // Block brake is engaged, so the train is on the block brake
      gatesSwitchClosed();
    } else {
      // Block brake is disengaged, so the train must be between the block brake and the station
      brakeSwitchClosed();
    }
  }
}

void setLED(int led) {
  // Disable all 3 LEDs
  digitalWrite(RED, LOW);
  digitalWrite(YELLOW, LOW);
  digitalWrite(GREEN, LOW);

  // Enable specified LED
  digitalWrite(led, HIGH);
}

bool brakeReed() {
  // Train has finished the circuit and has arrived at the block brake
  // Need to close the station gates
  mgr.resetContext();
  
  delay(1000);
  setLED(YELLOW);
  digitalWrite(GATES_1, HIGH);
  digitalWrite(GATES_2, LOW);

  mgr.addListener(new EvtPinListener(GATES_SWITCH, 50, HIGH, (EvtAction)gatesSwitchClosed));
  mgr.addListener(new EvtTimeListener(500, false, (EvtAction)gatesSwitchClosed));

  return true;
}

bool gatesSwitchClosed() {
  // Station gates have almost finished closing (need to wait 100ms)
  // Need to send the train in the station to the lift hill as well as disengage the block brake
  mgr.resetContext();

  delay(100);
  digitalWrite(GATES_1, LOW);
  digitalWrite(GATES_2, LOW);
  delay(1000);
  setLED(GREEN);
  digitalWrite(TIRES, HIGH);
  digitalWrite(BRAKE_1, HIGH);
  digitalWrite(BRAKE_2, LOW);

  mgr.addListener(new EvtPinListener(BRAKE_SWITCH, 50, HIGH, (EvtAction)brakeSwitchClosed));
  mgr.addListener(new EvtTimeListener(500, false, (EvtAction)brakeSwitchClosed));

  return true;
}

bool brakeSwitchClosed() {
  // Block brake has almost finished disengaging (need to wait 100ms)
  // Need to wait for the train in the station to fully clear the station reed switch (500ms)
  // Need to wait for the train from the block brake to arrive at the station
  mgr.resetContext();

  delay(100);
  digitalWrite(BRAKE_1, LOW);
  digitalWrite(BRAKE_2, LOW);
  delay(500);
  setLED(RED);
  
  mgr.addListener(new EvtPinListener(STATION_REED, 50, HIGH, (EvtAction)stationReed));

  return true;
}

bool stationReed() {
  // Train has arrived at the station
  // Need to engage block brake
  mgr.resetContext();

  digitalWrite(TIRES, LOW);
  delay(1000);
  digitalWrite(BRAKE_1, LOW);
  digitalWrite(BRAKE_2, HIGH);
  
  mgr.addListener(new EvtPinListener(BRAKE_SWITCH, 50, LOW, (EvtAction)brakeSwitchOpen));
  mgr.addListener(new EvtTimeListener(500, false, (EvtAction)brakeSwitchOpen));
  
  return true;
}

bool brakeSwitchOpen() {
  // Block brake has finished engaging
  // Need to open the station gates
  mgr.resetContext();

  digitalWrite(BRAKE_1, LOW);
  digitalWrite(BRAKE_2, LOW);
  digitalWrite(GATES_1, LOW);
  digitalWrite(GATES_2, HIGH);

  mgr.addListener(new EvtPinListener(GATES_SWITCH, 50, LOW, (EvtAction)gatesSwitchOpen));
  mgr.addListener(new EvtTimeListener(500, false, (EvtAction)gatesSwitchOpen));
  
  return true;
}

bool gatesSwitchOpen() {
  // Station gates have finished opening
  // Need to wait for the train to finish the course and return to the block brake
  mgr.resetContext();

  digitalWrite(GATES_1, LOW);
  digitalWrite(GATES_2, LOW);

  mgr.addListener(new EvtPinListener(BRAKE_REED, 50, HIGH, (EvtAction)brakeReed));

  return true;
}

USE_EVENTUALLY_LOOP(mgr)
