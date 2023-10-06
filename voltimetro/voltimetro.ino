#include <PCD8544.h>
#include <math.h>

#define SIZE 5
#define SAMPLES 20
#define USARTpin 2
#define ACDCpin 12
#define LED(x) (x == 0 ? 8 : (x == 1 ? 9 : (x == 2 ? 10 : (x == 3 ? 11 : -1))))

/* Nota sobre la conversión digital-analógico de las tensiones:
 De la hoja de datos del Arduino UNO se sabe que la resolución
 de los ADC son 1024. Esto quiere decir que el rango de tensiones
 entre -24 V a 24 V hay 1024 intervalos. La función analogRead()
 devuelve un número N entre 0 a 1023. Este valor N se divide entre
 1023 para normalizar el valor entre 0 a 1 y a su vez se multiplica
 por 48. Así M = 48*N/1023 será un número entre 0 a 48. Finalmente,
 se le resta 24 para que el rango de tensiones quede entre -24 V a
 24, puesto que P = M - 24 = -24 + 48*N/1023 quedará entre -24 a 24.
*/

void leer_tensiones(float tensiones[4][SIZE], int AC) {
  // desplaza los valores del array a la derecha para
  // conservar solamente las últimas 5 lecturas
  for (int i = 0; i < 4; i++) {
    for (int j = SIZE-1; j > 0; j--) {
      tensiones[i][j] = tensiones[i][j-1];
    }

  }

  float temp[4];

  // modo AC: muestrea 20 veces la tensión y guarda el valor máximo
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

  // modo DC: guarda la tensión en el instante que realiza la lectura
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

    // calcula el promedio de lecturas para DC
    resultado[i] /= SIZE-1;

    // y lo divide entre sqrt(2) si el modo es AC
    if (AC) resultado[i] /= sqrt(2);
  }
}

void leds(float resultado[4], int AC) {
  for (int i = 0; i < 4; i++) {
    // fijar el umbral en 20 V o 20/sqrt(2)
    // para encender los LEDs
    float threshold = 20.0;

    if (AC) threshold /= sqrt(2);

    if (abs(resultado[i]) > threshold)
      digitalWrite(LED(i), HIGH);

    else
      digitalWrite(LED(i), LOW);
  }
}

static PCD8544 lcd;

// para configurar los pines, serial y
// pantalla LCD
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

  // imprime las lecturas en la pantalla LCD
  for (int i = 0; i < 4; i++) {
    lcd.setCursor(0, i);
    lcd.print("V");
    lcd.print(i);
    lcd.print(": ");
    lcd.print(resultado[i]);
  }

  // envía las lecturas por comunicación serial
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
