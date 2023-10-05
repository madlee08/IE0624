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
#include <math.h>

#define SIZE 5
#define SAMPLES 20
#define USARTpin 2
#define ACDCpin 12
#define LED(x) (x == 0 ? 8 : (x == 1 ? 9 : (x == 2 ? 10 : (x == 3 ? 11 : -1))))


void leer_tensiones(float tensiones[4][SIZE], int AC) {
  for (int i = 0; i < 4; i++) {
    for (int j = SIZE-1; j > 0; j--) {
      tensiones[i][j] = tensiones[i][j-1];
    }

  }

  float temp[4];

  if (AC) {
    for (int i = 0; i < SAMPLES; i++) {
      temp[0] = -24.0 + (analogRead(A0)/1023.0)*48.0;
      temp[1] = -24.0 + (analogRead(A1)/1023.0)*48.0;
      temp[2] = -24.0 + (analogRead(A2)/1023.0)*48.0;
      temp[3] = -24.0 + (analogRead(A3)/1023.0)*48.0;

      for (int j = 0; j < 4; j++){
        if (temp[j] > tensiones[j][0])
          tensiones[j][0] = temp[j];
      }
      delay(10);
    }
  }

  else {
      tensiones[0][0] = -24.0 + (analogRead(A0)/1023.0)*48.0;
      tensiones[1][0] = -24.0 + (analogRead(A1)/1023.0)*48.0;
      tensiones[2][0] = -24.0 + (analogRead(A2)/1023.0)*48.0;
      tensiones[3][0] = -24.0 + (analogRead(A3)/1023.0)*48.0;
      delay(60);
  }
}

void procesar(float resultado[4], float tensiones[4][SIZE], int AC) {
  for (int i = 0; i < 4; i++) {
    for (int j = 1; j < SIZE; j++) {
      resultado[i] += tensiones[i][j];
    }
    resultado[i] /= SIZE-1;

    if (AC) resultado[i] /= sqrt(2);
  }
}

void leds(float resultado[4], int AC) {
  for (int i = 0; i < 4; i++) {
    float threshold = 20.0;

    if (AC) threshold /= sqrt(2);

    if (abs(resultado[i]) > threshold)
      digitalWrite(LED(i), HIGH);

    else
      digitalWrite(LED(i), LOW);
  }
}

static PCD8544 lcd;

void setup() {
  lcd.begin(84, 48);
  Serial.begin(115200);
  pinMode(LED(0) , OUTPUT);
  pinMode(LED(1) , OUTPUT);
  pinMode(LED(2) , OUTPUT);
  pinMode(LED(3) , OUTPUT);
  pinMode(ACDCpin , INPUT_PULLUP);
  pinMode(USARTpin, INPUT_PULLUP);
}

void loop() {
  float tensiones[4][SIZE];
  float resultado[4] = {0.0, 0.0, 0.0, 0.0};

  int AC = digitalRead(ACDCpin);

  leer_tensiones(tensiones, AC);
  procesar(resultado, tensiones, AC);
  leds(resultado, AC);

  for (int i = 0; i < 4; i++) {
    lcd.setCursor(0, i);
    lcd.print("V");
    lcd.print(i);
    lcd.print(": ");
    lcd.print(resultado[i]);
  }

  if (digitalRead(USARTpin)) {
    for (int i = 0; i < 4; i++) {
      Serial.print("V");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(resultado[i]);
      Serial.print("\t");
    }

    Serial.print("\n");
  }
}
