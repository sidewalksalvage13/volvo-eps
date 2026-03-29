#include "mcp_can.h"
#include <SPI.h>
#include <EEPROM.h>

#define PIN_CAN_CS    10
#define PIN_CAN_INT   2
#define MAX_EPS_SPEED 6000

#define PIN_JOY_UP    A1
#define PIN_JOY_LEFT  A2
#define PIN_JOY_DOWN  A3
#define PIN_JOY_ENTER A4
#define PIN_JOY_RIGHT A5
#define DEBOUNCE_MS   50
#define EEPROM_SPEED_ADDR 0

const int send_interval_ms = 15;
const unsigned long EPS_SPEED_MSG_ID  = 0x2104136;
const unsigned long KEEP_ALIVE_MSG_ID = 0x1AE0092C;

unsigned char eps_speed_msg[8]  = { 0xBB, 0x00, 0x3F, 0xFF, 0x06, 0xE0, 0x00, 0x00 };
unsigned char keep_alive_msg[8] = { 0x00, 0x00, 0x22, 0xE0, 0x41, 0x90, 0x00, 0x00 };

uint8_t eps_speed_counter  = 0;
uint8_t keep_alive_counter = 0;

unsigned long previous_time_ms = 0;
bool eps_connected = false;
uint16_t eps_speed = 0;

unsigned long joy_last_ms_up    = 0;
unsigned long joy_last_ms_down  = 0;
unsigned long joy_last_ms_left  = 0;
unsigned long joy_last_ms_right = 0;
unsigned long joy_last_ms_enter = 0;

uint8_t joy_state_up    = HIGH;
uint8_t joy_state_down  = HIGH;
uint8_t joy_state_left  = HIGH;
uint8_t joy_state_right = HIGH;
uint8_t joy_state_enter = HIGH;

MCP_CAN CAN(PIN_CAN_CS);

void handleButton(uint8_t pin, uint8_t &state, unsigned long &last_ms, unsigned long current_time_ms, void (*action)())
{
  uint8_t reading = digitalRead(pin);
  if (reading != state && current_time_ms - last_ms >= DEBOUNCE_MS)
  {
    last_ms = current_time_ms;
    state = reading;
    if (reading == LOW)
      action();
  }
}

void handleJoystick(unsigned long current_time_ms)
{
  handleButton(PIN_JOY_UP, joy_state_up, joy_last_ms_up, current_time_ms, []() {
    eps_speed = (eps_speed + 1000 > MAX_EPS_SPEED) ? MAX_EPS_SPEED : eps_speed + 1000;
    Serial.print("Up: speed=");
    Serial.println(eps_speed);
  });

  handleButton(PIN_JOY_DOWN, joy_state_down, joy_last_ms_down, current_time_ms, []() {
    eps_speed = (eps_speed < 1000) ? 0 : eps_speed - 1000;
    Serial.print("Down: speed=");
    Serial.println(eps_speed);
  });

  handleButton(PIN_JOY_LEFT, joy_state_left, joy_last_ms_left, current_time_ms, []() {
    eps_speed = 0;
    Serial.print("Left: speed=0\r\n");
  });

  handleButton(PIN_JOY_RIGHT, joy_state_right, joy_last_ms_right, current_time_ms, []() {
    eps_speed = MAX_EPS_SPEED;
    Serial.print("Right: speed=6000\r\n");
  });

  handleButton(PIN_JOY_ENTER, joy_state_enter, joy_last_ms_enter, current_time_ms, []() {
    EEPROM.put(EEPROM_SPEED_ADDR, eps_speed);
    Serial.print("Enter: saved speed=");
    Serial.println(eps_speed);
  });
}

void setup()
{
  Serial.begin(115200);

  pinMode(PIN_CAN_INT, INPUT);

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

  pinMode(PIN_JOY_UP,    INPUT_PULLUP);
  pinMode(PIN_JOY_LEFT,  INPUT_PULLUP);
  pinMode(PIN_JOY_DOWN,  INPUT_PULLUP);
  pinMode(PIN_JOY_ENTER, INPUT_PULLUP);
  pinMode(PIN_JOY_RIGHT, INPUT_PULLUP);

  uint16_t saved_speed;
  EEPROM.get(EEPROM_SPEED_ADDR, saved_speed);
  eps_speed = (saved_speed <= MAX_EPS_SPEED) ? saved_speed : 0;
  Serial.print("EEPROM speed=");
  Serial.println(eps_speed);
}

void loop()
{
  unsigned long current_time_ms = millis();

  handleJoystick(current_time_ms);

  if (eps_connected)
  {
    if (current_time_ms - previous_time_ms >= send_interval_ms)
    {
      previous_time_ms = current_time_ms;

      eps_speed_msg[6] = eps_speed >> 8;
      eps_speed_msg[7] = eps_speed;

      CAN.sendMsgBuf(EPS_SPEED_MSG_ID, 1, 8, eps_speed_msg);

      if (++eps_speed_counter == 30)
      {
        eps_speed_counter = 0;
        keep_alive_counter = (keep_alive_counter + 1) % 4;
        keep_alive_msg[0] = keep_alive_counter << 6;
        CAN.sendMsgBuf(KEEP_ALIVE_MSG_ID, 1, 8, keep_alive_msg);
      }
    }
  }

  if (!eps_connected && !digitalRead(PIN_CAN_INT))
  {
    unsigned long id;
    unsigned char length;
    unsigned char buffer[8];

    CAN.readMsgBuf(&id, &length, buffer);

    if ((id & 0x80000000) && (id & 0x1FFFFFFF) == 0x1B200002)
    {
      eps_connected = true;
    }
  }
}
