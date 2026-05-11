#include <DHT.h>

#define DHTPIN 23        // Change this to the pin you connected DATA to
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
float humidity,h = 0;
float temperature,t = 0;
volatile bool dhtError = false;

void initDHT(){
  dht.begin();
}

// dht.begin(); in main setup()
bool getDHTError(){
  return dhtError;
}

unsigned long lastDHTRead = 0;
void updateDHT(){
  if(millis() - lastDHTRead > 2000){ // 2sec between reading data
        lastDHTRead = millis();

        t = dht.readTemperature();
        h = dht.readHumidity();
        U0PutString("Updated Temp/Humid\r\n");

    if (!isnan(h) && !isnan(t)) { // If theres a value update the vars
      humidity = h;
      temperature = t;
    }else{// If theres not a value then thats an error
      U0PutString("Failed to read from DHT11!\r\n");
      dhtError = true;
    }
  }

}

float getHum(){
  return humidity;
}

float getTemp(){
  return temperature;
}


void displayTempHumid(){

    lcd.setCursor(0,0); // Go to bottom row to display info

    lcd.print("Temp:");
    lcd.print(getTemp(), 1);
    lcd.print("C ");

    lcd.setCursor(0,1);
    lcd.print("Humid:");
    lcd.print(getHum(), 1);
    lcd.print("%   "); // extra spaces clear leftover chars
 
}