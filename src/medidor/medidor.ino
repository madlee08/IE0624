#include <TrueRMS.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define LPERIOD 10000    // loop period time in us. In this case 2.0ms
#define ADC_VIN 0       // define the used ADC input channel for voltage
#define ADC_IIN 2       // define the used ADC input channel for current
#define RMS_WINDOW 40   // rms window of 40 samples, means 4 periods @50Hz
#define MODE 1
#define AVG_WINDOW 20

LiquidCrystal_I2C lcd(0x27,16,2);

unsigned long nextLoop;
int acVolt;
int acCurr;
int cnt=0;
float acVoltRange = 5; // peak-to-peak voltage scaled down to 0-5V is 700V (=700/2*sqrt(2) = 247.5Vrms max).
float acCurrRange = 5; // peak-to-peak current scaled down to 0-5V is 5A (=5/2*sqrt(2) = 1.77Arms max).

Power acPower;
Average avgV;
Average avgI;

void setup() {
  // objeto para iniciar el LCD
  lcd.init();
  lcd.clear();
  lcd.backlight();

  // objetos de la clase Average para medir en DC
	avgV.begin(5.00, AVG_WINDOW, ADC_10BIT, CNT_SCAN);
  avgV.start();
  avgI.begin(5.00, AVG_WINDOW, ADC_10BIT, CNT_SCAN);
  avgI.start();

  // objeto de una clase Power para medir en AC
	acPower.begin(acVoltRange, acCurrRange, RMS_WINDOW, ADC_10BIT, BLR_ON, SGL_SCAN);
  acPower.start();
  
	nextLoop = micros() + LPERIOD; // Set the loop timer variable for the next loop interval.
	}

void loop() {
	acVolt = analogRead(ADC_VIN);
	acCurr = analogRead(ADC_IIN);
  
	acPower.update(acVolt, acCurr);
  avgV.update(acVolt);
  avgI.update(acCurr);

	cnt++;

  // PARTE AC
	if(cnt >= 250 && MODE == 0) { // publish every 0.5s
		acPower.publish();

    // tension
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print(acPower.rmsVal1);
    lcd.print("V");

    // corriente
    lcd.setCursor(8,0);
    lcd.print(acPower.rmsVal2);
    lcd.print("A");

    // potencia real
    lcd.setCursor(1,1);
    lcd.print(acPower.realPwr);
    lcd.print("W");

    // factor de potencia
    lcd.setCursor(8,1);
    lcd.print(acPower.pf);
    lcd.print("pf");
		cnt=0;

		acPower.start();
	}

  // PARTE DC
	if(cnt >= 100 and MODE == 1) {
		avgV.publish();
    avgI.publish();

    // tension
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print(avgV.average);
    lcd.print("V");

    // corriente
    lcd.setCursor(8,0);
    lcd.print(avgI.average);
    lcd.print("A");

    // potencia real
    lcd.setCursor(1,1);
    lcd.print(avgV.average*avgI.average);
    lcd.print("W");
		cnt=0;
  }

	while(nextLoop > micros());  // wait until the end of the time interval
	nextLoop += LPERIOD;  // set next loop time to current time + LOOP_PERIOD
}
