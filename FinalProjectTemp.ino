// Authors: Henry Timmons, Alan Rodriguez, Abraham Rodriguez Sanchez


#define RDA 0x80
#define TBE 0x20
#include <LiquidCrystal.h>
// For RTC
#include <Wire.h>
#include <RTClib.h>


// UART Pointers
volatile unsigned char *myUCSR0A  = (unsigned char *)0x00C0;
volatile unsigned char *myUCSR0B  = (unsigned char *)0x00C1;
volatile unsigned char *myUCSR0C  = (unsigned char *)0x00C2;
volatile unsigned int  *myUBRR0   = (unsigned int *)0x00C4;
volatile unsigned char *myUDR0    = (unsigned char *)0x00C6;

// Timer Pointers
volatile unsigned char *myTCCR1A  = (unsigned char *)0x80;
volatile unsigned char *myTCCR1B  = (unsigned char *)0x81;
volatile unsigned char *myTCCR1C  = (unsigned char *)0x82;
volatile unsigned char *myTIMSK1  = (unsigned char *)0x6F;
volatile unsigned char *myTIFR1   = (unsigned char *)0x36;
volatile unsigned int  *myTCNT1   = (unsigned int *)0x84;


volatile unsigned char *portA = (unsigned char *) 0x22;
volatile unsigned char *ddrA = (unsigned char *) 0x21;
volatile unsigned char *pinA = (unsigned char *) 0x20;

//FAN
volatile unsigned char *portB = (unsigned char *) 0x25;
volatile unsigned char *ddrB = (unsigned char *) 0x24;
volatile unsigned char *pinB = (unsigned char *) 0x23;

//BUTTONS PINS 31,33,35,37
volatile unsigned char *portC = (unsigned char *) 0x28;
volatile unsigned char *ddrC = (unsigned char *) 0x27;
volatile unsigned char *pinC = (unsigned char *) 0x26;


volatile unsigned char *portD = (unsigned char *) 0x2B;
volatile unsigned char *ddrD = (unsigned char *) 0x2A;
volatile unsigned char *pinD = (unsigned char *) 0x29;

// 0-3? 5?
volatile unsigned char *portE = (unsigned char *) 0x2E;
volatile unsigned char *ddrE = (unsigned char *) 0x2D;
volatile unsigned char *pinE = (unsigned char *) 0x2C;

//6-9
volatile unsigned char *portH = (unsigned char *) 0x102;
volatile unsigned char *ddrH = (unsigned char *) 0x101;
volatile unsigned char *pinH = (unsigned char *) 0x100;

//LEDS 39,41
volatile unsigned char *portG = (unsigned char *) 0x34;
volatile unsigned char *ddrG = (unsigned char *) 0x33;
volatile unsigned char *pinG = (unsigned char *) 0x32;
//LEDS 43,45
volatile unsigned char *portL = (unsigned char *) 0x10B;
volatile unsigned char *ddrL = (unsigned char *) 0x10A;
volatile unsigned char *pinL = (unsigned char *) 0x109;



byte in_char;

enum State{
  OFF,
  IDLE,
  ACTIVE,
  ERROR
};


//global ticks counter
unsigned int currentTicks = 65535;
unsigned char timer_running = 0;

const int ON_BUTTON = 2;
const int OFF_BUTTON = 3;
const int RESET_BUTTON = 18; // Can use ISR interrupt at pin 18,2,3,20,21
const int SWITCH_BUTTON = 31;

const int ACTIVE_LED = 39;
const int OFF_LED = 41;
const int IDLE_LED = 43;
const int ERROR_LED = 45;
const int FAN_PIN = 27;

State currentState = OFF;

// LCD pins <--> Arduino pins
const int RS = 10, EN = 9, D4 = 8, D5 = 7, D6 = 6, D7 = 5;
int right=0,up=0;
int dir1=0,dir2=0;

LiquidCrystal lcd(RS, EN, D4, D5, D6, D7); 
int x = 0;
int y = 0;
volatile bool onPressed = false;
volatile bool offPressed = false;
volatile bool resetPressed = false;
volatile bool switchPressed = false;
volatile bool showTempHumid = true; // Start with showing temperature and humidity
volatile int lastButtonState = HIGH;

int tempThreshold = 23; // Temp Threshold to transition IDLE <--> ACTIVE in Celcuis

volatile unsigned long lastOnInterrupt = 0;


// PROTOTYPES
void runOFF();
void runIDLE();
void runACTIVE();
void runERROR();
void displayDateTime();
void initDHT();
void U0Init(int U0baud);
void updateCurrentState(State state);
void tempHumSensorON();
float getTemp();
void setup_timer_regs();
bool getDHTError();
unsigned char kbhit();
void setDHTError(bool x);

extern volatile bool dhtError;


void onButtonISR() {
  if(debounce(lastOnInterrupt, 100)){
    onPressed = true;
  }
  
}

void offButtonISR() {
  if(debounce(lastOnInterrupt, 100)){
    offPressed = true;
  }
}

void resetButtonISR() {
  if(debounce(lastOnInterrupt, 100)){
    resetPressed = true;
  }
}

RTC_DS1307 rtc;

void setup() 
{
  rtc.begin();
  // Run only once to set the clock then it will automatically keep track due to its battery
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  initDHT(); // for temperature humidity module
  // Set up Display
  // Start the UART
  U0Init(9600);
  U0PutString("UART TEST\r\n");
  lcd.begin(16, 2); // set up number of columns and rows
  lcd.clear();
  lcd.setCursor(x, y);
  
  // Button pin modes
  // pinMode(ON_BUTTON, INPUT_PULLUP);
  *ddrE &= ~(1 << 4);   // input
  *portE |= (1 << 4);   // pullup enabled 

  //pinMode(OFF_BUTTON, INPUT_PULLUP);
  *ddrE &= ~(1 << 5); // input
  *portE |= (1 << 5); // pullup enabled

  // pinMode(RESET_BUTTON, INPUT_PULLUP);
  *ddrD &= ~(1 << 3); // input
  *portD |= (1 << 3); // pullup enabled

  // pinMode(SWITCH_BUTTON, INPUT_PULLUP);
  *ddrC &= ~(1 << 6); // input
  *portC |= (1 << 6); // pullup enabled

  // LED pin modes
  //pinMode(ACTIVE_LED, OUTPUT);
  *ddrG |= (1 << 2);

  //pinMode(OFF_LED, OUTPUT);
  *ddrG |= (1 << 0);

  // pinMode(IDLE_LED, OUTPUT);
  *ddrL |= (1 << 6);

  // pinMode(ERROR_LED, OUTPUT);
  *ddrL |= (1 << 4);

  // pinMode(FAN_PIN, OUTPUT);
  *ddrA |= (1 << 5); // fan on: portA |= (1 << 5); fan off portA &= ~(1 << 5);

attachInterrupt(digitalPinToInterrupt(ON_BUTTON), onButtonISR, FALLING);
attachInterrupt(digitalPinToInterrupt(OFF_BUTTON), offButtonISR, FALLING);
attachInterrupt(digitalPinToInterrupt(RESET_BUTTON), resetButtonISR, FALLING);

  // Turn LED's to off
  // (ACTIVE_LED, LOW);
  *portG &= ~(1 << 2);

  // (OFF_LED, LOW);
  *portG &= ~(1 << 0);

  // (IDLE_LED, LOW);
  *portL &= ~(1 << 6);

  // (ERROR_LED, LOW);
  *portL &= ~(1 << 4);

  // Make sure fan is OFF
  // (FAN_PIN, LOW);
  *portA &= ~(1 << 5);

 
  // set PB6 to output
  *ddrB |= (1 << 6); // PB6 pin 12
  // set PB6 LOW
  *portB &= ~(1 << 6);
  // setup the Timer for Normal Mode, with the TOV interrupt enabled
  setup_timer_regs();

  U0PutString("PROGRAM BEGIN!\r\n");
  updateCurrentState(currentState);
}

void loop()
{
  switch (currentState){
    case OFF:
      runOFF();
      break;
    case IDLE:
      runIDLE();
      break;
    case ACTIVE:
      runACTIVE();
      break;
    case ERROR:
      runERROR();
      break;
    default:
      break;
  }


  // if we recieve a character from serial
  if (U0kbhit()) {
    // read the character
    in_char = U0GetChar();
    // echo it back
    U0PutChar(in_char);

    // if it's the quit character
    if(in_char == 'q' || in_char == 'Q')
    {
      // set the current ticks to the max value
      currentTicks = 65535;
      // if the timer is running
      if(timer_running)
      {
        // stop the timer
        *myTCCR1B &= 0xF8;
        // set the flag to not running
        timer_running = 0;
        // set PB6 LOW (pin 12) change to whatever to disable the action thats happening
        *portB &= ~(1 << 6); // 0xBF
      }
    }
  }
} // END OF LOOP

  void runOFF(){
    lcd.clear(); // make sure nothing is displayed
      
      // OFF LED
      *portG |= (1 << 0); //(OFF_LED, HIGH);

      // LCD Displays:

      if(onPressed){ // OFF state to IDLE state
        onPressed = false;
        U0PutString("ON Button Pressed\r\n");

        currentState = IDLE; // Update state
        *portG &= ~(1 << 0); //(OFF_LED, LOW); Remember to turn LED off when leaving state
        
        updateCurrentState(currentState);
      }
  }
  void runIDLE(){
      // IDLE LED
      *portL |= (1 << 6); //(IDLE_LED, HIGH);

      // If next(reset) button pressed during off state (fix so doesnt rapidly swap in between)
     // Read current button state
      int currentButtonState = !(*pinC & (1 << 6));

      // Detect NEW button press
      if(lastButtonState == HIGH && currentButtonState == LOW){
        switchPressed = true;
      }

      // Save state for next loop
      lastButtonState = currentButtonState;

      if(switchPressed){
        switchPressed = false;
        U0PutString("Switched Screens\r\n");
        showTempHumid = !showTempHumid;
        lcd.clear();
      }

      if(showTempHumid == true){ // Show Temp/Humid
        tempHumSensorON(); // Updates and Displays T/H
      }else{ // Show Date/Time
        displayDateTime();
        updateDHT(); // Keep the temp updated in case it goes over while in date/time screen to move to ACTIVE STATE
      }
      displayPhotoRes();

      if(getTemp() > tempThreshold){ // Threshold to active state
        *portL &= ~(1 << 6); //(IDLE_LED, LOW); turn idle led off as leaving
        currentState = ACTIVE;
      }
      if(getDHTError()){
        currentState = ERROR;
      }

      // LCD Displays:
      if(offPressed){ // ACTIVE or IDLE to OFF state
        //delay(100);
        offPressed = false;
        U0PutString("OFF Button Pressed\r\n");

        currentState = OFF; // Update State
        *portL &= ~(1 << 6); // (IDLE_LED, LOW); Remember to turn LED off when leaving state
        updateCurrentState(currentState);
      }

  }
  
  void runACTIVE(){
        // ACTIVE LED
      *portG |= (1 << 2); // (ACTIVE_LED, HIGH);

      // Turn fan ON
      *portA |= (1 << 5); // (FAN_PIN, HIGH);
      if(getDHTError()){
        currentState = ERROR;
      }

      tempHumSensorON();
      // displayTempHumid();
      // displayPhotoRes();

      if(getTemp() < tempThreshold){ // Threshold to go back to IDLE state
        *portG &= ~(1 << 2); // (ACTIVE_LED, LOW); // Turn active led off as leaving
        *portA &= ~(1 << 5); // (FAN_PIN, LOW); Turn fan off when leaving
        currentState = IDLE;
      }

      // LCD Displays:

      if(offPressed && !(*pinE & (1 << 5))){ // ACTIVE or IDLE to OFF state
        offPressed = false;
        U0PutString("OFF Button Pressed\r\n");

        *portA &= ~(1 << 5); // (FAN_PIN, LOW);
        *portG &= ~(1 << 2); // (ACTIVE_LED, LOW); Remember to turn LED off when leaving state
        currentState = OFF; // Update State
        updateCurrentState(currentState);
      }

  }
  void runERROR(){
     // ERROR LED
     //updateCurrentState(currentState);
      // Turn off all other LEDS error mightve originated from
      *portG &= ~(1 << 2); //(ACTIVE_LED, LOW);
      *portL &= ~(1 << 6); // (IDLE_LED, LOW);
      *portA &= ~(1 << 5); //(FAN_PIN, LOW);

      *portL |= (1 << 4); //(ERROR_LED, HIGH);

      // LCD Displays:
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("ERROR");

      if(resetPressed){ // If in ERROR state to IDLE state
        resetPressed = false;
        dhtError = false; // Reset error variable back to original state
        U0PutString("RESET Button Pressed\r\n");

        currentState = IDLE; // Update State
        *portL &= ~(1 << 4); // (ERROR_LED, LOW); Remember to turn LED off when leaving state
        updateCurrentState(currentState);

      }
  }

bool debounce(volatile unsigned long &lastTime, unsigned long interval)
{
    unsigned long currentTime = millis();

    if(currentTime - lastTime >= interval)
    {
        lastTime = currentTime;
        return true;
    }

    return false;
}

void tempHumSensorON(){
    updateDHT();
    displayTempHumid(); // get readings from the DHT11 sensor to the LCD
}

// Timer setup function
void setup_timer_regs()
{
  // setup the timer control registers
  *myTCCR1A= 0x00;
  *myTCCR1B= 0X00;
  *myTCCR1C= 0x00;
  
  // reset the TOV flag
  *myTIFR1 |= 0x01; // ??
  
  // enable the TOV interrupt
  *myTIMSK1 |= 0x01; // ??
}


// TIMER OVERFLOW ISR
ISR(TIMER1_OVF_vect)
{
  // Stop the Timer
  *myTCCR1B &= 0xF8; // ??
  // Load the Count
  *myTCNT1 =  (unsigned int) (65535 -  (unsigned long) (currentTicks));
  // Start the Timer
  *myTCCR1B |= 0x01; // ??
  // if it's not the STOP amount
  if(currentTicks != 65535)
  {
    // XOR to toggle PB6
    *portB ^= 0x40; // (0x40)
  }
}
void U0Init(int U0baud){
  unsigned long FCPU = 16000000;
 unsigned int tbaud;
 tbaud = (FCPU / 16 / U0baud - 1);
 // Same as (FCPU / (16 * U0baud)) - 1;
 *myUCSR0A = 0x20;
 *myUCSR0B = 0x18;
 *myUCSR0C = 0x06;
 *myUBRR0  = tbaud;

 
}
unsigned char U0kbhit(){
  return *myUCSR0A & RDA;
}
unsigned char U0GetChar(){
  return *myUDR0;
}
void U0PutChar(unsigned char U0pdata){
  while((*myUCSR0A & TBE) == 0);
  *myUDR0 = U0pdata;
}
void U0PutString(const char *str){ // Take a string
  // Timestamp every output to the serial monitor
    DateTime now = rtc.now();
    U0PutChar('[');

    // Hour
    if(now.hour() < 10) U0PutChar('0');
    U0PutChar(now.hour() / 10 + '0');
    U0PutChar(now.hour() % 10 + '0');

    U0PutChar(':');

    // Minute
    if(now.minute() < 10) U0PutChar('0');
    U0PutChar(now.minute() / 10 + '0');
    U0PutChar(now.minute() % 10 + '0');

    U0PutChar(':');

    // Second
    if(now.second() < 10) U0PutChar('0');
    U0PutChar(now.second() / 10 + '0');
    U0PutChar(now.second() % 10 + '0');

    U0PutChar(']');
    U0PutChar(' ');
  
 while(*str != '\0'){ // Iterates through string until hit the end of the string
        U0PutChar(*str); // Put each char in the string to the existing char printing function
        str++;
    }
    //U0PutChar('\n'); // Send a new line character after the string
}

void updateCurrentState(State state){
  switch (state) {
    case OFF:
      U0PutString("State: OFF\r\n");
      break;

    case IDLE:
      U0PutString("State: IDLE\r\n");
      break;

    case ACTIVE:
      U0PutString("State: ACTIVE\r\n");
      
      break;

    case ERROR:
      U0PutString("State: ERROR\r\n");
      break;
  }
}

