/* 
IMPORTANT: USE ARDUINO PIN 3 FOR PWM INPUT. Also make sure your input is protected by at least a resistor, 100kohm, if its a 5v pwm signal. If 12v you may want a level shifter
or at minimum a voltage divider.
MAKE SURE TO SET OUTPUT MODE ON HALTECH TO DUTY CYCLE.
*/
#include "mcp_can.h"

#include <SPI.h>

MCP_CAN CAN(10); // default for those can bus shields - 10 for old boards; 9 for newer ones?
#define CAN_INT 2 //default for those can bus shields 
char msgString[128]; //terrible way of comparing data
long unsigned int rxId; //ID of incoming message
long unsigned int hi = 0x1B200002;
unsigned char len = 0; //not used here, since we dont care about contents of message
unsigned char rxBuf[8]; //not used here, since we dont care about contents of message
unsigned char keepAlive[8] = { 0x00, 0x00, 0x22, 0xE0, 0x41, 0x90, 0x00, 0x00 }; //counter KA
unsigned char speed[8] = { 0xBB,  0x00,  0x3F,  0xFF,  0x06,  0xE0,  0x00,  0x00 }; //counter KA
unsigned char slowRollTable[4] = { 0x00, 0x40, 0x80, 0xC0 };
unsigned long previousMillis = 0;
const long interval = 15; //how fast we send
int slowRoll;
int roll; //how many data messages we're sent, 0-30
bool oHi = false;
unsigned long calVel = 0; // the pwm speed value read 6000-0

void setup() {

  Serial.begin(115200);
  int init = CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ);
  if (init == CAN_OK) {
    Serial.print("CAN init ok\r\n");
  } else {
    Serial.print("CAN init failed: ");
    Serial.print(init, DEC);
    Serial.print("\r\n");
  }
  CAN.setMode(MCP_NORMAL);
  pinMode(CAN_INT, INPUT);
}

void loop() {
// calVel = map(analogRead(1), 0, 1024, 6000, 0);
        // Serial.print("pot value: ");
        // Serial.print(calVel, DEC);
        // Serial.print("\r\n");

  unsigned long currentMillis = millis();
  speed[6] = calVel >> 8;
  speed[7] = calVel;

  if (oHi == true) {
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      if (roll == 0) {
        // Serial.print("sending A\r\n");
        // calVel = map(analogRead(1), 0, 1024, 6000, 0);
        // Serial.print("pot value: ");
        // Serial.print(calVel, DEC);
        // Serial.print("\r\n");

        CAN.sendMsgBuf(0x1AE0092c, 1, 8, keepAlive); //send keep alive with roll
        keepAlive[0] = slowRollTable[slowRoll];
        slowRoll++;
        if (slowRoll == 4) {
          slowRoll = 0;
        }
      }
      
      // Serial.print("sending B\r\n");
      CAN.sendMsgBuf(0x2104136, 1, 8, speed); //send speed
      roll++;
      if (roll == 30) {
        roll = 0;
      }
    }
  }

  if (!digitalRead(CAN_INT)){ //Wait for some Can data to come in
    CAN.readMsgBuf( & rxId, & len, rxBuf);
    // Serial.print("read message\r\n");

    if ((rxId & 0x80000000) == 0x80000000)
      sprintf(msgString, "%.8lX", (rxId & 0x1FFFFFFF));
      // Serial.print("msgString: ");
      // Serial.print(msgString);
      // Serial.print("\r\n");

    if (!strcmp(msgString, "1B200002")) { //This is a workaround since extID is 29 bit and arduino is 8 bit.

      oHi = true;
    }
  }

}