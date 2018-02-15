 
#include <SPI.h>

#include <SD.h>

#include <SoftwareSerial.h>

File logFile;

// TODO: discharge pulses, charge after, config system

int ampRead = A0; // amp read pin
int voltRead = A1; // volt read pin

int setHighAmp = 4; // set high amp pin
int resetHighAmp = 5; // reset high amp pin
int charge = 6; // charge pin
int lowAmp = 7; // low amp pin

long timeInPhase = 0; // time spent in current test phase
int sampleRate = 100; // sample rate in miliseconds

long accurateDelayTime = 0;//var used by accurateDelay

SoftwareSerial serial(9,8);

void accurateDelay(long wait){ // waits until wait miliseconds from last time accurateDelayReset() was called
  if(millis() - accurateDelayTime < wait){
    delay(wait-(millis()-accurateDelayTime));
  }
}
void accurateDelayReset(){ // resets accurate delay
  accurateDelayTime = millis();
}

double readVolts(){ // converts ain to volts
  
  return (0.01935*analogRead(voltRead)); // 1023=19.8v 0=0v
  
}
double readAmps(){ // converts ain to amps
  
  return ((0.17595*analogRead(ampRead))-1.65);
  
}

void freeze(){ // freeze up
      while(true){
        delay(10000);
      }
}

void clearScreen(){ // clears screen
    serial.write(0xFE);
    serial.write(0x01);
}

void logData(long phaseTime, int phase){
  data.println(String(readVolts()) + ", " + String(readAmps) + ", " + String(phaseTime) + ", " + String(phase)); // logs data to sd card
}

void setup() {
    analogReference(EXTERNAL);
    
    pinMode(setHighAmp, OUTPUT);// set high amperage
    pinMode(resetHighAmp, OUTPUT);// reset high amperage
    pinMode(charge, OUTPUT);// charge
    pinMode(lowAmp, OUTPUT);// low amperage
    //A0 is Current Sense
    //A1 is Voltage Sense
    
    serial.begin(9700);
    
}

void loop() {
    serial.write("   Loading...   ");
    serial.write("    Team 302    ");

    delay(4000); // wait for logic translator to turn on

    clearScreen();

    serial.write(" Checking SD... "); // check for sd card every 500 ms
    while(!SD.begin()){
      delay(500);  
    }

    clearScreen();
    
    serial.write(" SD card found! ");

    delay(1000);

    clearScreen();

    File data = SD.open("data.csv", FILE_WRITE); // write file to sd card

    if(!data){ // if file won't open then display error and freeze up
      serial.write(" SD card error! ");
      freeze();
    }

    data.println("phase 0 is baseline phase 1 is discharge phase 2 is recharge phase 3 and onwards is discharge pulses highest num is recharge"); // write phases explanation row to data file
    data.println("Voltage, Amperage, Time(in ms), Phase"); // write header row to data file
    logData(0, 0); // logs baseline as time 0 phase 0
    
    digitalWrite(lowAmp, HIGH); // slowly discharge battery until at 7 volts or less while recording data
    while(ainToVolt(analogRead(voltRead)) > 7){
      accurateDelayReset();
      clearScreen();
      serial.print(" Discharging... ");
      serial.print("V: "+String(ainToVolt(analogRead(voltRead))));
      timeInPhase += sampleRate;
      accurateDelay(sampleRate);
      logData(timeInPhase, 1);
    }    
    digitalWrite(lowAmp, LOW); // stop discharging

    timeInPhase = 0;

    clearScreen();

    digitalWrite(charge, HIGH); // charge battery until full
    while(readAmps() <= 1){ // stop when at 1 amp or less
      accurateDelayReset();
      clearScreen();
      serial.print("  Recharging... ");
      serial.print("A: "+String(readAmps()));
      timeInPhase += sampleRate;
      accurateDelay(sampleRate);
      logData(timeInPhase, 2);
    }    
    digitalWrite(charge, LOW); // stop charging

    // discharge pulses code
    // hi-1s lo-10% drop wait-15m num-until 10% soc
    while()
    
    
}

