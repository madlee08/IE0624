/*
 * To use this sketch, connect the eight pins from your LCD like thus:
 *
 * Pin 1 -> +3.3V (rightmost, when facing the display head-on)
 * Pin 2 -> Arduino digital pin 3
 * Pin 3 -> Arduino digital pin 4
 * Pin 4 -> Arduino digital pin 5
 * Pin 5 -> Arduino digital pin 7
 * Pin 6 -> Ground
 * Pin 7 -> 10uF capacitor -> Ground
 * Pin 8 -> Arduino digital pin 6
 *
 * Since these LCDs are +3.3V devices, you have to add extra components to
 * connect it to the digital pins of the Arduino (not necessary if you are
 * using a 3.3V variant of the Arduino, such as Sparkfun's Arduino Pro).
 */


#include <PCD8544.h>


// A custom glyph (a smiley)...
static const byte glyph[] = { B00010000, B00110100, B00110000, B00110100, B00010000 };


static PCD8544 lcd;


void setup() {
  // PCD8544-compatible displays may have a different resolution...
  lcd.begin(84, 48);

  // Add the smiley to position "0" of the ASCII table...
  lcd.createChar(0, glyph);
  pinMode(8 , OUTPUT);
  pinMode(9 , OUTPUT);
  pinMode(10 , OUTPUT);
  pinMode(11 , OUTPUT);
}


void loop() {
  // Just to show the program is alive...
  float valor_adc[4];
  valor_adc[0] = -24.0 + (analogRead(A0)/1023.0)*48.0;
  valor_adc[1] = -24.0 + (analogRead(A1)/1023.0)*48.0;
  valor_adc[2] = -24.0 + (analogRead(A2)/1023.0)*48.0;
  valor_adc[3] = -24.0 + (analogRead(A3)/1023.0)*48.0;
  //int valor_adc = analogRead(A0);
  
  int led[4];
  led[0] = 8;
  led[1] = 9;
  led[2] = 10;
  led[3] = 11;

  for (int i = 0; i <4; i++) {
    if (valor_adc[i] < -20.0 || 20.0 < valor_adc[i]) {
      digitalWrite(led[i], HIGH);
    }
    else {
      digitalWrite(led[i], LOW);
    }
  }


  // Write a piece of text on the first line...
  lcd.setCursor(0, 0);
  lcd.print("V1: ");
  lcd.print(valor_adc[0]);


  // Write a piece of text on the first line...
  lcd.setCursor(0, 1);
  lcd.print("V2: ");
  lcd.print(valor_adc[1]);


  // Write a piece of text on the first line...
  lcd.setCursor(0, 2);
  lcd.print("V3: ");
  lcd.print(valor_adc[2]);


  // Write a piece of text on the first line...
  lcd.setCursor(0, 3);
  lcd.print("V4: ");
  lcd.print(valor_adc[3]);

  // Use a potentiometer to set the LCD contrast...
  // short level = map(analogRead(A0), 0, 1023, 0, 127);
  // lcd.setContrast(level);

  delay(5);
}


/* EOF - HelloWorld.ino */
