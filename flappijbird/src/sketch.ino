#include "LedControlMS.h"
#include "scroller.h"
#include <EEPROM.h>
/*** 

flappIJbird for Ijduino. (C)2015 edwin@datux.nl

first created at the https://nurdspace.nl Nurd-inn.

ijduino: http://ijhack.nl/project/ijduino

github: https://github.com/psy0rz/stuff/tree/master/flappijbird

Released under GPL. 

***/


static const int DATA_PIN = 20;
static const int CLK_PIN  = 5;
static const int CS_PIN   = 21;

static const int lowPin = 11;             /* ground pin for the buton ;-) */
static const int buttonPin = 9;           /* choose the input pin for the pushbutton */
static const int soundPin = 8;             

static const int resetScorePin = 10;      /* pull this to ground to reset score */
static const int scoreAddress = 10;           /* choose the input pin for the pushbutton */

static const int INTENSITY = 5;


LedControl lc=LedControl(DATA_PIN, CLK_PIN, CS_PIN, 1);

void setup()
{
    /*
       The MAX72XX is in power-saving mode on startup,
       we have to do a wakeup call
     */
    lc.shutdown(0,false);
    /* Set the brightness to a medium values */
    lc.setIntensity(0,INTENSITY);
    /* and clear the display */
    lc.clearDisplay(0);
    /* setup pins */
    pinMode(buttonPin, INPUT_PULLUP);
    pinMode(lowPin, OUTPUT);
    digitalWrite(lowPin, LOW);

    //reset score?
    pinMode(resetScorePin, INPUT_PULLUP);
    if (!digitalRead(resetScorePin))
    {
      EEPROM.write(scoreAddress,0);
    }
}

//screen size
#define Y_MAX 1000 //not actual screen size..we downscale this later because arduino is bad at floats
#define Y_MIN -100 //negative so the player doesnt instantly die on the bottom
#define X_MAX 7
#define X_MIN 0

#define BIRD_JUMP_SPEED 60

#define TUBES 3 //max nr of tubes active at the same time

char msg[100];

struct tube_status
{
  int y;
  int x;
  int gap;
};

void(* reboot) (void) = 0;

void finished(int score)
{
  noTone(soundPin); //interference with scroll
  sprintf(msg,"    %d  ", score);
  scrolltext(lc, msg, 50);

  //got highscore?
  if (score>EEPROM.read(scoreAddress) || EEPROM.read(scoreAddress==255))
  {
    EEPROM.write(scoreAddress, score);
    //TODO: rickroll
  }
  else
  {
    ;
  }
}


void loop()
{


  //bird physics
  int bird_y=Y_MAX/2;
  int bird_x=3;
  int bird_speed=0;
  int bird_gravity=-7;
  byte bird_bits=0;

  //tube
  int tube_min=100;
  int tube_max=900;
  int tube_gap=300;

  int tube_shift_delay=250; //milliseconds between each left shift
  tube_status tubes[TUBES]; 
  unsigned long tube_time=millis();
  int tube_countdown=10; //cycles before creating next tube
  int tube_countdown_min=10;
  int tube_countdown_max=100;
  byte tube_bits_at_bird=0;

  int score=0;

  unsigned long start_time=millis();
  int frame_time=25;

  bool button_state=true;

  //init tubes  
  for (int tube_nr=0; tube_nr<TUBES; tube_nr++)
  {
    tubes[tube_nr].x=X_MIN-1; //disables it
  }

  //show highscore
  sprintf(msg,"   highscore %d - flappIJbird ", EEPROM.read(scoreAddress));
  while(scrolltext(lc, msg, 25, buttonPin));

  while(1)
  {
    start_time=millis();
    //////////////////////////////// bird physics and control

    //gravity, keep accelerating downwards
    bird_speed=bird_speed+bird_gravity;

    //button changed
    if ( digitalRead(buttonPin) != button_state)
    {
      button_state=digitalRead(buttonPin);

      //its pressed, so jump! 
      if (!button_state)
          bird_speed=BIRD_JUMP_SPEED;
    }

    //change y postion of bird
    bird_y=bird_y+bird_speed;

    //crashed on bottom?
    if (bird_y<Y_MIN)
    {
      for (int i=0; i<20; i++)
      {
        tone(soundPin, 2000- (i*100), 200);
        lc.setRow(0, bird_x, tube_bits_at_bird);
        delay(50);
        lc.setRow(0, bird_x, bird_bits);
        delay(50);
      }
      finished(score);
      reboot();
    }

    if (bird_y>Y_MAX)
      bird_y=Y_MAX;

    //downscale birdheight to 8 pixels :P
    //( LSB is top pixel )
    bird_bits=(B10000000 >> ((bird_y*7) / Y_MAX));

    //////////////////////////////// tubes

    tube_countdown--;

    //shift all tubes left
    if (millis()-tube_time > tube_shift_delay)
    {
      tube_bits_at_bird=0;

      tube_time=millis();
      for (int tube_nr=0; tube_nr<TUBES; tube_nr++)
      {
        if (tubes[tube_nr].x>=X_MIN)
        {
          //remove from old location
          lc.setRow(0, tubes[tube_nr].x, 0);
          tubes[tube_nr].x--;

          //draw on new location, and do collision detection
          if (tubes[tube_nr].x>=X_MIN)
          {
            //determine tube-bits (some bitwise magic instead of forloops)
            byte tube_bits=(1 <<((tubes[tube_nr].gap*7/Y_MAX)+1))-1; //determine gap-pixels  00000111    
            tube_bits=tube_bits << (tubes[tube_nr].y*7/Y_MAX); //shift gap in place          00011100     
            tube_bits=~tube_bits; //invert it to get an actual gap                           11100011

            //are we on the birdplace?
            if (tubes[tube_nr].x==bird_x)
            {
              tube_bits_at_bird=tube_bits;
            }
            else
            {
              lc.setRow(0, tubes[tube_nr].x, tube_bits);
            }

            //is the tube past the bird?
            if (tubes[tube_nr].x==bird_x-1)
            {
              score++; //scored a point! \o/
              tone(soundPin, 1000+(score*100), 100);
            }
          }
        }
        //tube is inactive, do we need a new tube?
        else
        {
          if (tube_countdown<0)
          {
            tubes[tube_nr].x=X_MAX+1;
            tubes[tube_nr].y=random(tube_min, tube_max);
            tubes[tube_nr].gap=tube_gap;
            tube_countdown=random(tube_countdown_min, tube_countdown_max);
          }
        }
      }
    }

    //draw bird     
    lc.setRow(0, bird_x,  bird_bits|tube_bits_at_bird);
 
    //is the tube at the bird? 
    if (tube_bits_at_bird)
    {
      //collision?
      if ( (tube_bits_at_bird|bird_bits) == tube_bits_at_bird)
      {
        for (int i=0; i<10; i++)
        {
          tone(soundPin, 1000, 100);
          lc.setRow(0, bird_x, bird_bits);
          delay(50);
          tone(soundPin, 500, 100);
          lc.setRow(0, bird_x, bird_bits|tube_bits_at_bird);
          delay(50);
        }
        finished(score);
        reboot();
      }
    }


    //wait for next frame
    while( (millis()-start_time) < frame_time){
      ;
    }
  }
}