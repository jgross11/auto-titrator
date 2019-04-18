// number of pulses in operation
long pulseCount = 0;

// amount of voltage to supply to valve
int valveVoltage = 255;

// time in milliseconds that valve is open
int valveOpenTime = 500;

// time in milliseconds that valve is closed
int valveCloseTime = 2000;

// time in milliseconds that stirring occurs
int stirTime = 2000;

// pin that holds connection to valve
int valvePin = 5;

// pin that holds connection to pH reader adapter
int sensorPin = 0;

// number of readings averaged to find accurate reading from pH reader
int numReadings = 10;

// calibration coefficient - must be calculated through buffer data extraction
float slope = 1.0;

// offset coefficient - must be calculated through buffer data extraction
float offset = 1.0;

// set pin to 0
int pinOff = 0;

// volumetric calibration parameters (initial guess)
float v2 = -0.0004;
float v1 = 0.3155;
float v0 = -0.0223;

// *** TEST VARIABLES ***

// orange juice pH reading
float testpH;

// volume of orange juice (in mL)
float testVolume;

// water pH reading
float waterpH;

// desired solution pH reading
float desiredpH;

// actual solution pH reading
float solutionpH;

// test pH incrementer
float pHPlus;

// test volume incrementer
float volumePlus;

// actual solution volume
float solutionVolume;

// +/- offset from desired pH allowable
float delta;

// number of operations done in testing
int numTestOps;

// set to true if error happens
boolean errorExit;

String input;

// *** END TEST VARIABLES

void setup() {
  // raise communications barrier
  Serial.begin(9600);

  // user choice
  input = "";
  Serial.println("Enter 0 for tests, 1 for real deal");

  // obtain and interpret user input
  while (true) {
    // wait for serial to open
    if (Serial.available()) {
      // read user input
      input = Serial.readString();

      // test
      input.trim();
      if (input == "0") {
        Serial.println("Entering test mode");
        tests();
        break;
      }
      else if (input == "1") {
        Serial.println("Entering the real deal");
        realOperation();
        break;
      }
      else {
        Serial.println("Invalid choice, restart program");
        break;
      }
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}

void tests() {
  // *** Test 1: simluate dilution of orange juice with water ***
  testpH = 3.5;
  testVolume = 10;
  waterpH = 7;
  desiredpH = 5.2;
  solutionpH = testpH;
  solutionVolume = testVolume;
  delta = 0.1;
  numTestOps = 0;
  errorExit = false;
  pHPlus = 0.12;
  volumePlus = 1;

  Serial.println("Starting test 1");
  doTestOperation();
  Serial.println("End of test 1\n");


  // *** Test 2: Invalid reading obtained - pH too low***

  testpH = -123;
  testVolume = 10;
  waterpH = 7;
  desiredpH = 5.2;
  solutionpH = testpH;
  solutionVolume = testVolume;
  delta = 0.1;
  numTestOps = 0;
  errorExit = false;

  Serial.println("Starting test 2");
  doTestOperation();
  Serial.println("End of test 2\n");

  // *** Test 3: Invalid reading obtained - pH too high***
  testpH = 123;
  testVolume = 10;
  waterpH = 7;
  desiredpH = 5.2;
  solutionpH = testpH;
  solutionVolume = testVolume;
  delta = 0.1;
  numTestOps = 0;
  errorExit = false;

  Serial.println("Starting test 3");
  doTestOperation();
  Serial.println("End of test 3\n");

  // *** Test 4: simluate dilution of orange juice with water pt 2***
  testpH = 2.1;
  testVolume = 12.5;
  waterpH = 7;
  desiredpH = 3.1;
  solutionpH = testpH;
  solutionVolume = testVolume;
  delta = 0.1;
  numTestOps = 0;
  errorExit = false;
  pHPlus = 0.07;
  volumePlus = 0.8;

  Serial.println("Starting test 4");
  doTestOperation();
  Serial.println("End of test 4\n");

  // *** Test 5: simluate dilution of orange juice with water pt 3***
  testpH = 0.1;
  testVolume = 0.5;
  waterpH = 7;
  desiredpH = 6.9;
  solutionpH = testpH;
  solutionVolume = testVolume;
  delta = 0.05;
  numTestOps = 0;
  errorExit = false;
  pHPlus = 0.07;
  volumePlus = 0.2;

  Serial.println("Starting test 5");
  doTestOperation();
  Serial.println("End of test 5\n");
}

void doTestOperation() {
  // print initial values
  Serial.println();
  Serial.println("Initial values");
  Serial.print("Initial pH reading: ");
  Serial.println(solutionpH);
  Serial.print("Initial volume reading (mL): ");
  Serial.println(solutionVolume);
  Serial.println();
  while ( (solutionpH - delta) <= desiredpH || (solutionpH + delta) >= desiredpH ) {
    // increment number of operations
    numTestOps++;
    Serial.print("Operation iteration #");
    Serial.println(numTestOps);

    // add 1 mL of water to solution
    solutionVolume = addWater(solutionVolume);
    
    // obtain solution pH reading (an actual function would read the pH meter in the solution)
    // read ph meter for 1 second

    float newReading = obtainSolutionpHTest(solutionpH);
    Serial.print("New solution pH reading: ");
    Serial.println(newReading);
    Serial.print("New solution volume reading (mL): ");
    Serial.println(solutionVolume);

    // reading is valid
    if (newReading != -1) {
      // accepted pH reading obtained
      if ((newReading - delta) <= desiredpH && (newReading + delta) >= desiredpH) {
        solutionpH = newReading;
        Serial.println();
        break;
      }
    }
    // reading is invalid
    else {
      // exit loop due to error
      errorExit = true;
      break;
    }
    solutionpH = newReading;
    Serial.println();
  }
  if (!errorExit) {
    Serial.print("Accepted solution pH achieved: ");
    Serial.println(solutionpH);
    Serial.print("Number of operations performed: ");
    Serial.println(numTestOps);
    Serial.print("Solution volume (mL): ");
    Serial.println(solutionVolume);
  }
  else {
    Serial.println("Invalid reading, try again");
  }
}

// simulates reading and verification of pH meter
float obtainSolutionpHTest(float result) {
  // water was added, so add a bit to solution pH
  // in testing, this is linear, but in practice,
  // it's very finnicky, especially near equivalence point
  result += pHPlus;
  if (verifypHReading(result)) {
    return result;
  }
  else {
    return -1;
  }
}

// verifies reading was correctly read
boolean verifypHReading(float reading) {
  // check if pH reading is within max and min pH values
  if (reading > 14 || reading < 0) {
    return false;
  }
  else {
    return true;
  }
}

// simulates fluid dispersal
float addWater(float solutionVolume) {
  solutionVolume += volumePlus;
  return solutionVolume;
}

// opens valve in real mode
void realOperation() {
  Serial.println("Press enter to start");
  while (true) {
    // wait for user to continue
    if (Serial.available()) {
      
      // read user input
      input = Serial.readString();
      pulseCount++;
      Serial.print("Pulse ");
      Serial.println(pulseCount);
      
      // open valve
      Serial.print("\nValve open... ");
      analogWrite(valvePin, valveVoltage);
      delay(valveOpenTime);

      // close valve
      analogWrite(valvePin, pinOff);
      Serial.print("closed... Estimated Volume: ");
      delay(valveCloseTime);

      // allow solution to be stirred to ensure proper mixing
      delay(stirTime);
      
      // get the next reading
      solutionpH = getpHReading();
      Serial.print("New reading: ");
      Serial.println(solutionpH);
      
      // TODO: This only works for strong acid / strong base, 
      // TODO: research and refine for any type of titration
      if(solutionpH >= 6.8 && solutionpH <= 7.2){
        Serial.println("Titration complete - process stopped");
        break;
      }
      else if(solutionpH > 7.2){
        Serial.println("Solution pH is above equivalence point + delta (0.2) - process stopped");
        break;
      }
      else{
        Serial.println("Press enter to continue");  
      }
    }
  }
}

// get average reading from pH reader adapter
float getpHReading(){
    long pHsum = 0;
    for(int i = 0; i < numReadings; i++){
      unsigned long raw = analogRead(sensorPin);

      // convert from raw reading to actual voltage and add to sum
      pHsum = raw*(5.0/1023.0);
    }
    // return average of these readings
    return (pHsum / numReadings)*slope + offset;
}
