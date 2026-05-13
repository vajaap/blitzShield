#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <LiquidCrystal_I2C.h>

//-------------------------------------

HardwareSerial DalySerial(2);

float currentBMS = 0;
float voltageBMS = 0;
int tempBMS = 0;
SemaphoreHandle_t bmsMutex;

uint8_t response[13];

void sendCommand(uint8_t cmd) {

  uint8_t packet[13] = {
    0xA5, 0x40, cmd, 0x08,
    0,0,0,0,0,0,0,0,
    0
  };

  uint8_t checksum = 0;
  for (int i = 0; i < 12; i++) checksum += packet[i];
  packet[12] = checksum;

  while (DalySerial.available()) DalySerial.read();
  DalySerial.write(packet, 13);
}

bool readFrame() {

  unsigned long start = millis();

  while (DalySerial.available() < 13) {
    if (millis() - start > 1000) return false;
  }

  for (int i = 0; i < 13; i++) {
    response[i] = DalySerial.read();
  }

  return true;
}

void bmsTask(void * parameter) {

  while (true) {

    float v = 0, c = 0, t = 0;

    // -------- VOLTAGE + CURRENT --------
    sendCommand(0x90);

    if (readFrame()) {
      v = ((response[4] << 8) | response[5]) / 10.0;
      c = (((response[8] << 8) | response[9]) - 30000) / 10.0;
    }

    vTaskDelay(150 / portTICK_PERIOD_MS);

    // -------- TEMPERATURE --------
    sendCommand(0x92);

    if (readFrame()) {
      t = response[4] - 40;
    }

    vTaskDelay(150 / portTICK_PERIOD_MS);

    // -------- SAFE UPDATE --------
    xSemaphoreTake(bmsMutex, portMAX_DELAY);

    voltageBMS = v;
    currentBMS = c;
    tempBMS = (int)t;

    xSemaphoreGive(bmsMutex);

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
//---------------------------------------------------
LiquidCrystal_I2C lcd(0x27, 20, 4);
U8G2_ST7567_ENH_DG128064I_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

// The array containing your 4 temperature values
int tempLED[4] = {22, 25, 30, 18}; 
   int lm1000;
  int lm500;



void setup() {
  
  DalySerial.begin(9600, SERIAL_8N1, 16, 17);

  bmsMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    bmsTask,
    "BMS_TASK",
    4096,
    NULL,
    1,
    NULL,
    0  
  );
//---------------------------------------------------

  
 
   Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.print("BATTERY INFO");

  u8g2.setI2CAddress(0x3F * 2); 
  u8g2.begin();
  u8g2.setContrast(225); 

  Serial.begin(115200);
}

void loop() {

  if(lm1000!=millis()/1000)
  {
    lm1000=millis()/1000;
    lcd.setCursor(0, 1);
    lcd.print("Voltage:" + String(voltageBMS)+" V");
    lcd.setCursor(0, 2);
    lcd.print("Current:" + String(currentBMS)+" A");
    lcd.setCursor(0, 3);
    lcd.print("Temprature:" + String(tempBMS)+" C");
  
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_fub30_tn   );
    u8g2.setCursor(0, 31);
    u8g2.print(tempLED[0]);
    u8g2.setCursor(78,31);
    u8g2.print(tempLED[1]);
    u8g2.setCursor(0, 65);
    u8g2.print(tempLED[2]);
    u8g2.setCursor(78, 65);
    u8g2.print(tempLED[3]);
    u8g2.sendBuffer();
  }
}
