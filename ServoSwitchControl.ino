#include <Time.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Servo.h> 
 
Servo myservo;

int pin_servo = 2;
int pins_force_ground[] = {5,8,11};
int num_pins_force_ground = sizeof(pins_force_ground)/sizeof(*pins_force_ground);
int pin_b1 = 3;
int pin_b2 = 6;
int pin_b3 = 9;


int angle_neutral = 90;
int angle_on = 180;
int angle_off = 0;

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 32)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

// text size 1 is 8x6, so can fit 21x4 chars

char* pad2(int v, char pad='0') {
  static char buf[3];
  if (v<0) {
    buf[0] = '-';
    buf[1] = '-';
  } else if (v<10) {
    buf[0] = pad;
    buf[1] = v+'0';
  } else if (v<100){
    buf[0] = (v/10)+'0';
    buf[1] = (v%10)+'0';
  } else {
    buf[0] = '+';
    buf[1] = '+';
  }
  buf[2] = '\0';
  return buf;
}


char* format_time(boolean with_dow_abbrev=false, boolean with_seconds=false) {
  static char buf[24];
  buf[0] = '\0';
  if (with_dow_abbrev) {
    strcat(buf, weekday_abbrev());
    //strcat(buf, " ");
  }
  strcat(buf, pad2(hourFormat12(),' ')); 
  strcat(buf, ":"); 
  strcat(buf, pad2(minute(),'0')); 
  if (with_seconds) {
    strcat(buf, ":"); 
    strcat(buf, pad2(second(),'0')); 
    strcat(buf, isAM() ? "a" : "p");
  }
  return buf;
}

void set_switch(boolean state) {
  myservo.attach(pin_servo);
  int angle = state ? angle_on : angle_off;
  display.fillRect(0, 0, 128, 8, BLACK);
  display.setCursor(0,0); 
  display.print("Last: ");
  display.print(state ? "On " : "Off "); 
  display.display();
  
  Serial.print(format_time(true,true));
  Serial.println(state ? "Turning ON" : "Turning OFF");
  myservo.write(angle);
  delay(500);
  set_servo_neutral();
  display.print(format_time(true,true)); display.display();
}

void set_servo_neutral() {
  if (!myservo.attached()) myservo.attach(pin_servo);
  myservo.write(angle_neutral);
  delay(500);
  myservo.detach();
}

void turn_on()  { set_switch(1); }
void turn_off() { set_switch(0); }

boolean clock_valid = false;


void setup()   {                
  Serial.begin(9600);

  for (int i=0; i<num_pins_force_ground; i++) {
    pinMode(pins_force_ground[i],OUTPUT);
    digitalWrite(pins_force_ground[i],0);
  }
  pinMode(pin_b1,INPUT_PULLUP);
  pinMode(pin_b2,INPUT_PULLUP);
  pinMode(pin_b3,INPUT_PULLUP);

  const int start_h = 0;
  const int start_m = 0;
  const int start_s = 0;
  setTime(32L*24*60*60 + start_h*60*60 + start_m*60 + start_s); // set it to Jan-30 at 12am (for no reason -- any 12am is fine)

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);

  Serial.println("Resetting servo...");
  display.clearDisplay(); display.println("Resetting servo..."); display.display();
  set_servo_neutral();

  display.setCursor(0,0); display.println("Last: none        "); display.display();

}

char* weekday_name() {
  char* dow_names[] = {"INV_DAY","Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
  return dow_names[weekday()];
}

char* weekday_abbrev() {
  char* dow_abbrev[] = {"D?", "Su","Mo","Tu","We","Th","Fr","Sa"};
  return dow_abbrev[weekday()];
}

int sel=0;

int state_b1=1;
int state_b2=1;
int state_b3=1;

void loop() {
  char* dow_names[] = {"INV_DAY","Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
  if (!clock_valid && millis()%1000<500) {
    display.fillRect(0, 8, 128, 24, BLACK);
  } else {
    display.setCursor(0,8); 
    display.setTextSize(2);
    display.print(format_time(false,true));
    display.setTextSize(1);
    display.setCursor(0,24); 
    display.print(weekday_name()); 
    display.print("    "); 
  }
  display.display();

  int wday = weekday();
  int h = hour();
  int m = minute();
  int s = second();
  if (dowMonday <= wday && wday <= dowFriday && h==8 && m==0 && s==0) {
    turn_on(); delay(1000);
  }
  if (dowMonday <= wday && wday <= dowFriday && h==17 && m==0 && s==0) {
    turn_off(); delay(1000);
  }

  int new_state_b1 = digitalRead(pin_b1);
  int new_state_b2 = digitalRead(pin_b2);
  int new_state_b3 = digitalRead(pin_b3);
  if (state_b1==1 && new_state_b1==0) {
    clock_valid = true;
    sel = (sel+1) % 5;
    display.setCursor(128-8*5,24);
    if (sel==0) display.print("     ");
    if (sel==1) display.print(">HOUR");
    if (sel==2) display.print(">MIN ");
    if (sel==3) display.print(">SEC ");
    if (sel==4) display.print(">DAY ");
    display.display();
  }
  if (new_state_b2==0) {
    if (sel==1) adjustTime(-60*60);
    if (sel==2) adjustTime(-60);
    if (sel==3) adjustTime(-1);
    if (sel==4) adjustTime(-24L*60*60);
  } else if (new_state_b3==0) {
    if (sel==1) adjustTime(+60*60);
    if (sel==2) adjustTime(+60);
    if (sel==3) adjustTime(+1);
    if (sel==4) adjustTime(+24L*60*60);
  }
  state_b1 = new_state_b1;
  state_b2 = new_state_b2;
  state_b3 = new_state_b3;

  delay(50);
  
}

