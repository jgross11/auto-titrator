// number of pulses in operation
long pulseCount = 0;

// amount of voltage to supply to valve
int valveVoltage = 255;

// time in milliseconds that valve is open
int valveOpenTime = 2000;

// time in milliseconds to dispense one drop with 90% accuracy
int oneDropTime = 125;

// time in milliseconds that valve is closed
int valveCloseTime = 2000;

// time in milliseconds that stirring occurs
int stirTime = 6000;

// pin that holds connection to valve
int valvePin = 5;

// pin that holds connection to pH reader adapter
int sensorPin = 0;

// number of readings averaged to find accurate reading from pH reader
int numReadings = 100;

// calibration coefficient - must be calculated through buffer data extraction
float offset;

// offset coefficient - must be calculated through buffer data extraction
float slope;

// set pin to 0
int pinOff = 0;

// coefficient to alter dispense time TODO pinpoint an acceptable number
float dispenseTimeCoeff = 1.1;

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
  Serial.println("Enter 0 for tests, 1 for real deal");
  delay(50);
  // user choice
  input = "";

  // obtain and interpret user input
  while (true) {
    // wait for serial to open
    if (Serial.available()) {
      // read user input
      input = Serial.readString();
      input.trim(); 

      // test
      if (input == "0") {
        Serial.println("Entering test mode");
        tests();
        break;
      }
      else if (input == "1") {
        // initial values are these so the meter returns uncalibrated results
        slope= -3.53;
        offset= 14.34;


        // calibration offer
        Serial.print("Current calibration values are: slope= ");
        Serial.print(slope);
        Serial.print(", offset= ");
        Serial.print(offset);
        Serial.println(". Do you wish to recalibrate? y/n");
        
        // must calibrate meter before starting titration
        while (true) {
            // wait for serial to open
            if (Serial.available()) {
                // read user input
                input = Serial.readString();
                input.trim();
                input.toLowerCase();
                break; 
            }
        }
        if(input == "y"){
            calibrateMeter();
        }
        // obtain desired pH and delta values
        Serial.println("\nEnter equivalence point pH");
        while(true){
         if(Serial.available()){
          input = Serial.readString();
          input.trim();
          desiredpH = input.toFloat();
          Serial.print("Equivalence point pH: "); Serial.println(desiredpH);
          break;
         }
        }
        Serial.println("Enter delta");
        while(true){
         if(Serial.available()){
          input = Serial.readString();
          input.trim();
          delta = input.toFloat();
          Serial.print("Delta: "); Serial.println(delta);
          break;
         }
        }

        // wait for final confirmation and start titration
        Serial.println("Now, clean the meter, place it in your starting solution, and press enter to obtain the initial pH reading.");
        while(true){
          bool read = false;
          while(Serial.available()){
            Serial.readString().trim();
            read = true;
          }
          if (read){
            break;
          }
        }
        Serial.flush();
        delay(1000);
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

void calibrateMeter(){
  float bufferpH1, readpH1, bufferpH2, readpH2;
  Serial.println("\n Before you start titrating, you must calibrate your meter.");
  Serial.println("Place your meter in the first buffer solution and type in its pH: ");
  while(true){
    if(Serial.available()){
      input = Serial.readString();
      input.trim();
      bufferpH1 = input.toFloat();
      readpH1 = getpHReading();
      break;
    }
  }
  Serial.println("Now, clean the meter, place it in the second buffer solution, and type in its pH: ");
  while(true){
    if(Serial.available()){
      input = Serial.readString();
      input.trim();
      bufferpH2 = input.toFloat();
      readpH2 = getpHReading();
      break;
    }  
  }
  // slope = deltaY / deltaX, or in this case, known pH difference / uncalibrated reading difference
  slope = (bufferpH2 - bufferpH1) / (readpH2 - readpH1);

  // offset = known pH - (the slope * uncalibrated reading), or in this case, the average of both readings in that formula 
  offset = ((bufferpH1 - (slope*readpH1)) + (bufferpH2 - (slope*readpH2))) / 2;
  Serial.println("Obtained following results");
  Serial.print("buffer one actual pH: "); Serial.println(bufferpH1);
  Serial.print("buffer one read pH: "); Serial.println(readpH1);
  Serial.print("buffer two actual pH: "); Serial.println(bufferpH2);
  Serial.print("buffer two read pH: "); Serial.println(readpH2);
  Serial.print("slope: "); Serial.println(slope);
  Serial.print("offset: "); Serial.println(offset);
}


// opens valve in real mode
void realOperation() {
  Serial.print("Initial pH reading: ");
  solutionpH = getpHReading();
  Serial.println(solutionpH);
  Serial.println("Press enter to start titrating");
  while(true){
      if(Serial.available()){
          break;  
      }
  }
  while (true) {
      // increment and display pulse 
      pulseCount++;
      Serial.print("\nPulse ");
      Serial.println(pulseCount);
      
      // open valve
      Serial.print("Valve open | ");
      analogWrite(valvePin, valveVoltage);
      delay(valveOpenTime);

      // close valve
      analogWrite(valvePin, pinOff);
      Serial.print("closed | ");
      
      Serial.print("stirring | ");
      Serial.println();
      // allow solution to be stirred to ensure proper mixing
      delay(stirTime);
      
      // get the next reading
      float oldReading = solutionpH;
      solutionpH = getpHReading();
      
      // eqValue = valve open time - (change in reading * dispenseCoeff) 
      if(valveOpenTime != oneDropTime){
        float eqValue;
        if(solutionpH - oldReading > 0.0){
            // TODO 2 = initial dispense time
            eqValue = (2 - ((solutionpH - oldReading) * dispenseTimeCoeff)) * 1000;
            valveOpenTime = (eqValue < valveOpenTime) ? ((eqValue <= oneDropTime) ? oneDropTime : (eqValue)) : valveOpenTime;
        }
        Serial.print("oldReading | solutionpH | eqval | valveOpentime: ");
        Serial.print(oldReading);
        Serial.print(" | ");
        Serial.print(solutionpH);
        Serial.print(" | ");
        Serial.print(eqValue);
        Serial.print(" | ");
        Serial.println(valveOpenTime); 
      }
        Serial.print("New reading: ");
        Serial.println(solutionpH);
        Serial.print("New dispense time: ");
        Serial.print(valveOpenTime/1000.0);
        Serial.println("s");
      // TODO: This only works for strong acid / strong base, 
      // TODO: research and refine for any type of titration
      if(solutionpH >= desiredpH - delta && solutionpH <= desiredpH + delta){
        Serial.println("Titration complete within +/- error - process stopped");
        break;
      }
      else if(solutionpH > desiredpH + delta){
        Serial.println("Solution pH is above equivalence point + delta - process stopped");
        break;
      }
  }
}

// get average reading from pH reader adapter
float getpHReading(){
    float pHsum = 0;
    for(int i = 0; i < numReadings; i++){
      unsigned long raw = analogRead(sensorPin);

      // convert from raw reading to actual voltage and add to sum
      pHsum += raw*(5.0/1023.0);
    }
    // return average of these readings
    return (pHsum / numReadings)*slope + offset;
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
  return (reading > 14 || reading < 0);
}

// simulates fluid dispersal
float addWater(float solutionVolume) {
  solutionVolume += volumePlus;
  return solutionVolume;
}
