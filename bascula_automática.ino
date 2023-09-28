
  

/*
   -------------------------------------------------------------------------------------
   HX711_ADC
   Arduino library for HX711 24-Bit Analog-to-Digital Converter for Weight Scales
   Olav Kallhovd sept2017
   -------------------------------------------------------------------------------------
*/

/*
   Settling time (number of samples) and data filtering can be adjusted in the config.h file
   For calibration and storing the calibration value in eeprom, see example file "Calibration.ino"

   The update() function checks for new data and starts the next conversion. In order to acheive maximum effective
   sample rate, update() should be called at least as often as the HX711 sample rate; >10Hz@10SPS, >80Hz@80SPS.
   If you have other time consuming code running (i.e. a graphical LCD), consider calling update() from an interrupt routine,
   see example file "Read_1x_load_cell_interrupt_driven.ino".

   This is an example sketch on how to use this library
*/



#include <SD.h>
#include <LiquidCrystal_I2C.h>
#include<DHT.h>
#include <Wire.h>   // incluye libreria para interfaz I2C
#include <RTClib.h>   // incluye libreria para el manejo del modulo RTC

#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

//pins:
const int HX711_dout = 7; //mcu > HX711 dout pin
const int HX711_sck = 8; //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

LiquidCrystal_I2C lcd(0x27,16,2);
RTC_DS3231 rtc;     // crea objeto del tipo RTC_DS3231


const int calVal_eepromAdress = 0;
unsigned long t = 0;

void setup() {

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);

  if (! rtc.begin()) {       // si falla la inicializacion del modulo
 Serial.println("Modulo RTC no encontrado !");  // muestra mensaje de error
 while (1);         // bucle infinito que detiene ejecucion del programa
 }

//rtc.adjust(DateTime(__DATE__, __TIME__));  // funcion que permite establecer fecha y horario
   // al momento de la compilacion. Comentar esta linea

 
    
  Serial.begin(57600); delay(10);
  Serial.println();
  Serial.println("Starting...");



  
  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  float calibrationValue; // calibration value (see example file "Calibration.ino")
  calibrationValue = -394.58
  ;  // uncomment this if you want to set the calibration value in the sketch  // -1829.14 alta temperatura
#if defined(ESP8266)|| defined(ESP32)
  //EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
#endif
  //EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }

  if (!SD.begin(10)) {
    Serial.println("Falla en tarjeta o no presente");
    // don't do anything more:
    while (1);
  }
  Serial.println("configurando tarj. sd.");
  
  File dataFile = SD.open("datalog2.txt", FILE_WRITE);  
  if (dataFile) {  }
  else {
    Serial.println("error en archivo datalog2.txt");

  }
  
}

void loop() {

  static boolean newDataReady = 0;
  const int serialPrintInterval = 10000; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(i);



      File dataFile = SD.open("datalog2.txt", FILE_WRITE);
       if (dataFile) {
        DateTime fecha = rtc.now(); 
        float t = rtc.getTemperature();

        float i_adjust;

        if((fecha.hour()>9)&&(fecha.hour()<18)){
        //algoritmo día
        float a = 0.9735;
        float b = -14.0292;
        float R = a*t+b;
        i_adjust = i+R;
        }
        //algoritmo noche
        else { float a = 1.00;
        float b = -11.0757;
        float R = a*t+b;
        i_adjust = i+R;
        }
            
        dataFile.print(fecha.day());
        dataFile.print("/");        
        dataFile.print(fecha.month());
        dataFile.print("/");
        dataFile.print(fecha.year()); // Se puede cambiar entre dÃ­a y mes si se utiliza el sistema Americano
        dataFile.print(", ");
        dataFile.print(fecha.hour());
        dataFile.print(":");
        dataFile.print(fecha.minute());
        dataFile.print(":");
        dataFile.print(fecha.second());
        dataFile.print(" ,");        
        dataFile.print("mo: "); //masa ajustada
        dataFile.print(i);        
        dataFile.print(" ,ma: "); //masa ajustada
        dataFile.print(i_adjust);
        dataFile.print(" ,T: ");
        dataFile.println(t);
        dataFile.close();
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(fecha.hour());
        lcd.print(":");
        lcd.print(fecha.minute());
        lcd.print(":");
        lcd.print(fecha.second());
        lcd.print(" T:");
        lcd.print(t);    
        lcd.setCursor(0,1);
        lcd.print("mo:");
        lcd.print(i);
        lcd.print("ma:");
        lcd.print(i_adjust);
            
       }
             
      newDataReady = 0;
      t = millis();
    }
  }

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }

}

//void fecha_hora(){
  
/*  myRTC.updateTime();

  lcd.setCursor (0,0);
  //lcd.print(myRTC.year);
  //lcd.print("/");
  lcd.print(myRTC.dayofmonth);
  lcd.print("/");
  lcd.print(myRTC.month);
  lcd.print(" ");
  lcd.print(myRTC.hours);
  lcd.print(":");
  lcd.print(myRTC.minutes);*/

/*  File dataFile = SD.open("datalog2.txt", FILE_WRITE);
  if (dataFile) {
    myRTC.updateTime();
    dataFile.print(myRTC.hours);
    dataFile.print(":");
    dataFile.print(myRTC.minutes);
    dataFile.print(" ");
    dataFile.print(myRTC.dayofmonth);
    dataFile.print("/");
    dataFile.print(myRTC.month);
    dataFile.print("/");
    dataFile.print(myRTC.year); // Se puede cambiar entre dÃ­a y mes si se utiliza el sistema Americano
    dataFile.print("/ ");
}
}
  
/*
  lcd.setCursor (0,1);
  lcd.print(t);
  lcd.print(" C ");
  lcd.print(h);
  lcd.print(" % ");
  lcd.display();*/
   

/*void temperatura(){
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    lcd.setCursor (0,0);
    lcd.print("Error en sensor DHT11");
    return;
  }
  
  lcd.setCursor (10,0);
  lcd.print(t);
  lcd.print("C");

  File dataFile = SD.open("datalog2.txt", FILE_WRITE);
  if (dataFile) {
    dataFile.print(t);
    dataFile.print(", C, ");
    dataFile.print(h);
    dataFile.println(" %");
    dataFile.close();
  }
*/
