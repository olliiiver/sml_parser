#include "Arduino.h"
#include <SPI.h>
#include <U8g2lib.h>

#include "sml.h"
#include <SoftwareSerial.h>

#define rxPin 36

#define MAX_STR_MANUF 5
unsigned char manuf[MAX_STR_MANUF];
double T1Wh = -2, SumWh = -2;

typedef struct
{
  const unsigned char OBIS[6];
  void (*Handler)();
} OBISHandler;

void Manufacturer()
{
  smlOBISManufacturer(manuf, MAX_STR_MANUF);
}

void PowerT1()
{
  smlOBISWh(T1Wh);
}

void PowerSum()
{
  smlOBISWh(SumWh);
}

OBISHandler OBISHandlers[] = {
    {{0x81, 0x81, 0xc7, 0x82, 0x03, 0xff}, &Manufacturer}, /* 129-129:199.130.3*255 */
    {{0x01, 0x00, 0x01, 0x08, 0x01, 0xff}, &PowerT1},      /*   1-  0:  1.  8.1*255 (T1) */
    {{0x01, 0x00, 0x01, 0x08, 0x00, 0xff}, &PowerSum},     /*   1-  0:  1.  8.0*255 (T1 + T2) */
    {{0}, 0}
};

SoftwareSerial inputSerial;

U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/15, /* data=*/4, /* reset=*/16);

sml_states_t currentState;
unsigned char currentChar = 0;
unsigned long counter = 0;
char buffer[50];
char floatBuffer[20];

void updateDisplay()
{
  sprintf(buffer, "Msg..: %lu", counter);
  u8x8.drawString(0, 2, buffer);
  sprintf(buffer, "Manuf: %s", manuf);
  u8x8.drawString(0, 3, buffer);

  dtostrf(T1Wh, 10, 1, floatBuffer);
  sprintf(buffer, "T1.: %s", floatBuffer);
  u8x8.drawString(0, 4, buffer);

  dtostrf(SumWh, 10, 1, floatBuffer);
  sprintf(buffer, "Sum: %s", floatBuffer);
  u8x8.drawString(0, 5, buffer);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting");

  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 1, "SML Receiver");

  pinMode(rxPin, INPUT);
  inputSerial.begin(9600, SWSERIAL_8N1, rxPin, -1, false, 0, 95);
  inputSerial.enableRx(true);
  inputSerial.enableTx(false);
  updateDisplay();
}

void readByte()
{
  unsigned int iHandler = 0;
  currentState = smlState(currentChar);
  if (currentState == SML_START) 
  {
      /* reset local vars */
      manuf[0] = 0; T1Wh = -3; SumWh = -3;
  }
  if (currentState == SML_LISTEND)
  {
    /* check handlers on last received list */
    for (iHandler = 0; OBISHandlers[iHandler].Handler != 0 &&
                       !(smlOBISCheck(OBISHandlers[iHandler].OBIS));
         iHandler++);
    if (OBISHandlers[iHandler].Handler != 0)
    {
      OBISHandlers[iHandler].Handler();
    }
  }
  if (currentState == SML_UNEXPECTED)
  {
    Serial.print(F(">>> Unexpected byte!\n"));
  }
  if (currentState == SML_FINAL)
  {
    updateDisplay();
    counter++;
    Serial.print(F(">>> Successfully received a complete message!\n"));
  }
}

void loop()
{
  if (inputSerial.available())
  {
    currentChar = inputSerial.read();
    readByte();
  }
}
