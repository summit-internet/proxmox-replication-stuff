#include <SPI.h>
// #include <EEPROM.h>

#include "../pin_config_default.h"
// #include "messaging.h"
// #include "eeprom_config.h"
#include "utils.h"

// hardware SPI pins:
// For "classic" Arduinos (Uno, Duemilanove,
// etc.), data = pin 11, clock = pin 13.  For Arduino Mega, data = pin 51,
// clock = pin 52.  For 32u4 Breakout Board+ and Teensy, data = pin B2,
// clock = pin B1.  For Leonardo, this can ONLY be done on the ICSP pins.

//dont change these, use COLOR_BYTE below.
#define R 0
#define G 1
#define B 2


//32/leds/meter version:
#define LED_COUNT 160
#define COLOR_BYTE0 G
#define COLOR_BYTE1 R
#define COLOR_BYTE2 B

//54/leds/meter version:
//#define LED_COUNT 160
// #define LED_COUNT 160
// #define COLOR_BYTE0 B
// #define COLOR_BYTE1 R
// #define COLOR_BYTE2 G


//current rgbvalues
byte curr_rgb[LED_COUNT][3];
//target rbgvalues
byte want_rgb[LED_COUNT][3];
//fade speed to reach the target with
char fade_speed[LED_COUNT]; //we use the char just as an 'signed byte' ;)

word fade_step=0;
word par_pat=9;
byte par_col[3][3]={
  { 80,0  ,00 }, //main color
  { 127 ,50,0 }, //second color
  { 10  ,0  ,0 } //background color
};

byte par_fade[3]={
  0, //main fadespeed
  0, //second fadespeed
  0//background fadespeed
};

byte par_speed=127; //"speed" of pattern, sometimes beats per minute, sometimes something else

//sets the led to specified value on next update.
void led_set(word led, byte r, byte g, byte b)
{
  want_rgb[led][0]=r;
  want_rgb[led][1]=g;
  want_rgb[led][2]=b;
  curr_rgb[led][0]=r;
  curr_rgb[led][1]=g;
  curr_rgb[led][2]=b;
  fade_speed[led]=0;
}

//fades from current value to target value.
void led_fade_to(word led, byte r, byte g, byte b, char new_speed)
{
  want_rgb[led][0]=r;
  want_rgb[led][1]=g;
  want_rgb[led][2]=b;
  fade_speed[led]=new_speed;
}

//instantly sets the led to the specified value, and fades back to the current value.
//(nice for sparkles :))
void led_fade_from(word led, byte r, byte g, byte b, char new_speed)
{
  curr_rgb[led][0]=r;
  curr_rgb[led][1]=g;
  curr_rgb[led][2]=b;
  fade_speed[led]=new_speed;
}


bool check_radio=false;
void radio_interrupt()
{
  check_radio=true;
}

void setup() {



  // config_read();
  Serial.begin(115200);

  // attachInterrupt(0, radio_interrupt, FALLING);

  // msg.begin();
  // msg.send(PSTR("led.boot"));
  Serial.printf("CHUCCCCHE\n");

  // Start SPI communication for the LEDstrip
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.transfer(0); // 'Prime' the SPI bus with initial latch (no wait)


  //clear all leds, even if there are more than we use:
  for (word led=0; led<20000; led++)
  {
    SPI.transfer(0x80);
  }
  SPI.transfer(0);

  //initialize led array
  for (word led=0; led<LED_COUNT; led++)
  {
    led_set(led, 0,0,0);
  }
}


//yellow and red 'flames'
void do_fire()
{
   byte led;

  if (par_speed>=random(255))
  {
    led=random(LED_COUNT);
    led_fade_to(led, par_col[0][0], par_col[0][1], par_col[0][2], par_fade[0]);

    led=random(LED_COUNT);
    led_fade_to(led, par_col[1][0], par_col[1][1], par_col[1][2], par_fade[1]);
  }

  //let flames dimm slowly
  led=random(LED_COUNT);
  led_fade_to(led, par_col[2][0], par_col[2][1], par_col[2][2], par_fade[2]);
}



//give idle leds a glowy effect
void do_glowy(
  char chance=0, //chance there is a led glwoing? (e.g. lower is more glowing)
  byte f_r=1, byte f_g=0, byte f_b=0,
  byte t_r=5, byte t_g=0, byte t_b=0, //rgb range to glow between
  char fade_min=20, char fade_max=50 //minimum and maximum fade speed
)
{
  if (random(chance)==0)
  {
    byte led=random(LED_COUNT);
    if (fade_speed[led]==0)
    {
        led_fade_to(led,
          random(f_r, t_r),
          random(f_g, t_g),
          random(f_b, t_b),
          random(fade_min, fade_max)
          );
    }
  }
}

//sparkle a random led sometimes
void do_sparkle(
  char chance=10, //chance there is a sparc? (e.g. lower is more sparcles)
  boolean idle=true, //only leds that are not currently fading?
  byte r=127, byte g=127, byte b=127, // color
  word spd=-10  //fade speed
)
{
  if (random(chance)==0)
  {
    byte led=random(LED_COUNT);
    if (idle && fade_speed[led]!=0)
      return;
    led_fade_from(led, r,g, b, spd);
  }
}



//a 'kit radar'
//default its configured to look like it, but usually you will reconfigure it completely different.
//it can be usefull for many different effects.
void do_radar(
  byte r=127, byte g=0, byte b=0, // color
  word spd=64, //speed. (skips this many updates),
  char fade_min=2, char fade_max=2, //minimum and maximum fade length (randomizes  between this for smoother effect)
  word start_led=0, word end_led=7, //start and end lednr
  byte dir=2, //direction: 0 left, 1 right, 2 both
  boolean mix=false //mix existing colors
  )
{
  if (fade_step%spd == 0)
  {
     word led;
     word our_step=(fade_step/spd);

     //left
     if (dir==0)
     {
        led=(
         our_step
         %(end_led-start_led+1) //keep it in our range
        )+start_led;
     }
     //right
     else if (dir==1)
     {
        led=end_led-(
         our_step
         %(end_led-start_led+1) //keep it in our range
        );
     }
     //both
     else
     {
        word range=(end_led-start_led+1);
        led=(our_step%(range*2));
        if (led>=range)
          led=range-led+range-1;

//        0 1 2 3 4 5

//        0 1 2 2 1 0

        led=led+start_led;
     }

     if (mix)
     {
         if (!r)
           r=curr_rgb[led][0];

         if (!g)
           g=curr_rgb[led][1];

         if (!b)
           b=curr_rgb[led][2];

     }

     led_fade_from(led, r,g,b, random(fade_min, fade_max));
  }
}



unsigned long last_micros=0;
//execute one fade step and limit fps
void run_step(unsigned long time=1000)
{
  //use interupts to prevent unnecceary delays and SPI bus data
  // if (check_radio || Serial.available())
  // {
  //   check_radio=false;
  //   msg.loop();
  // }

  //update all the current and wanted values
  for (word led = 0; led < LED_COUNT; led++) {
    //FIRST send the current values, then do the fades
    SPI.transfer(0x80 | (curr_rgb[led][COLOR_BYTE0]));
    SPI.transfer(0x80 | (curr_rgb[led][COLOR_BYTE1]));
    SPI.transfer(0x80 | (curr_rgb[led][COLOR_BYTE2]));

    //now do all the fades
    char s=fade_speed[led];
    byte done=0;

    //127 means: DONE fading
    if (s!=127)
    {
      //change by 1 , every s+1 steps:
      if (s>=0)
      {
        if ((fade_step % (s+1) ) == 0)
        {
          for (byte color=0; color<3; color++)
          {
            if (want_rgb[led][color]>curr_rgb[led][color])
              curr_rgb[led][color]++;
            else if (want_rgb[led][color]<curr_rgb[led][color])
              curr_rgb[led][color]--;
            else
              done++;
          }
        }
      }
      //change by s, every step
      else
      {
        for (byte color=0; color<3; color++)
        {
          char diff=want_rgb[led][color]-curr_rgb[led][color];
          //step size is greater than difference, just jump to it at once
          if (-s > abs(diff))
          {
            curr_rgb[led][color]=want_rgb[led][color];
            done++;
          }
          else
          {
            //we need to increase current value (note that s is negative)
            if (diff>0)
              curr_rgb[led][color]-=s;
            //we need to decrease current value
            else if (diff<0)
              curr_rgb[led][color]+=s;
            else
              done++;
          }
        }
      }
      if (done==3)
        fade_speed[led]=127; //done fading, saves performance
    }
  }

  // Sending a byte with a MSB of zero enables the output
  // Also resets the led at which new commands start.
  SPI.transfer(0);

  fade_step++;

  //limit the max number of 'frames' per second
  while (micros()-last_micros < time) ;
  last_micros=micros();

}


void msg_handle(uint16_t from, char * event,  char * par)
{
  //when debugging parsing issues
  // Serial.println("from: ");
  // Serial.println(from);
  // Serial.println("event: ");
  // Serial.println(event);
  // Serial.println("par: ");
  // Serial.println(par);

  if (strcmp_P(event, PSTR("led.pat"))==0)
  {
    //fade leds to background color
    for (word led=0; led<LED_COUNT; led++)
    {
      led_fade_to(led, 0,0,0,1);
    }

    if (par!=NULL)
      par_pat=atoi(par);

    return;
  }

  if (strcmp_P(event, PSTR("led.speed"))==0)
  {
    if (par!=NULL)
      par_speed=atoi(par);
    return;
  }

  //adjust color
  //usually we have 3 color-settings with 3 rgb values each:
  // led.col <colornr> <rvalue> <gvalue> <bvalue>
  if (strcmp_P(event, PSTR("led.col"))==0)
  {
    if (par==NULL)
      return;

    byte colnr=atoi(par);
    par=strchr(par,' ')+1;

    if (colnr>2)
      return;

    byte i=0;
    while (par!=NULL && i<3)
    {
      par_col[colnr][i]=atoi(par);
      par=strchr(par,' ')+1;
      i++;
    }
    return;
  }

  //adjust fade speed
  //each color has a fade speed
  //led.fade <colornr> <speed>
  if (strcmp_P(event, PSTR("led.fade"))==0)
  {
    if (par==NULL)
      return;

    byte colnr=atoi(par);
    par=strchr(par,' ');

    if (colnr>2)
      return;

    if (par==NULL)
      return;

    par_fade[colnr]=atoi(par);

  }

  // if (strcmp_P(event, PSTR("some.event"))==0)
  // {
  //   ...do stuff
  //   return;
  // }


}


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
void loop() {

//   if (random(0,1000)==0)
//   {
//     par_pat=(par_pat+1)%10;
// }

  switch(par_pat)
  {

    //////////////////static
    case 0:
    {
      //fade leds to main color
      for (word led=0; led<LED_COUNT; led++)
      {
        led_fade_to(led, par_col[0][0], par_col[0][1], par_col[0][2], par_fade[0]);
      }
      run_step();
      break;
    }
    //////////////////regea multi smallones
    case 1:
    {
      const int width=1;
      byte c=127;
      for(int i=0; i<LED_COUNT-(width*3)-1; i++)
      {
        for(int s=(i%15); s<LED_COUNT-20; s=s+15)
        {
           led_fade_to(s, 0,0,0,-10);
           led_fade_to(s+1, c,0, 0,-10);
           led_fade_to(s+2, c,c,0,-10);
           led_fade_to(s+3, 0,c,0,-10);
        }

        for (int s=0; s<13; s++)
          run_step();
      }

      break;
    }

    /////////////////////regea small
    case 2:
    {
      const int width=3;
      byte c=127;
      for(int i=0; i<LED_COUNT-(width*3)-1; i++)
      {
        led_set(i, 0,0,0);
        for(int s=i; s<i+width; s++)
        {
           led_set(s+1, c,0,0);
           led_set(s+1+width, c,c,0);
           led_set(s+1+(width*2), 0,c,0);
        }
        if (c>0)
          c=c-5;

        if (c==0)
          c=127;

        for (int s=0; s< 10; s++)
          run_step();
      }
      break;
    }

    ////////////////////////////regea
    case 3:
    {
      const int width=10;
      for(int i=0; i<LED_COUNT-(width*3)-1; i++)
      {
        led_fade_to(i, 0,0,0, -1);
        for(int s=i; s<i+width; s++)
        {
           led_fade_to(s+1, 127,0,0, -1);
           led_fade_to(s+1+width, 127,127,0, -1);
           led_fade_to(s+1+(width*2), 0,127,0, -1);
           run_step();
        }
      }
      break;
    }

    /////////////////////////////////explosion
    case 4:
    {
      for(int i=20; i<((LED_COUNT/2)-1); i++)
      {
        for(int s=0; s<300; s++)
        {
          if (random(i*3)==0)
          {
            byte led=random((LED_COUNT/2)-i,(LED_COUNT/2)+i);
            led_fade_from(led, 147-i,0,0, -20+(i/20));
            led_fade_from(led, 147-i,20,0, -20+(i/20));
            if (random(50)==0)
              led_fade_from(led, 127,00,127, -20);
            run_step();
          }
        }
      }
      break;
    }

    ////////////////////////////////////////////multi radar
    case 5:
    {
      //for(int i=0; i<10000; i++)
      {
        const byte bright=127;
        const byte spd=10;
        const byte fade=5;

        do_radar(
              bright,0,0, //color
              spd, //speed. (skips this many updates)
              fade, fade+5, //minimum and maximum fade speed
              0,(LED_COUNT/2)-1, //start and end lednr
              1, //direction
              true //mix
            );

        do_radar(
              0,0,bright, //color
              spd, //speed. (skips this many updates)
              fade, fade+5, //minimum and maximum fade speed
              (LED_COUNT/2)-20,LED_COUNT-1, //start and end lednr
              1, //direction
              true //mix
            );

        run_step();
       }
       break;
    }

    ////////////////////////////////////////////color radars
    case 6:
    //for(int i=0; i<10000; i++)
    {

      #define R_FADE 20
      #define R_SPEED 10
      #define R_BRIGHT 60


      do_radar(
            R_BRIGHT,0,0, //color
            R_SPEED, //speed. (skips this many updates)
            R_FADE, R_FADE+5, //minimum and maximum fade speed
            0,LED_COUNT-1, //start and end lednr
            0, //direction
            true //mix
          );

      do_radar(
            0,0,R_BRIGHT, //color
            R_SPEED*2, //speed. (skips this many updates)
            R_FADE, R_FADE+5, //minimum and maximum fade speed
            0,LED_COUNT-1, //start and end lednr
            1, //direction
            true //mix
          );

      run_step();
     }
     break;

    //////////////////////////////////////////////////////sparkly
    case 7:
    //for(int i=0; i<10000 ;i++)
    {
      do_sparkle(
        3, //chance there is a sparc? (e.g. lower is more sparcles)
        true, //only leds that are not currently fading?
        0,127,0, // color
        1  //fade speed
      );


      do_glowy(
        0, //chance there is a led glwoing? (e.g. lower is more glowing)
        0,0,1, //rgb range
        0,0,10,
        -20, -20   //min max fade speed
      );

      run_step();
    }
    break;

    ////////////////////////////////////////RG radar
    case 8:
    //for(int i=0; i<10000 ;i++)
    {
      do_radar(
          127,0,0, //color
          14, //speed. (skips this many updates)
          5, 10, //minimum and maximum fade speed
          0,LED_COUNT-1, //start and end lednr
          0, //direction
          true
        );

      do_radar(
          0,127,0, //color
          14, //speed. (skips this many updates)
          5, 10, //minimum and maximum fade speed
          0,LED_COUNT-1, //start and end lednr
           1, //direction,
           true
        );

      run_step();
    }
    break;

    //////////////////////////////////////////////////////fire
    case 9:
    //for(int i=0; i<15000 ;i++)
    {
       do_fire();
       run_step();
       break;
    }


    ////////////////////////////////////////////////////////beat flash
    case 10:
    for(int i=0; i<20000 ;i++)
    {
      if ((i%150)==0)
      {
         int led=random(0,LED_COUNT-30);

         for (int l=led; l<led+20; l++)
         {
             led_fade_from(l, random(100,127), random(100,127), random(100,127),-3);
         }
      }
      do_glowy(
       1, //chance there is a led glwoing? (e.g. lower is more glowing)
       0,0,5, //rgb range
       0,0,5,
       -20, -20   //min max fade speed
      );

      run_step();
    }
    break;

    ////////////////////////////////////////////////////unknown
    default:
      break;
  }
}
