const int photoPin = A0;

unsigned long lastLightRead = 0;
void displayPhotoRes(){

  // 300 around normal light, flashlight 700-800

  if(millis() - lastLightRead > 2000){ // update every 6sec (millis is unsigned long so need to match otherwise ing overflow for lastLightRead)
    lastLightRead = millis();
    int lightValue = readA0(); // Read Light after 2 seconds
    if(lightValue > 600){
      U0PutString("Power: Solar \r\n");
    
    }else{
      U0PutString("Power: Battery \r\n");
    }
  }
}

int readA0(){
    ADMUX = (ADMUX & 0xF0) | 0;   // select A0
    ADCSRA |= (1 << ADSC);        // start conversion
    while (ADCSRA & (1 << ADSC)); // wait
    return ADC;                   // result
}