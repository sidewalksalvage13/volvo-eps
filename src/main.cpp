#include "mcp_can.h"
#include <SPI.h>

#define PIN_CAN_CS 10
#define PIN_CAN_INT 2
#define MIN_EPS_SPEED 0
#define MAX_EPS_SPEED 6000

const int send_interval_ms = 15;
const unsigned long EPS_SPEED_MSG_ID = 0x2104136;
const unsigned long KEEP_ALIVE_MSG_ID = 0x1AE0092C;

unsigned char eps_speed_msg[8] = { 0xBB,  0x00,  0x3F,  0xFF,  0x06,  0xE0,  0x00,  0x00 };
unsigned char keep_alive_msg[8] = { 0x00, 0x00, 0x22, 0xE0, 0x41, 0x90, 0x00, 0x00 };

int eps_speed_counter;
int keep_alive_counter;

unsigned long previous_time_ms = 0;
bool eps_connected = false;
unsigned long eps_speed = 0;

MCP_CAN CAN(PIN_CAN_CS);

void setup() 
{
  // Enable Serial
  Serial.begin(115200);
  
  // Setup CAN Interrupt Pin
  pinMode(PIN_CAN_INT, INPUT);
  
  // Configure and start CAN Bus
  int init = CAN.begin(MCP_ANY, CAN_500KBPS, MCP_16MHZ);
  if (init == CAN_OK) 
  {
    Serial.print("CAN init ok\r\n");
  } 
  else 
  {
    Serial.print("CAN init failed: ");
    Serial.print(init, DEC);
    Serial.print("\r\n");
  }
  CAN.setMode(MCP_NORMAL);
}

void loop() 
{
  unsigned long current_time_ms = millis();

  if (eps_connected == true) 
  {
    if (current_time_ms - previous_time_ms >= send_interval_ms) 
    {
      previous_time_ms = current_time_ms;

      eps_speed_msg[6] = eps_speed >> 8;
      eps_speed_msg[7] = eps_speed;

      // Send Speed
      CAN.sendMsgBuf(EPS_SPEED_MSG_ID, 1, 8, eps_speed_msg);
      eps_speed_counter++;
      if (eps_speed_counter == 30) {
        eps_speed_counter = 0;
      }

      if (eps_speed_counter == 0) 
      {
        //send keep alive with roll
        CAN.sendMsgBuf(KEEP_ALIVE_MSG_ID, 1, 8, keep_alive_msg);
        keep_alive_msg[0] = keep_alive_counter << 6;
        keep_alive_counter++;
        if (keep_alive_counter == 4) {
          keep_alive_counter = 0;
        }
      }
    }
  }

  // If we're not yet connected and the CAN interrupt is fired
  // check if the data coming in is for the volvo eps
  if (!eps_connected && !digitalRead(PIN_CAN_INT))
  { 
    unsigned long id;
    unsigned char length;
    unsigned char buffer[8];
    char msgString[128];

    CAN.readMsgBuf(&id, &length, buffer);

    // Determine if ID is standard (11 bits) or extended (29 bits)
    if ((id & 0x80000000) == 0x80000000)
    {
      sprintf(msgString, "%.8lX", (id & 0x1FFFFFFF));
    }

    // Determine if the extended id we recieved is the one for the Volvo EPS
    if (!strcmp(msgString, "1B200002")) 
    { 
      eps_connected = true;
    }
  }
}