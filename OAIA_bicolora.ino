#include "LedControl.h" //  need the library 
LedControl lc = LedControl(12,11,10,2); //DIN, CLK, LOAD, No. DRIVER: rosu + verde de la matricea bicolora

#include <LiquidCrystal.h>
#define V0_PIN 9  
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); 

#define X_PIN A1
#define Y_PIN A2

#define BUTTON_PIN 8

byte green[8] = {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B11111111};
byte red[8] = {B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B00000000,B11111111}; 
 
/*
              columns
   row 0: 7 6 5 4 3 2 1 0
   row 1: 7 6 5 4 3 2 1 0
   ... 
*/

int rowValue, colValue; /// pozitiile pe care le ia oaia

bool stop = false; /// cat timp merge jocul pe matrice
bool lost = false;

struct pen{
  int col; bool done; // col = coloana tarcului, done = daca a fost vizitat
}pen[8];
int nrPen = 4; 

unsigned long time; /// = millis()
int movePeriod = 200, blinkPenPeriod = 400, moveCarsPeriod = 500, oneSecond = 1000, freezePeriod = 100;
unsigned long freezeTime, timeToMove = 0, blinkPenTime = 0, createTime = 0, moveCarsTime = moveCarsPeriod, updateDisplayTime = 0; 

int score, nrLives, nrLevel, levelPeriod, dontMoveTime = 0, currentTime,freezeStage = 0;   

bool displayPressTheButton = false, continueButton = true,  blinkButton = false, freeze = false;

void startPosition() ///pozitia pe care o ia oaia
{
  rowValue = 6; 
  colValue = 3; 
}
void setPeriodTimes()
{ 
  timeToMove = time;
  blinkPenTime = time; 
  createTime = time; 
  moveCarsTime = time + moveCarsPeriod; 
  updateDisplayTime = time;
}
void setLevel()
{
  startPosition();
  for(int i=0;i<nrPen;i++)
   { 
      pen[i].done = false;
   } 
  setPen(); 
  setPeriodTimes();
} 
void startFirstLevel() 
{
  setLevel();
  levelPeriod = 50;
  score = 0; 
  currentTime = levelPeriod; 
  nrLives = 3;
  nrLevel = 1;
  startPosition(); 
} 
void nextLevel()
{
  setLevel();
  nrLevel++; 
  if(levelPeriod > 25) levelPeriod -= 5; 
  currentTime = levelPeriod; 
  if(moveCarsPeriod > 300) moveCarsPeriod -= 30;
}

void joystick_value(int value, int &matrix_value)
{
  /*Serial.println(value);
       0
    0     1023
      1023
   */
  
   if(value<400)
  {  
    matrix_value--;
    if(matrix_value==-1) matrix_value=0;
  }
  else if(value>600)
  { 
    matrix_value++;
    if(matrix_value==8) matrix_value=7;
  }
  
}
 
void setPen() /// setam coloanele celor 4 tarcuri
{   
  pen[0].col = 7;
  pen[1].col = 5;
  pen[2].col = 2;
  pen[3].col = 0;
}

void blinkPen() /// animatia de clipire a tarcurilor
{
  for(int k = 0; k < nrPen; k++) 
    if(pen[k].done == false)
  {
    int column = pen[k].col;  
    if(bitRead(green[0],column) == 1) bitWrite(green[0],column,0);
      else bitWrite(green[0],column,1); 
  }
    else bitWrite(green[0],pen[k].col,1);
} 

void displayMatrix(int nrDriver, byte matrix[]) /// 0 = verde; 1 = rosu
{
  for (int row = 0; row < 8; row++)
    for (int col = 0; col < 8; col++)
      {
        lc.setLed(nrDriver, row, col, bitRead(matrix[row],col));      
      }
}

void deleteMatrix(int r) // sterge matricea incepand cu linia r
{
  for (int row = r; row < 7; row++)
    for (int col = 0; col < 8; col++)
      { 
        bitWrite(green[row],col,0);       
        bitWrite(red[row],col,0); 
      }
    for (int col = 0; col < 8; col++)
      { 
        bitWrite(green[7],col,1);       
        bitWrite(red[7],col,1); 
      } 
}


void setup() 
{ 
  
   lc.shutdown(0, false); lc.shutdown(1, false);  // turn off power saving, enables display
  
   lc.clearDisplay(0); lc.clearDisplay(1); // clear screen
  
   lc.setIntensity(0, 15); lc.setIntensity(1, 15); // sets brightness (0~15 possible values)
 
   Serial.begin(9600);

   randomSeed(analogRead(0));

   pinMode(V0_PIN, OUTPUT); // PWN in loc de POTENTIOMETRU
   analogWrite(V0_PIN, 140); // PWN in loc de POTENTIOMETRU
   lcd.begin(16, 2);
   lcd.clear();
 

   byte heart[8] =  { B00000, B01010, B10101, B10001, B01010, B00100, B00000, B00000};  
   //byte rightArrow[8] =  { B00000, B00100, B00110, B11111, B00110, B00100, B00000, B00000};
   byte arrow[8] = { B00100, B01110, B11111, B00100, B00100, B00100, B00100, B00000};
   
   lcd.createChar(1,heart); 
   lcd.createChar(2,arrow); 
   
   pinMode(BUTTON_PIN,INPUT);

   stop = true; nrLevel = 0;
  
}
 
void createCar() /// creeaza o masina pe o linie random de la 2 la 4
{
  bitSet(red[random(2,5)],7); // 7 e prima poz  
}

void moveCars()
{
  // row 0: 7 6 5 4 3 2 1 0
  // row 1: 7 6 5 4 3 2 1 0
  // ... 
  
  for (int row = 4; row >= 2; row--)
  {
    if(bitRead(red[row],0) == 1 && bitRead(red[row],1) == 0)
       { 
           bitWrite(red[row],0,0);
       } 
       
    if(bitRead(red[row],0) == 1 && bitRead(red[row],1) == 1)
       { 
           bitWrite(red[row],1,0);
       }       
       
    for (int col = 1; col <= 6; col++)
    {
       if(bitRead(red[row],col) == 1 && bitRead(red[row],col+1) == 1)
       {
          bitWrite(red[row],col-1,1);
          bitWrite(red[row],col+1,0); 
       }
    }
    
    if(bitRead(red[row],7) == 1)
       {
           bitWrite(red[row],6,1);
       }
       
  }
} 

bool checkPen(int col) /// verifica daca ai mai fost, in caz contrar done devine true
{
  for(int i=0; i<nrPen; i++)
    if(col == pen[i].col)
    {
      if(pen[i].done == false)
      {
        pen[i].done = true;
        return true;
      } 
      else return false; 
    }
    
  return false;
}

bool checkLastPen() /// verifica daca mai exista tarcuri de vizitat
{
  for(int i=0; i<nrPen; i++)
    if(pen[i].done == false)
      return false;
  return true;
} 


void printLevel()
{ 
   lcd.setCursor(5, 0);
   lcd.print("Level ");
   lcd.print(nrLevel);
}
void printTime(int time)
{
  lcd.setCursor(0, 1);
  lcd.print(time); 
  lcd.print("s");
  if (time == 0) { stop = true; lost = true; displayPressTheButton = false; }
}

void printLives()
{ 
  int col = 15; /// ultima col din lcd
  for(int k=0; k<nrLives; k++)
     {
       lcd.setCursor(col--, 1); 
       lcd.write(1);  
     }
  if (nrLives == 0) { stop = true; lost = true; displayPressTheButton = false; }
}
void printScore()
{ 
  lcd.setCursor(6, 1);
  lcd.print("*");
  lcd.print(score); 
  lcd.print("*"); 
}


void displayMenu()
{  
  if(lost == true) /// daca ai pierdut  
        { 
           lcd.clear();
           if(displayPressTheButton == false)
           {
               lcd.setCursor(4,0); 
               lcd.print("GAME OVER");
     
               lcd.setCursor(0,1);
               lcd.print("YOU SCORED: ");
               lcd.print(score);
               displayPressTheButton = true;
           }
           else
           {
               lcd.setCursor(0,0);   
               lcd.print("Press the button");
              
               lcd.setCursor(2,1);
               lcd.print("to restart "); lcd.write(2);
               displayPressTheButton = false;
           }
            updateDisplayTime += 5*oneSecond; 
              
        }
    else if(nrLevel == 0)  /// primul meniu
        { 
           lcd.clear();
           if(displayPressTheButton == false)
           {
               lcd.setCursor(4,0); 
               lcd.print("SHEEPISH");
               lcd.setCursor(6,1); 
               lcd.write(1); lcd.write(1); lcd.write(1); lcd.write(1);
      
               displayPressTheButton = true;
           }
           else 
           {
               lcd.setCursor(0,0);   
               lcd.print("Press the button");
              
               lcd.setCursor(4,1);
               lcd.print("to play "); lcd.write(2);
               displayPressTheButton = false;
           }
            updateDisplayTime += 5*oneSecond; 
              
        }
      else /// meniul dintre nivele
        {
           lcd.clear();
           
           lcd.setCursor(3,0); 
           lcd.print("Score: "); lcd.print(score);

           if(blinkButton == true)
           {
              if(continueButton == false)
              {   
                lcd.setCursor(0,1); 
                lcd.print("Continue");
              }
              else
              {     
                lcd.setCursor(9,1);   
                lcd.print("Restart"); 
              }
              blinkButton = false;
              updateDisplayTime += 300; 
           } 
           else 
           {
              lcd.setCursor(0,1); 
              lcd.print("Continue Restart");  
              blinkButton = true;
              updateDisplayTime += 600; 
           }
          
        }  
}  
void carCollision(int r, int c)
{
  if(score>0) score -= 5;
  freezeTime = time;
  freezeStage = 1;  
  freeze = true;   
  nrLives--;    
  bitWrite(green[r],c,0); 
}
void changeMatrix(int r, int c) /// animatia de coliziune
{ 
  if(freezeStage%5 == 1)
  {
    bitWrite(red[r],c,0);
    bitWrite(red[r],c+1,1);
    bitWrite(red[r],c-1,1);
    bitWrite(red[r+1],c,1);
    bitWrite(red[r-1],c,1);
    //displayMatrix(1,red); delay(freezePeriod);
  }
  else if(freezeStage%5 == 2)
  {
    for(int col=c-1; col<=c+1; col++) 
    {
      bitWrite(red[r-2],col,1);
      bitWrite(red[r+2],col,1);
    }
    for(int row=r-1; row<=r+1; row++) 
    {
      bitWrite(red[row],c-2,1);
      bitWrite(red[row],c+2,1);
    }
    //displayMatrix(1,red); delay(freezePeriod);
  }
  else if(freezeStage%5 == 3)
  {
    bitWrite(red[r-2],c-1,0); 
    bitWrite(red[r-2],c+1,0);
    bitWrite(red[r+2],c-1,0);
    bitWrite(red[r+2],c+1,0);
    bitWrite(red[r-1],c-2,0); 
    bitWrite(red[r-1],c+2,0);
    bitWrite(red[r+1],c-2,0);
    bitWrite(red[r+1],c+2,0);
    bitWrite(red[r-1],c-1,1); 
    bitWrite(red[r-1],c+1,1);
    bitWrite(red[r+1],c-1,1);
    bitWrite(red[r+1],c+1,1); 
    //displayMatrix(1,red); delay(freezePeriod);
  }
  else if(freezeStage%5 == 4)
  {  
    bitWrite(red[r-1],c-1,0); 
    bitWrite(red[r-1],c+1,0);
    bitWrite(red[r+1],c-1,0);
    bitWrite(red[r+1],c+1,0); 
    bitWrite(red[r-2],c,1);  
    bitWrite(red[r+2],c,1); 
    bitWrite(red[r],c-2,1);  
    bitWrite(red[r],c+2,1);
    //displayMatrix(1,red); delay(freezePeriod);
  }
  else  
  { 
    bitWrite(red[r],c,1); 
    bitWrite(red[r-2],c,0);  
    bitWrite(red[r+2],c,0); 
    bitWrite(red[r],c-2,0);  
    bitWrite(red[r],c+2,0); 
    //displayMatrix(1,red); delay(freezePeriod); 
  }
  
  freezeStage++;
} 
void loop() 
{  
 
  
  time = millis();   
  if(stop == true) /// lcd-ul afiseaza un meniu
  { 
      int value = analogRead(X_PIN);
      if(value<400) continueButton = true;
        else if(value>600) continueButton = false;

      if(digitalRead(BUTTON_PIN) == 1) 
      {
        //Serial.println("click"); 
        if(nrLevel == 0 || lost == true || continueButton == false) startFirstLevel();
        else 
        {
          nextLevel();
        }
        lost = false;
        stop = false;
      }   
        
      if(time > updateDisplayTime)
         {
            displayMenu();
         }       
 
  } 
  else 
  {     
      
      if(time > updateDisplayTime)
      {
        lcd.clear(); 
        printLevel();
        printTime(currentTime);
        printScore();
        printLives();
        currentTime--;
        updateDisplayTime += oneSecond; 
        
      }

      if(freeze == false)
      {
        if(time > blinkPenTime) /// animatia de clipire a tarcurilor
        {
          blinkPen();
          blinkPenTime += blinkPenPeriod;
        }
       
        if(time > moveCarsTime) /// misca toate masinile o data la moveCarsPeriod
        {
          moveCars(); 
          moveCarsTime += moveCarsPeriod;
        } 
        
        if(time > createTime) /// creeaz o masina o data la 1.5 - 2.5 sec
        {
          createCar();
          createTime += random(1500,2500); 
        } 
      
        if(time > timeToMove) /// se misca oaia
        {       
          if(dontMoveTime == 0) 
          {
            joystick_value(analogRead(Y_PIN),rowValue); 
            joystick_value(analogRead(X_PIN),colValue);
            if(rowValue == 7) rowValue = 6;
           
            bitWrite(green[rowValue],7 - colValue,1); 
  
  
            if(bitRead(red[rowValue],7 - colValue) == 1) /// daca acolo se afla un punct rosu pe matrice = masina
                carCollision(rowValue,7 - colValue);
            
          }
            else dontMoveTime -= movePeriod; /// oaia sta nemiscata cat timp dontMoveTime>0
          
       
          displayMatrix(1,red); displayMatrix(0,green); /// afiseaza matricea
         
          if(dontMoveTime == 0) 
            bitWrite(green[rowValue],7 - colValue,0);
      
          if(rowValue == 0 && checkPen(7 - colValue) == true) /// daca se afla intr-un tarc
          {
            score += 20; // Serial.println("+20p tarc");
            if(checkLastPen() == true) /// daca este ultimul treci la meniul dintre nivele
            { 
              stop = true; 
              score = score+currentTime+1; // Serial.println(currentTime+1);
            }
            else /// altfel se duce la poz de start si sta nemiscata o secunda
            {
              dontMoveTime = 1000; 
              startPosition(); 
              bitWrite(green[rowValue],7 - colValue,1); 
            }
          } 
          timeToMove += movePeriod;   
        }
          
      }
     else /// coliziune cu o masina
      {
        Serial.println(freezeStage);
        Serial.print(time);
        Serial.print(" ");
        Serial.println(freezeTime);
        
        if(time > freezeTime)
        {
          changeMatrix(rowValue, 7-colValue); 
          displayMatrix(1,red);
          freezeTime += freezePeriod; 
        }
        if(freezeStage == 11)
        { 
          freeze = false;
          deleteMatrix(1); startPosition();
          displayMatrix(1,red);
          displayMatrix(0,green);
          setPeriodTimes();
        }
      }
       
      if(stop == true) /// urmeaza un meniu
        {
          deleteMatrix(0);
          displayMatrix(1,red);
          displayMatrix(0,green);
        }
       
  }
  
 
}
