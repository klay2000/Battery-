 
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

long highTime = -1; // time of high pulse
int socDrop = -1; // drop in soc for each pulse
long pulseInt = -1; // interval between pulses
int minSOC = -1; // min soc for discharge

double ahs = 0; // amp hours of battery

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

double readVolts(){ // reads volts
  
  return (0.01935*analogRead(voltRead)); // 1023=19.8v 0=0v
  
}

double readAmps(){ // reads amps
  
  return ((0.17595*analogRead(ampRead))-1.65);
  
}

String readLine(File file){ // returns the next line from a file
  
  String line = "";

  char cur;

  while(cur != '\n' && file.available()){
    line += cur;
    cur = file.read();
  }

  return line;
  
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

void logData(long phaseTime, int phase, File data){
  data.println(String(readVolts()) + ", " + String(readAmps()) + ", " + String(phaseTime) + ", " + String(phase)); // logs data to sd card
  data.flush();
}

void setup() {
    analogReference(EXTERNAL);
    
    pinMode(setHighAmp, OUTPUT);// set high amperage
    pinMode(resetHighAmp, OUTPUT);// reset high amperage
    pinMode(charge, OUTPUT);// charge
    pinMode(lowAmp, OUTPUT);// low amperage
    //A0 is Current Sense
    //A1 is Voltage Sense
    
    serial.begin(9600);

    Serial.begin(9600);
    
}

void loop() {
    serial.write("   Loading...   ");
    serial.write("    Team 302    ");

    Serial.println("serial debug workz");

    delay(4000); // wait for logic translator to turn on

    clearScreen();

    serial.write(" Checking SD... "); // check for sd card every 500 ms
    while(!SD.begin()){
      delay(500);  
    }

    clearScreen();
    
    delay(1000);

    clearScreen();

    // read the config
    File conf = SD.open("config.txt", FILE_READ);

    while(conf.available()){
      String line = readLine(conf);
      if(!line.charAt(0) == '#'){
        String var = line.substring(0, 3);
        String arg = line.substring(5);

        Serial.println((line));

        if(arg.toInt() < 0){
          serial.write("invalid config! ");
          freeze();
        }
        
        if(var.equals("htim")){ // high pulse time in seconds
          highTime = arg.toInt()*1000;
        }else if(var.equals("drop")){ // total percent soc drop
          socDrop = arg.toInt();
        }else if(var.equals("pint")){ // interval in minutes between pulses
          pulseInt = arg.toInt()*1000*60;
        }else if(var.equals("msoc")){ // charge stop soc
          minSOC = arg.toInt();
        }else{
          serial.write("invalid config! ");
          freeze();
        }
      }
    }
    if(highTime == -1 || socDrop == -1 || pulseInt == -1 || minSOC == -1 || pulseInt%sampleRate > 0 || highTime%sampleRate > 0){ // if something is not right throw error and freeze
          serial.write("invalid config! ");
          freeze();
    }
    
    File data = SD.open("data.csv", FILE_WRITE); // write file to sd card

    if(!data){ // if file won't open then display error and freeze up
      serial.write(" SD card error! ");
      freeze();
    }

    data.println("phase 0 is baseline phase 1 is a discharge phase 2 is a recharge phase 3 is a high pulse"); // write phases explanation row to data file
    data.println("Voltage, Amperage, Time(in ms), Phase"); // write header row to data file
    logData(0, 0, data); // logs baseline as time 0 phase 0
    
    digitalWrite(lowAmp, HIGH); // slowly discharge battery until at 7 volts or less while recording data
    while(readVolts() > 7){
      accurateDelayReset();
      
      ahs += ((readAmps()/(sampleRate/1000))/60)/60;
      
      clearScreen();
      serial.print(" Discharging... ");
      serial.print("V: "+String(readVolts()));
      
      timeInPhase += sampleRate;
      
      accurateDelay(sampleRate);
      
      logData(timeInPhase, 1, data);
    }    
    digitalWrite(lowAmp, LOW); // stop discharging

    timeInPhase = 0;

    clearScreen();

    // recharge battery

    digitalWrite(charge, HIGH); // charge battery until full
    while(readAmps() <= 1){ // stop when at 1 amp or less
      accurateDelayReset();
      
      clearScreen();
      serial.print("  Recharging... ");
      serial.print("A: "+String(readAmps()));
      
      timeInPhase += sampleRate;
      
      accurateDelay(sampleRate);
      
      logData(timeInPhase, 2, data);
    }    
    digitalWrite(charge, LOW); // stop charging

    timeInPhase = 0;
    
    clearScreen();
  
    // discharge pulses code

    double totalDischargedAH = 0;

    for(int i = 0; i++; i < pulseInt / 250){ // wait for length of pulse interval before starting
      accurateDelayReset();

        clearScreen();
        serial.print(" Bat Resting... ");
        serial.print(String(((i*250)/pulseInt)*100) + "%");

      accurateDelay(250);
    }

    clearScreen();

    while((totalDischargedAH/ahs)*100 > minSOC){ // while soc is more than min keep doing stuff
      double pulseDischargedAH = 0;

      // high pulse
      digitalWrite(setHighAmp, HIGH);
      for(int i = 0; i < highTime/sampleRate; i++){
        accurateDelayReset();
  
        logData(i*sampleRate, 3, data);
  
        pulseDischargedAH += ((readAmps()/(sampleRate/1000))/60)/60;
  
        clearScreen();
        serial.print("Testing Battery.");
        serial.print("V: "+String(readVolts())+" A: "+String(readAmps()));
        
        accurateDelay(sampleRate);
      }

      
      digitalWrite(setHighAmp, LOW);
      digitalWrite(resetHighAmp, HIGH);
      delay(250);
      digitalWrite(resetHighAmp, LOW);

      // low discharge pulse
      digitalWrite(lowAmp, HIGH);
      while(pulseDischargedAH / ahs < .1){ // while soc hasn't dropped 10% keep going
        accurateDelayReset();
        
        logData(timeInPhase, 1, data);

        timeInPhase += sampleRate;
        
        pulseDischargedAH += ((readAmps()/(sampleRate/1000))/60)/60;

        clearScreen();
        serial.print("Testing Battery.");
        serial.print("V: "+String(readVolts())+" A: "+String(readAmps()));
        
        accurateDelay(sampleRate);
      }
      digitalWrite(lowAmp, LOW);
      timeInPhase = 0;

      //let battery rest
      for(int i = 0; i++; i < pulseInt / 250){ // wait for length of pulse int before starting
        accurateDelayReset();

          clearScreen();
          serial.print(" Bat Resting... ");
          serial.print(String(((i*250)/pulseInt)*100) + "%");
  
        accurateDelay(250);
      }
    }

    //recharge battery

    digitalWrite(charge, HIGH); // charge battery until full
    while(readAmps() <= 1){ // stop when at 1 amp or less
      accurateDelayReset();
      
      clearScreen();
      serial.print("  Recharging... ");
      serial.print("A: "+String(readAmps()));
      
      timeInPhase += sampleRate;
      
      accurateDelay(sampleRate);
      
      logData(timeInPhase, 2, data);
    }    
    digitalWrite(charge, LOW); // stop charging

    timeInPhase = 0;
    
    clearScreen();

    serial.print(" Done testing!  ");
        
    data.close();

    while(true){
    }
    
}

