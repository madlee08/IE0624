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

#define SIZE 10
#define ITERATION 20
#include <PCD8544.h>
#include <math.h>


void save_max(float max_volt[4][SIZE], float volt[4]) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < SIZE; j++) {
      max_volt[i][j+1] = max_volt[i][j];
    }

    max_volt[i][0] = -24.0;
  }

  for (int i = 0; i < ITERATION; i++) {
    volt[0] = -24.0 + (analogRead(A0)/1023.0)*48.0;
    volt[1] = -24.0 + (analogRead(A1)/1023.0)*48.0;
    volt[2] = -24.0 + (analogRead(A2)/1023.0)*48.0;
    volt[3] = -24.0 + (analogRead(A3)/1023.0)*48.0;

    for (int j = 0; j < 4; j++){
      if (volt[j] > max_volt[j][0]) {
        max_volt[j][0] = volt[j];
      }
    }
    delay(60);
  }
}

void result(float resultado[4], float max_volt[4][SIZE]) {
  for (int i = 0; i < 4; i++) {
    for (int j = 1; j < SIZE; j++) {
      resultado[i] += max_volt[i][j];
    }
  }
}

static PCD8544 lcd;
const int led[4] = {8, 9, 10, 11};

void setup() {
  // PCD8544-compatible displays may have a different resolution...
  lcd.begin(84, 48);
  Serial.begin(115200);
  pinMode(8 , OUTPUT);
  pinMode(9 , OUTPUT);
  pinMode(10 , OUTPUT);
  pinMode(11 , OUTPUT);
  pinMode(12 , INPUT);
  pinMode(13 , INPUT);

}


void loop() {
  // Just to show the program is alive...
  float volt[4];
  float max_volt[4][SIZE];
  float resultado[4] = {0.0, 0.0, 0.0, 0.0};

  save_max(max_volt, volt);

  int boton1 = digitalRead(12);
  int boton2 = digitalRead(13);
  

  // for (int i = 0; i <4; i++) {
  //   if (valor_adc[i] < -20.0 || 20.0 < valor_adc[i]) {
  //     digitalWrite(led[i], HIGH);
  //   }
  //   else {
  //     digitalWrite(led[i], LOW);
  //   }
  // }

  result(resultado, max_volt);
  // Write a piece of text on the first line...
  for (int i = 0; i < 4; i++) {
  lcd.setCursor(0, i);
  lcd.print("V");
  lcd.print(i);
  lcd.print(": ");
  if (boton1) lcd.print(resultado[i]/9.0/sqrt(2));
    else lcd.print(resultado[i]/9.0);
  }

  if (boton2){
    for (int i = 0; i < 4; i++) {
    Serial.print("V");
    Serial.print(i);
    Serial.print(": ");
    if (boton1) Serial.print(resultado[i]/9.0/sqrt(2));
      else Serial.print(resultado[i]/9.0);
    Serial.print("\t");
    }
    Serial.print("\n");
  }
}


/* EOF - HelloWorld.ino */
