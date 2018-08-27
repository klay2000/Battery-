// Reset a SparkFun Serial LCD (16*2)
// Bob Stephens 03/06/2009
// -------------------------------------------------------------------------------

void resetLCD();
void setSplash();

#include <SoftwareSerial.h>


SoftwareSerial serial(9,8);

// --------------------------------------------------------
void setup()
{
// start serial port
serial.begin(9600); // open serial
// setSplash();
}

// --------------------------------------------------------
void loop()
{
resetLCD();
}

void resetLCD() {
serial.write(0x7C);
serial.write(0x04); 
}
