/*
 * Seeedstudio Odyssey LED cover code for SAMD21 processor on Odyssey
 * Copyright (C) Sami Viitanen <sami.viitanen@gmail.com>
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>

#define I2C_ADDRESS B1010101
#define STATE_ARRAY_SIZE 31
byte stateArray[STATE_ARRAY_SIZE];
#define STATE_ARRAY_DISABLED 0
#define STATE_ARRAY_MODE 1
#define STATE_ARRAY_MODEMEM 2
#define STATE_ARRAY_USE_COLORS 10
#define STATE_ARRAY_COLOR_START 11
#define BASE_COLOR_COUNT 6
volatile byte receiveRegister = 0x00;

#define NUMPIXELS 47
#define NEOPIXEL_PIN 10
#define DISABLE_PIN 11
#define DELAYVAL 50
#define OUTER_RING_SIZE 24
#define MID_RING_SIZE 16
#define INNER_RING_SIZE 6

unsigned char ORIGO_LED = 40;
unsigned char IN_CIRCLE[] = {42,43,44,45,46,41};
unsigned char MID_CIRCLE[] = {35,34,33,32,31,30,29,28,27,26,25,24,39,38,37,36};
unsigned char OUT_CIRCLE[] = {6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,0,1,2,3,4,5};
unsigned char LED_ORDER[NUMPIXELS];

unsigned int loopCounter;

Adafruit_NeoPixel pixels(NUMPIXELS, NEOPIXEL_PIN, NEO_GRB);

inline void setDisabled(boolean newDisabled) {
  stateArray[STATE_ARRAY_DISABLED] = newDisabled ? 0x01 : 0x00;
  digitalWrite(DISABLE_PIN, newDisabled ? HIGH : LOW);
  loopCounter = 0;
}

void receiveEvent(int howMany) {
  receiveRegister = Wire.read();
  if(howMany == 2) {
    byte address = receiveRegister;
    if(address == 0x00) {
        byte val = Wire.read();
        if(val == 0x00 || val == 0x01) {
          setDisabled(val == 0x01);
        }
    } else if(address < STATE_ARRAY_SIZE) {
      stateArray[address] = Wire.read();
      loopCounter = 0;
    }
  }

  while(1 < Wire.available()) {
    Wire.read();
  }
}

void requestEvent() {
  if(receiveRegister < STATE_ARRAY_SIZE) {
    Wire.write(stateArray[receiveRegister]);
  } else {
    Wire.write(0xFF);
  }
}


inline boolean isDisabled() {
  return stateArray[STATE_ARRAY_DISABLED] != 0x00;
}

inline char getMode() {
  return (char)stateArray[STATE_ARRAY_MODE];
}

inline void setMode(byte val) {
  stateArray[STATE_ARRAY_MODE] = val;
  loopCounter = 0;
}

inline void setModeMem(int index, byte value) {
  stateArray[STATE_ARRAY_MODEMEM + index] = value;
}

inline byte getModeMem(int index) {
  return stateArray[STATE_ARRAY_MODEMEM + index];
}

byte getModeMemNotZero(int index, byte defVal) {
  byte val = getModeMem(index);
  if(val == 0x00) {
    return defVal;
  } else {
    return val;
  }
}


uint32_t getColor(int index, int count, uint8_t brightness) {
  if(stateArray[STATE_ARRAY_USE_COLORS] != 0x00) {
    byte * colorStart = stateArray + STATE_ARRAY_COLOR_START + index * 3;
    byte red = (byte)((((int)colorStart[0])*((int)brightness))>>8);
    byte green = (byte)((((int)colorStart[1])*((int)brightness))>>8);
    byte blue = (byte)((((int)colorStart[2])*((int)brightness))>>8);
    return pixels.Color(red,green,blue);
  } else {
    uint16_t hue = 65536 / count * index;
    return pixels.ColorHSV(hue, 255, brightness);
  }
}

unsigned char FIRE_COLORS[] = { 64, 64, 160,  180,16, 16,  220, 48, 48,  110, 24, 0,  55, 12, 0,  28, 6, 0,  14, 3, 0,  10, 2, 0,  6, 1, 0,  2, 0, 0,  1, 0, 0,  0, 0, 0,  1, 0, 0 };
int FIRE_COLOR_COUNT = sizeof(FIRE_COLORS) / 3;

void fireCircle(int loop, unsigned char * leds, unsigned int ringsize, int shift) {
  for(unsigned int i = 0; i < ringsize; ++i) {
    unsigned char led_i = leds[i];
    int offset = (loop + i) % ringsize;
    if (offset < FIRE_COLOR_COUNT) {
      int colorstart = offset * 3;
      unsigned red = FIRE_COLORS[colorstart] >> shift;
      unsigned green = FIRE_COLORS[colorstart+1] >> shift;
      unsigned blue = FIRE_COLORS[colorstart+2] >> shift;
      pixels.setPixelColor(led_i, pixels.Color(red,green,blue));
    } else if(random(256) > 250) {
      pixels.setPixelColor(led_i, pixels.Color(32 + random(128), 32 + random(128), 128 + random(128)));
    }
  }
}

void fireLoop(unsigned int loop) {
  pixels.clear();

  fireCircle(loop, OUT_CIRCLE, OUTER_RING_SIZE, 0);
  fireCircle(loop, MID_CIRCLE, MID_RING_SIZE, 3);
  fireCircle(loop, IN_CIRCLE, INNER_RING_SIZE, 6);

  int coloroffset = random(4) * 3;
  pixels.setPixelColor(ORIGO_LED, pixels.Color(FIRE_COLORS[coloroffset], FIRE_COLORS[coloroffset+1], FIRE_COLORS[coloroffset+2]));

  pixels.show();
  
  delay(DELAYVAL);
}

// -------------------------------------------------------------

unsigned char ripple_index = 0;

void setCircleColor(unsigned char * leds, int ringsize, int colorIndex, int colorCount, uint8_t brightness) {
  uint32_t color = getColor(colorIndex, colorCount, brightness);
  for(int i = 0; i < ringsize; ++i) {
    pixels.setPixelColor(leds[i], color);
  }
}

void rippleLoop(unsigned int loop) {
  pixels.clear();
  int colorCount = getModeMemNotZero(0, 6);
  int colorIndex = (loop / 52) % colorCount;
  int frame = loop % 52;

  uint8_t brightness = getModeMemNotZero(1, 255);
  int brightnessStep = frame % 13;
  if(brightnessStep <= 6) {
    brightness = brightness >> (6 - brightnessStep);
  } else {
    brightness = brightness >> (1 + brightnessStep - 6);
  }

  int ring = 3 - frame / 13;

  if(ring == 0) {
    brightness = brightness >> 3;
    setCircleColor(OUT_CIRCLE, OUTER_RING_SIZE, colorIndex, colorCount, brightness);
  } else if(ring == 1) {
    brightness = brightness >> 2;
    setCircleColor(MID_CIRCLE, MID_RING_SIZE, colorIndex, colorCount, brightness);
  } else if(ring == 2) {
    brightness = brightness >> 1;
    setCircleColor(IN_CIRCLE, INNER_RING_SIZE, colorIndex, colorCount, brightness);
  } else {
    uint32_t color = getColor(colorIndex, colorCount, brightness);
    pixels.setPixelColor(ORIGO_LED, color);
  }

  pixels.show();
  delay(30);
}

// -------------------------------------------------------------

inline float getProgress() {
  return getModeMem(0) / 255.0;
}

uint32_t mixColors(uint32_t a, uint32_t b) {
  uint32_t c = 0;

  c += (((a >> 24) + (b >> 24)) >> 2) << 24;
  c += ((((a & 0xFF0000) >> 16) + ((b & 0xFF0000) >> 16)) >> 2) << 16;
  c += ((((a & 0xFF00) >> 8) + ((b & 0xFF00) >> 8)) >> 2) << 8;
  c += (((a & 0xFF) + (b & 0xFF)) >> 2);

  return c;
}

void pieCircle(unsigned int ringSize, float progress, unsigned char * leds, uint32_t * colors) {
  int turnOn = (int)floor(ringSize * progress);
  int almostThere = (int)round(ringSize * progress);
  for(unsigned int i = 0; i < ringSize; ++i) {
    if (i < turnOn) {
      pixels.setPixelColor(leds[i], colors[0]);
    } else if (i < almostThere) {
      pixels.setPixelColor(leds[i], colors[1]);
    } else {
      pixels.setPixelColor(leds[i], colors[2]);      
    }
  }
}

void pieChartLoop(int loop) {
  pixels.clear();
  
  float progress = getProgress();
  byte brightness = getModeMemNotZero(1, 255);

  // Blink when ready
  if(progress >= 1.0 && (loop >> 3) % 2 == 1) {
    brightness = 0;
  }

  uint32_t offColor = getColor(0, 3, brightness);
  uint32_t onColor = getColor(1, 3, brightness);
  uint32_t colors[] = { onColor, mixColors(onColor, offColor), offColor };

  pieCircle(OUTER_RING_SIZE, progress, OUT_CIRCLE, colors);
  pieCircle(MID_RING_SIZE, progress, MID_CIRCLE, colors);
  pieCircle(INNER_RING_SIZE, progress, IN_CIRCLE, colors);

  if(progress > 0.66) {
    pixels.setPixelColor(ORIGO_LED, onColor);
  } else if(progress < 0.33) {
    pixels.setPixelColor(ORIGO_LED, offColor);
  } else {
    pixels.setPixelColor(ORIGO_LED, colors[1]);
  }

  pixels.show();
  delay(100);
  
}


// -------------------------------------------------------------

void rainbowCircle(int loop, int ringSize, byte* circle, uint8_t brightness) {
  int offset = loop % ringSize;
  int step = 65536 / ringSize;
  for(int i = 0; i < ringSize; ++i) {
    int ledi = i;
    if (i < offset) {
      ledi = i - offset + ringSize;
    } else {
      ledi = i - offset;
    }
    uint16_t hue = step * i;
    
    pixels.setPixelColor(circle[ledi], pixels.ColorHSV(hue, 255, brightness));
  }
}

void rainbowLoop(int loop) {

  pixels.clear();

  uint8_t brightness = getModeMemNotZero(0, 8);
  if(brightness < 1) {
    brightness = 1;
  }

  rainbowCircle(loop, OUTER_RING_SIZE, OUT_CIRCLE, brightness);
  rainbowCircle(loop, MID_RING_SIZE, MID_CIRCLE, brightness);

  uint16_t hue = loop * 128;
  for(int i = 0; i < INNER_RING_SIZE; ++i) {
    pixels.setPixelColor(IN_CIRCLE[i], pixels.ColorHSV(hue, 255, 0x10));
  }
  pixels.setPixelColor(ORIGO_LED, pixels.ColorHSV(hue, 255, 0x20));

  pixels.show();
  delay(50);
  
}

// -------------------------------------------------------------

void nightSkyLoop(int loop) {
  pixels.clear();

  for(int i = 0; i < NUMPIXELS; ++i) {
    if(random(150) == 0) {
      unsigned char secondary = random(2);
      unsigned char blue = 2 + random(4);

      pixels.setPixelColor(IN_CIRCLE[i], pixels.Color(secondary, secondary, blue));
    }
  }

  pixels.show();
  delay(50);
}

// -------------------------------------------------------------
// setup and loop methods
// -------------------------------------------------------------

void setup() {
  // Clear mem
  for(int i = 0; i < STATE_ARRAY_SIZE; ++i) {
    stateArray[i] = 0x00;
  }
  
  loopCounter = 0;
  
  int filli = -1;
  for(unsigned int i = 0; i < sizeof(OUT_CIRCLE); ++i) {
    LED_ORDER[++filli] = OUT_CIRCLE[i];
  }
  for(unsigned int i = 0; i < sizeof(MID_CIRCLE); ++i) {
    LED_ORDER[++filli] = MID_CIRCLE[i];
  }
  for(unsigned int i = 0; i < sizeof(IN_CIRCLE); ++i) {
    LED_ORDER[++filli] = IN_CIRCLE[i];
  }
  LED_ORDER[++filli] = ORIGO_LED;
  
  pinMode(DISABLE_PIN, OUTPUT);
  setDisabled(false);
  pixels.begin();

  randomSeed(0xDEADBEEF);

  //Wire.setClock(3400000);
  Wire.begin(I2C_ADDRESS); 
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  pixels.clear();
  pixels.setPixelColor(ORIGO_LED, pixels.Color(3, 0, 0));
  pixels.show();
}


void loop() {

  if(isDisabled()) {
    delay(250);
    
  } else {
    char mode = getMode();

    if(mode == 0x00) {
      if(loopCounter < 312) {
        rippleLoop(loopCounter);
      } else if(loopCounter < 10000) {
        fireLoop(loopCounter);
      } else {
        setMode('R');
        setDisabled(true);
      }
    } else if(mode == 'F') {
      fireLoop(loopCounter);
    } else if(mode == 'P') {
      pieChartLoop(loopCounter);
    } else if(mode == 'S') {
      nightSkyLoop(loopCounter);
    } else if(mode == 'B') {
      rainbowLoop(loopCounter);
    } else {
      rippleLoop(loopCounter);
    }
    ++loopCounter;
  }

}

