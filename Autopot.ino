#include <LiquidCrystal.h>
#include "Adafruit_seesaw.h"

//Adafruit sensor
Adafruit_seesaw ss;
float tempC;
uint16_t capread;

//soil sensor variables
const uint16_t SEN_MIN = 0;
const uint16_t SEN_MAX = 1023;

//Setpoint for manual soil sensor, will be set by user and -1 is unset
uint16_t drySet = NULL; 
uint16_t wetSet = NULL;
bool soilSet = false;

//LCD pins
const int rs = 2, en = 3, d4 = 7, d5 = 5, d6 = 6, d7 = 4;

//LED and valve pins
const int red_pin=9, grn_pin=10, blu_pin=11, vlv_pin=8;
bool vlv_state = false;

//button
const int button = 12;
int buttonState = 0;
bool buttonFlag = false;
unsigned long buttonPress, buttonRelease;

//iterator to cycle through screens 
int scrnItr = -1;
const int SCREEN_QTY = 10;
bool changeRequired = false; //tracks whether a state change occured for a screen and the lcd needs to be cleared.

//prevents the screen from cycling when an action is performed
bool noCycle = false;

//runtime timers, will capture milliseconds
unsigned long timer, lastCycle, cycleDue, tempDisplay;

//LCD screen object
LiquidCrystal lcd (rs, en, d4, d5, d6, d7);

//screen string containers
char *row0 = "    AUTOPOT!    ";
char *row1 = " EDDIES DREAM PP";

//sample container
const uint16_t SMPL_CT = 60;
uint16_t samples[SMPL_CT];

void setup() {
  // Pins:
 pinMode(red_pin, OUTPUT);
 pinMode(vlv_pin, OUTPUT);
 pinMode(red_pin, OUTPUT);
 pinMode(grn_pin, OUTPUT);
 pinMode(blu_pin, OUTPUT);
 pinMode(button, INPUT);
 Serial.begin(9600);

if (!ss.begin(0x36)) {
    //Serial.println("Fuuuuck! seesaw not found");
    while(1);
  } else {
    //Serial.print("seesaw started! version: ");
    //Serial.println(ss.getVersion(), HEX);
  }

 lcd.begin(16,2);
 lcd.setCursor(0,0);
 lcd.print("    AUTOPOT!    ");
 lcd.setCursor(0,1);
 lcd.print(" EDDIES DREAM PP");

 for (int x = 0, phaseDur = 250, blinkLen= 3000; x<blinkLen;x=x+(2*phaseDur)){
  RGB_color(25,0,0);
  delay(phaseDur);
  RGB_color(0,0,0);
  delay(phaseDur);
 }

 lastCycle = millis();
 timer = millis();
 
 //sets a 5-minute cycle
 cycleDue = lastCycle + 1000*60*5;
 RGB_color(0,0,25);
 CycleScreen();
}

int looper = 0;

void loop() {
delay(250);
ReadSensor();
ReadButton();
DisplayScreen(scrnItr); 

while (buttonState == HIGH){
    ReadButton();
    delay(50);
    ButtonDelayCheck(buttonPress);
   // DisplayScreen(scrnItr);
    if(buttonState == LOW && !noCycle){
      CycleScreen();
    }   
}
 
  //Checks if action was performed on screen
  if(noCycle && buttonRelease > buttonPress)
  {noCycle = false;}
 
  looper = (looper+1)%4;

  if (looper == 0 && soilSet){
    AutoWater();
  }
 }

void ReadSensor(void){
    tempC = ss.getTemp();
    capread = ss.touchRead(0);
}
//Changes an LED based on RGB values 0 to 255
void RGB_color(int red, int green, int blue){
  analogWrite(red_pin, red);
  analogWrite(grn_pin, green);
  analogWrite(blu_pin, blue);
}

//Valve cycle sequence
void ValveOpen(int duration){
  digitalWrite(vlv_pin, HIGH); 
  vlv_state = true;
  delay(10);
  
  //suppresses screen cycle during MeasureSoil()
  if (!noCycle){
    DisplayScreen(scrnItr);  
  }

//indefinite open
  while(duration <= 0 && buttonState == HIGH){
    RGB_color(25,0,0);
    delay(250);
    RGB_color(0,0,0);
    delay(250);
    ReadButton();
  }

  //timed open
  if(duration>0){
    for (int x = 0; x < duration; x = x+500){
      RGB_color(25,0,0);
      delay(250);
      RGB_color(0,0,0);
      delay(250);
    }
  }
}

void ValveClose (void){
 digitalWrite(vlv_pin, LOW);
 delay(10);
 vlv_state = false;
 RGB_color(0,0,25); 
}

//checks whether a cycle is due
bool checkCycle(void){
  if (timer >= cycleDue)
  {return true;}
  else
  {return false;}  
}

//current screen ct#10
void DisplayScreen(int x){
  if(changeRequired == true){
    lcd.clear();
    changeRequired = false;
  }
  switch (x) {
    case 0:    // Live Sensor readings
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.setCursor(6,0);
      lcd.print((int)tempC);
      lcd.print("C ");
      lcd.print((int)((float)tempC * (9/5) + 32));
      lcd.print("F");
      lcd.setCursor(0,1);
      lcd.print("Moisture: ");
      lcd.setCursor(11,1);
      lcd.print(capread);
      break;
      
    case 1:    //valve status and manual open
      lcd.setCursor(0, 0);
      lcd.print("  Valve Status  ");
    
      lcd.setCursor(0,1);
      
      //get valve state
      if (vlv_state == true){
        lcd.print("    <<Open>>    ");
      }
      else{
        lcd.print("  <<Closed>>   ");
      }
      break;
      
    case 2: //Soil sensor not calibrated screen   
    if(!soilSet){
      lcd.setCursor(0, 0);
      lcd.print("NO SOIL SETPOINT");
      lcd.setCursor(0,1);
      lcd.print("HOLD TO SET     ");
    }

    else if (drySet && !wetSet && !soilSet){
      lcd.setCursor(0, 0);
      lcd.print("Wetpoint: ");
      lcd.setCursor(9,1);
      lcd.print(wetSet);
      lcd.setCursor(0,1);
      lcd.print("Drypoint: N/A");
    }
    else{
      //TODO 
      lcd.setCursor(0, 0);
      lcd.print("Wetpoint: ");
      lcd.setCursor(9,1);
      lcd.print(wetSet);
      lcd.setCursor(0,1);
      lcd.print("Drypoint: ");
      lcd.setCursor(9,1);
      lcd.print(drySet);
    }
      break;
      
    case 3: //Soil sensor Status screen   
      lcd.setCursor(0, 0);
      lcd.print("SOIL SENSOR:    ");
      lcd.setCursor(12,0);
      lcd.print(capread);
      
      //Graph
      //Open box (char) 219
      //Filled box (char) 255

      lcd.setCursor(0,1);
      
      //lEtS mAkE a GrApH
      for (int x = 0, y = -1; x < 16; x++, y=y+64)
      {
        lcd.setCursor(x,1);
        if (drySet > y && drySet <= y + 64){
          lcd.print("L");
          lcd.noBlink();
        }
        if (wetSet > y && wetSet <= y+64){
          lcd.print("W");
          lcd.noBlink();
        }
        if (capread > y + 64){
          lcd.print((char) 255);
          lcd.noBlink(); 
        }
        if (capread < y){
          lcd.print((char) 219);
          lcd.noBlink();
        } 
        if(capread > y && capread <= y + 64){
          lcd.blink();
        }
      }
      break;

    case 4: //Calibration reset confirm screen
      row0 = "RESET SOIL CAL?";
      row1 = "HOLD TO CONFIRM";
      lcd.setCursor(0, 0);
      lcd.print("RESET SOIL CAL?");
      lcd.setCursor(0,1);
      lcd.print("HOLD TO CONFIRM");
         ReadButton();
         ButtonDelayCheck(millis());
      break;

    case 5: // wet/dry graph
      lcd.setCursor(0, 0);
      lcd.print(" MOISTURE LEVEL ");
      lcd.setCursor(0,1);
      lcd.print("L");
      lcd.setCursor(15,1);
      lcd.print("H");
      lcd.setCursor(1,1);

      uint16_t minM, maxM, range, interval;

      minM = wetSet > drySet?drySet:wetSet;
      maxM = wetSet > drySet?wetSet:drySet;
      range = maxM - minM;
      interval = range / 14;

      //lEtS mAkE a GrApH
      for (int x = 1, y = drySet; x < 15; x++, y=y+interval)
      {
        //TODO WHEN NOT HIGH
        lcd.setCursor(x,1);
        
        if (capread > y + interval){
          lcd.print((char) 255);
          lcd.noBlink(); 
        }
        if (capread < y){
          lcd.print(" ");
          lcd.noBlink();
        } 
        if (wetSet > y && wetSet <= y+interval){
          lcd.print("W");
          lcd.noBlink();
        }
        if(capread > y && capread <= y + interval){
          lcd.blink();
        }
      }
      break;

    case 6:
      lcd.setCursor(0, 0);
      lcd.print("Peepee Screen 6");
      break;

      case 7:
      lcd.setCursor(0, 0);
      lcd.print("Peepee Screen 7");
      break;

      case 8:
      lcd.setCursor(0, 0);
      lcd.print("Peepee Screen 8");
      break;

      case 9:
      lcd.setCursor(0, 0);
      lcd.print("Peepee Screen 9");
      break;
  }
}

//Cycles the screen iterator and refreshes the display.
void CycleScreen(void){
  if(!noCycle){
    scrnItr = (scrnItr+1)%SCREEN_QTY;
    changeRequired = false;  
  }

  if(scrnItr == 4 && !soilSet && !noCycle) //skips soil sensor calibration reset and wet/dry status
  {
    scrnItr = 6;
  }
  
  lcd.clear();
  DisplayScreen(scrnItr);
}

void MeasureSoil(void) {
  long sum = 0;
  uint16_t sum_ct = 0;
  uint16_t r = 25, b = 0;

  noCycle = true;
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  PLACE SENSOR  ");

  RGB_color(r,0,b);
  
  lcd.setCursor(0,1);
  lcd.print("SAMPLING IN: ");
  lcd.setCursor(13,1);
 
  for (int y = 5; y > 0;y--){
    lcd.print(y);
    lcd.setCursor(13,1);
    delay(1000);
  }
  
  //TODO Screen things
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  NOW SAMPLING  ");
  lcd.setCursor(0,1);
  lcd.print("  DO NOT TOUCH  ");


  //10-second sample
  for (int z = 0; z<2;z++){
    
    for (int y = 0, x=0; y < 2500;y=y+5){
      //cycle rgb light because features
      RGB_color(r,0,b);
    
      if (y%100 == 0){
        ReadSensor();
        //Serial.print("Cap = ");
        //Serial.println(capread);
        //Serial.print("Sum = ");
        //Serial.println(sum);
        //Serial.print("x = ");
        //Serial.println(x);
        sum = sum + capread;
        r--; b++; x++; sum_ct++;
      }
      delay(5);
    }
           
    for (int y = 0, x=0; y < 2500; y=y+5){
      //cycle rgb light because features
    
      RGB_color(r,0,b);
      if (y%100 == 0){
        ReadSensor();
        //Serial.print("Cap = ");
        //Serial.println(capread);
        //Serial.print("Sum = ");
        //Serial.println(sum);
        //Serial.print("x = ");
        //Serial.println(x);
        sum = sum + capread;
        r++; b--; x++; sum_ct++;
      }
      delay(5);
    }
  }
  drySet = sum / sum_ct;

  RGB_color(0,0,25);
 
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" DRY POINT SET! ");
  lcd.setCursor(0,1);
  lcd.print("SETPOINT:");
  lcd.setCursor(10,1);
  lcd.print(drySet);

  delay(3000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WATER SOIL TO   ");
  lcd.setCursor(0,1);
  lcd.print("SET MOIST LEVEL ");
  
  delay(3000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" HOLD TO WATER: ");
  lcd.setCursor(0,1);
  
  //Serial.println("Hold to water printed");
  unsigned long x = millis();
  bool timer = false;
  //Serial.print("x: ");
  //Serial.println(x);
  //Serial.print("millis: ");
  //Serial.println(millis());
  //Serial.print("buttonState: ");
  //Serial.println(buttonState);
  unsigned long mils = millis();

  ReadButton();
  
    ReadSensor();

//Flag for watering process completion
  bool waterConfirm = false;
  
    mils = millis();
  
  //prevents excessive screen updates
  bool screenchange = true;
  
  //do while watering did not occur
  while(!waterConfirm){
    ReadButton();  
    if(buttonState == LOW ){
      //Serial.println("Entered low state if");
      if (screenchange){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("HOLD TO WATER OR");
        lcd.setCursor(0,1);
        lcd.print("READ IN ");
        screenchange = false;
      }
      for(int z = 30; z > 0; z--){
        //Serial.println("Entered water loop");
        //Serial.print("z =");
        //Serial.println(z);
        lcd.setCursor(8,1);
        lcd.print("  ");
        lcd.setCursor(8,1);
        lcd.print(z);
        
        for (int y = 0; y < 10; y++){
          ReadButton();
          if(buttonState == HIGH){
            while(buttonState == HIGH){
              //Serial.println("Watering");
              ValveOpen(0);
              ReadButton();
              //delay(50);
            }
            ValveClose();
            waterConfirm = true;
            z = 10;
    
            //Serial.println("Button pressed, watered");
            //break;
          }
          delay(100);
        }
      }
      //Serial.println("Exited low state if");
      ReadButton();
    }
  }
  sum = 0;
  sum_ct = 0;

lcd.setCursor(0,0);
lcd.clear();
lcd.print("SAMPLING STARTS :");
lcd.setCursor(0,1);
lcd.print("IN ");
 
for (int g = 5; g < 0; g--){
lcd.print(g);
lcd.setCursor(3,1);
delay(1000);  
}
   //TODO Screen thing
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  NOW SAMPLING  ");
  lcd.setCursor(0,1);
  lcd.print("  DO NOT TOUCH  ");
  
for (int zz = 0; zz<2 ;zz++){
  //5-second sample
  for (int a = 0, c=0; a < 2500;a=a+5){
    
    //cycle rgb light because features
    RGB_color(r,0,b);
    if (a%100 == 0){
      ReadSensor();
      //Serial.print("Sum = ");
      //Serial.println(sum);
      //Serial.print("c = ");
      //Serial.println(c);
      sum = sum + capread;
      r--; b++; c++; sum_ct++;
    }
    delay(5);
      }
           
    for (int a = 0, c=0; a < 2500; a=a+5){
    //cycle rgb light because features
    RGB_color(r,0,b);
      if (a%100 == 0){
        ReadSensor();
        //Serial.print("Sum = ");
        //Serial.println(sum);
        //Serial.print("c = ");
        //Serial.println(c);
        sum = sum + capread;
        r++; b--; c++; sum_ct++;
      }
      delay(5);
     }
    }

    if(sum / sum_ct < drySet + 5 || sum / sum_ct > drySet -5){
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("NO CHANGE DETECTE");
      lcd.setCursor(0,1);
      lcd.print("D. ABORTING...");
      soilSet = false;
      delay(3000);
      scrnItr = 3;

    }
    else{
      wetSet = sum / sum_ct;

      RGB_color(0,0,25);
 
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print(" WET POINT SET! ");
      lcd.setCursor(0,1);
      lcd.print("Setpoint:");
      lcd.setCursor(10,1);
      lcd.print(wetSet);
      soilSet = true;
      delay(3000);
      scrnItr = 3;
      
    }
    DisplayScreen(scrnItr);

      noCycle = false;
}
//defines actions for screens when 3-sec hold is performed by users on certain screens
//SCREEN COUNT 10
void ButtonDelayCheck(unsigned long time)
{
    unsigned long tmp = millis();
    switch(scrnItr)
    {
      case 1: //valve status screen
      if (millis()- time >= 3000){     
        if (vlv_state==true){
          ValveClose();
    //    buttonPress = millis();
        }
        else{
          ValveOpen(0);
          ValveClose();
        }
        noCycle = true;
      }
      break;

      case 2: //Setpoint not set screen
       if (millis() - time >= 3000 && soilSet == false && buttonState == HIGH){
    //    CycleScreen();
           MeasureSoil();
       }
       
       case 3: //setpoint calibrated
       //TODO
      break;

      case 4: //calibration reset screen
      if (buttonPress > tempDisplay && millis()-buttonPress >= 3000 && buttonState == HIGH) {
       MeasureSoil();
      }
      else if (millis() - tempDisplay > 10000 && buttonPress < tempDisplay && buttonState == LOW){
        scrnItr--;
        DisplayScreen(scrnItr);
      }
    break;

    case 5: 
       //TODO
    break;
    case 6: 
       //TODO
    break;
    
    case 7: 
       //TODO
    break;
    
    case 8: 
       //TODO
    break;

    case 9:
      //TODO
    break;
}}

void ReadButton(void){
 // //Serial.println("ReadButton Entered");
  buttonState = digitalRead(button);
   
 if(buttonState == HIGH){
  if (buttonFlag == false){
    buttonPress = millis();
 // //Serial.print(buttonPress);
    buttonFlag = true; 
    //Serial.println("changed from false to true"); 
    } 
 }
 else if(buttonState == LOW && buttonFlag == true){
   buttonRelease = millis();
   buttonFlag = false;
   //Serial.println("changed from true to false");
 } 
}

//TODO NEED TO ACCOUNT FOR SOIL VARIANCE
void AutoWater(void){  
  //go through sample array
  for (int x = 0; x < SMPL_CT; x++){
    bool entered = false;

    if (samples[x] == NULL){
      samples[x] = capread;
      entered = true;
      Serial.print("Measurement entered: x=");
      Serial.print(x);
      Serial.print(": ");
      Serial.println(samples[x]);
      break;
      
    }
    
    else if (x == SMPL_CT-1 && !entered){
      Serial.println("Restructuring occurred");
      for (int y = 0; y < SMPL_CT-1; y++){
        samples[y] = samples[y+1];
      }
      samples[SMPL_CT-1] = capread;
      Serial.print("Measurement entered: y=");
      Serial.print(x);
      Serial.print(": ");
      Serial.println(samples[SMPL_CT-1]);
      entered = true;
    }

    if (x == 59 && entered){
      Serial.println("Doing math to sum");
      unsigned long sum = 0, mean = 0, range = (long)(((float) wetSet-(float) drySet) * 0.8);
  
      for (int x = 0; x < SMPL_CT; x++){
        sum = sum + samples[x];        
      }
      Serial.print("sum: ");
      Serial.println(sum);
      mean = sum/SMPL_CT;
      Serial.print("mean: ");
      Serial.println(mean);

      //if mean is within <= 5 of drypoint, run until it is greater than or equal to wetpoint
      if ((mean < drySet + 5 && drySet < wetSet) || (mean > drySet -5 && drySet > wetSet)){
        lcd.setCursor(0,0);
        lcd.clear();
        lcd.print("WATERING IN:  ");
        for (int x = 5; x >= 0; x--){
          lcd.setCursor(15,0);
          lcd.print(x);
          delay(1000);
        }
        lcd.clear();
        DisplayScreen(5);

        while ((capread < wetSet && drySet < wetSet) || (capread > wetSet && drySet > wetSet)){
          //no soil abort
          if (capread <=340){
            ValveClose();
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("SOIL NOT DETECT");
            lcd.setCursor(0,1);
            lcd.print("ED. ABORTING!  ");
            delay(5000);
            break;
          }
          ValveOpen(1);
          ReadSensor();

        }; 
        ValveClose();

        for (int a = 0; a < SMPL_CT;a++){
          samples[a] = NULL;
        }
        DisplayScreen(scrnItr);
      }
    }
  }
}
