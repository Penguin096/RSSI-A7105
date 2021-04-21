//Исправно компилится и работает на Arduino IDE 1.8.5 платформа digispark 5V 16MHz
//Извлекает RSSI из приемника HobbyKing HK6S
//Подключение: пин 1 чипа A7105 на вход A0, D0 - выход "аналоговый" на сглаживающую RC цепочку 24k(1uF)24k(1uF)->

#define ADCRSSIMAX 47   // Значение ацп при котором RSSI =100% (47=0,05вольта)
#define ADCRSSIMIN 651   // Значение ацп при котором RSSI =0% (651=0,7вольта по осцилографу срыв связи !!!в datashit=0.8)
#define ADCSTB 1020     // Минимальное значение АЦП строба
#define ADCNRM 837      // максимальное значение АЦП сигнала rssi (837=0,9Вольта)
#define TWAIT 1000      // время ожидания получения rssi (1=110 микросекунд примерно)
#define PWMIN 1000      // Минимальное значение PWM выхода
#define PWMAX 2000      // Максимальное значение PWM выхода
#define LEDMIN 500      // Интервал мигания светодиода при минимальном сигнале ms 
#define LEDMAX 20      // Интервал мигания светодиода при максимальном сигнале ms

int RssiPin = A1;       // сюда подключить пин 1 A7105
int LedPin = 1;        // пин светодиода
int PwmOutPin = 0;      // Выход ШИМ для RC цепочки (аналоговый выход 0-5В)

unsigned long previousMillis = 0;
int ledState = LOW;

long interval;           // interval мерцания светодиодом (milliseconds)

int AdcVal[5];     // масив значений АЦП для детектора положительного фронта импульса
int AdcFil[3];     // масив значений АЦП для детектра среднего из трех семплов
int contrcnt=0;    // счетчик ожидания нового значения RSSI (1=110 микросекунд примерно)
const int numReadings = 16;  // количество отсчетов для усреднеия

int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average (последнее усредненное значение АЦП здесь)

void setup()
{
  analogReference(INTERNAL); // Внутреннее опорное 1.1В (между пином REF и GND поставить конденсатор 

  pinMode(PwmOutPin, OUTPUT);
  pinMode(LedPin, OUTPUT);
}

void loop()
{
  
  AdcVal[0]=AdcVal[1];
  AdcVal[1]=AdcVal[2];
  AdcVal[2]=AdcVal[3];
  AdcVal[3]=AdcVal[4];
  AdcVal[4]=analogRead(RssiPin);

  // если найден положительный фронт импульса на пине 1 A7105
  if ( AdcVal[4]>=ADCSTB && AdcVal[3]>=ADCSTB && AdcVal[2]<ADCSTB && AdcVal[1]<ADCNRM && AdcVal[0]<ADCNRM)
  {
    // Вычисляю новое значение RSSI    
    
    
    contrcnt=0; // сбрасываю счетчик ожидания нового значения rssi
    
    AdcFil[0]=AdcFil[1];
    AdcFil[1]=AdcFil[2];
    AdcFil[2]=AdcVal[0];
    // Фильтрация первый этап - отброс импульсных помех
    // поиск среднего (отбрасываю мин и мах) значения из трех последних семплов (сумирую и отнимаю мин и макс) 
    int adcrssi=(AdcFil[0]+AdcFil[1]+AdcFil[2])-max(AdcFil[0],max(AdcFil[1],AdcFil[2]))-min(AdcFil[0],min(AdcFil[1],AdcFil[2]));

    adcrssi = constrain(adcrssi,ADCRSSIMAX ,ADCRSSIMIN);// Ограничиваем диапазон значений

    // Фильтрация второй этап - усреднение Фильтр от сюда http://www.arduino.cc/en/Tutorial/Smoothing
    // subtract the last reading:
    total = total - readings[readIndex];
    // read from the sensor:
    readings[readIndex] = adcrssi;
    // add the reading to the total:
    total = total + readings[readIndex];
    // advance to the next position in the array:
    readIndex = readIndex + 1;
    // if we're at the end of the array...
    if (readIndex >= numReadings) readIndex = 0;
    // calculate the average:
    average = total / numReadings;

    int msec = map(average,ADCRSSIMIN, ADCRSSIMAX, PWMIN,PWMAX);//Преобразуем диапазон ацп в длительность импульсов PWM
 
    int pwmDac = map(average,ADCRSSIMIN, ADCRSSIMAX, 0,255);//Преобразуем диапазон в заполнение шим 0=0B 255=5B 168=3,3В (при питании контроллера 5В)
    analogWrite(PwmOutPin, pwmDac);

    interval = map(average,ADCRSSIMIN, ADCRSSIMAX, LEDMIN,LEDMAX);//Преобразуем диапазон ацп в длительность импульсов светодиода

  }
  
  // На случай отключения пина 1 чипа A7105 сигна RSSI установим в ноль
  contrcnt++;
  if(contrcnt >= TWAIT)
  {
    contrcnt=0;
    analogWrite(PwmOutPin, 0);          // шим "аналогового выхода" также в ноль
    interval=LEDMIN;  // светодиод на минимальный rssi
  }

  // Мигание светодиодом
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }

    // set the LED with the ledState of the variable:
    digitalWrite(LedPin, ledState);
  }

}


