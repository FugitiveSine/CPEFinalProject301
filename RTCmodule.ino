//#include <Wire.h>
//#include <RTClib.h>
// before setup RTC_DS1307 rtc
// in setup: rtc.begin()
// Run only once to set the clock then it will automatically keep track due to its battery
//rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
// TODO: could make fucntions for each state where it turns the necessary compenents on off when toggled?


void displayDateTime(){
  DateTime now = rtc.now();

  //lcd.clear();
  lcd.setCursor(0, 0); // Date on first line
  lcd.print(now.month() + String("/") + now.day() + String("/") + now.year());

  lcd.setCursor(0, 1); // Time on second line
  lcd.print(now.hour() + String(":") + now.minute() + String(":") + now.second());

}
