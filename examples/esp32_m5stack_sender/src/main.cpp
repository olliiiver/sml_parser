#include <M5Stack.h>
#include <SoftwareSerial.h>
#include "sml.h"
#include "ehz_bin.h"

// M5Stack to produce some example data on TXD2 pin (blue grove connector)
// Press button A to toggle between inverted/uninverted signal

SoftwareSerial outSerial;

int i = 0, counter = 0;
unsigned char c;
sml_states_t s;
bool invert = true;

unsigned long previousMillis = 0;
const long interval = 2000;

void setup()
{
  outSerial.begin(9600, SWSERIAL_8N1, -1, TXD2, invert, 0, 95);
  outSerial.enableRx(false);
  Serial.begin(115200);
  M5.begin();

  M5.Lcd.setCursor(1, 10);
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setTextSize(5);
  M5.Lcd.print("SML Sender");
}

void updateDisplay()
{
  M5.Lcd.fillRect(0, 50, M5.Lcd.width(), 50, WHITE);
  M5.Lcd.setCursor(1, 50);
  M5.Lcd.setTextSize(2);
  M5.Lcd.printf("Send: %i", counter);
  M5.Lcd.setCursor(1, 70);
  M5.Lcd.printf("Invert: %s", (invert) ? "Yes" : "No");
  Serial.print("Message sent\n");
}

void loop()
{
  unsigned long currentMillis = millis();
  M5.update();
  if (M5.BtnA.wasReleased())
  {
    invert = (invert) ? false : true;
    outSerial.begin(9600, SWSERIAL_8N1, -1, TXD2, invert, 0, 95);
    Serial.print("Button pressed.\n");
    updateDisplay();
  }
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    for (i = 0; i < ehz_bin_len; ++i)
    {
      c = ehz_bin[i];
      s = smlState(c);
      outSerial.write(c);
      outSerial.flush();
      if (s == SML_FINAL)
      {
        counter++;
        updateDisplay();
      }
    }
  }
}
