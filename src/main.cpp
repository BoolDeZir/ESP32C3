#include "Include.h" //файл с включениями
#include "Define.h" //файл с глобальными переменными и константами

TaskHandle_t SetPWM_Task, Encoder_Task, Dallas_Task, LCD_Task; //Обявление хэндлов для задач
QueueHandle_t QPower, QTemp, QBtn, QEnc, QDallas, QPowerLCD, QDallasLCD; //Обявление очередей

uint32_t CheckPower(int _power) { //функция проверки диапазона мощьности
  if(_power >= MIN_POWER && _power <= MAX_POWER) {
    return _power;
  }
  if(_power<=0) {
    return 0;
  }
  if(_power>MAX_POWER) {
    return MAX_POWER;
  } else {
    return 0;
    Serial.println("Недопустимое значение");
  }
}

void IRAM_ATTR encISR() { //функция для обработки энкодера в перерывании
static int encCounter;
static boolean flag, resetFlag;
static uint8_t curState, prevState;
  curState = digitalRead(ENC_PINA) | digitalRead(ENC_PINB) << 1;  // digitalRead хорошо бы заменить чем-нибудь более быстрым
  if (resetFlag && curState == 0b11) {
    if (prevState == 0b10) {
      encCounter = 1;
      xQueueSendFromISR(QEnc, &encCounter, NULL); }
    if (prevState == 0b01) {
      encCounter = -1;
      xQueueSendFromISR(QEnc, &encCounter, NULL); }
    resetFlag = false;
    flag = true;
  }
  if (curState == 0b00) resetFlag = true;
  prevState = curState;
}
void IRAM_ATTR btnISR() { //функция для обработки кнопки в перерывании
  const uint32_t CLICK_TIME = 20; // 20 ms.
  const uint32_t LONGCLICK_TIME = 400; // 500 ms.
  buttonstate_t state;
  static uint32_t lastPressed = 0;
  if (digitalRead(BTN_PIN) == BTN_LEVEL) {
    state = BTN_PRESSED;
    lastPressed = millis();
  } else {
    if(lastPressed != 0) {
    if ((millis() - lastPressed) >= LONGCLICK_TIME) {
      state = BTN_LONGCLICK;    
    }
    else if ((millis() - lastPressed) >= CLICK_TIME) {
      state = BTN_CLICK;     
    }
    else {
      state = BTN_RELEASED;      
    }
    }
    lastPressed = 0;
  }
  xQueueSendFromISR(QBtn, &state, NULL);
}

void vTaskPWM(void *pvParameters) { //задача установки мошьности
  u_int32_t Freq, Power, FreqOLD;
  TickType_t Time_Delay = portMAX_DELAY;
  Serial.println("vTaskPWM Start");
  pinMode(GPIO_TEN2,OUTPUT);
  pinMode(GPIO_TEN3,OUTPUT);
  digitalWrite(GPIO_TEN2, LOW);
  digitalWrite(GPIO_TEN3, LOW);
  if (ledcSetup(CHANNAL_PWM, START_FREQ, RESULT_BITS)==0) {
    Serial.println("LedcSetup Fail");
  }
  else {
    ledcAttachPin(GPIO_PWM, CHANNAL_PWM);
    ledcWrite(CHANNAL_PWM, 0);
    }
  
  while(true) {
    if (xQueueReceive(QPower, &Power, Time_Delay) == pdTRUE) {
      if(Power==0) {
        Freq = 0;
        ledcWrite(CHANNAL_PWM, 0);
        digitalWrite(GPIO_TEN2, LOW);
        digitalWrite(GPIO_TEN3, LOW);
        Time_Delay = portMAX_DELAY;
      }
      if(Power>0 && Power<=MAX_POWER) {
        if(Power<=POWER_PWM_TEN) {
          Freq = (Power*100/POWER_PWM_TEN);
          digitalWrite(GPIO_TEN2, LOW);
          digitalWrite(GPIO_TEN3, LOW);
        } else {
          if(Power>POWER_PWM_TEN && Power<=(POWER_PWM_TEN+POWER_TEN_2) ) {
            Freq = (Power*100/POWER_PWM_TEN)-100;
            digitalWrite(GPIO_TEN2, HIGH);
            digitalWrite(GPIO_TEN3, LOW);
          } else {
            Freq = (Power*100/POWER_PWM_TEN)-200;
            digitalWrite(GPIO_TEN2, HIGH);
            digitalWrite(GPIO_TEN3, HIGH);
          }
        }
      }
    }  
    if(Freq==1) {
      Time_Delay=10;
      ledcChangeFrequency(CHANNAL_PWM, START_FREQ, RESULT_BITS);
      ledcWrite(CHANNAL_PWM, 16383);
      vTaskDelay(pdMS_TO_TICKS(11));
      ledcWrite(CHANNAL_PWM, 0);
      vTaskDelay(pdMS_TO_TICKS(990));
    }
    if(Freq==2) {
      Time_Delay=10;
      ledcChangeFrequency(CHANNAL_PWM, START_FREQ, RESULT_BITS);
      ledcWrite(CHANNAL_PWM, 16383);
      vTaskDelay(pdMS_TO_TICKS(11));
      ledcWrite(CHANNAL_PWM, 0);
      vTaskDelay(pdMS_TO_TICKS(490));
    }      
    if (Freq>=3 && Freq<=100 && FreqOLD != Freq) {
      FreqOLD = Freq;
      Time_Delay = portMAX_DELAY;
      if (Freq==100) ledcWrite(CHANNAL_PWM, 16383);
      else {
        if (ledcChangeFrequency(CHANNAL_PWM, Freq, RESULT_BITS)==0) Serial.println("ledcChangeFrequency Fail");
        else ledcWrite(CHANNAL_PWM, 164*Freq);
      }
    }
    if (Freq>100) {
        Serial.println("Frequency out of range");
        Serial.println(Freq);
        Freq = 0;
        ledcWrite(CHANNAL_PWM, 0);
        digitalWrite(GPIO_TEN2, LOW);
        digitalWrite(GPIO_TEN3, LOW);
    }
  }
}
void vTaskEnc(void *pvParameters) { //задача обработки энкодера
  Serial.println("vTaskEnc Start");
  while(true) {
    static int count=0, step=0;
    static uint32_t ms;
    if (xQueueReceive(QEnc, &count, portMAX_DELAY) == pdTRUE) {
      if(ms==0) {
        ms=millis();
        step+=count*STEP_COUNT;
        step = CheckPower(step);
        xQueueSend(QPower, &step, portMAX_DELAY);
        xQueueSend(QPowerLCD, &step, portMAX_DELAY);
      } else {
        if((millis()-ms) >= 100) {
        step+=count*STEP_COUNT;
        step = CheckPower(step);
      } else {
        step+=count*STEP_COUNT*10;
        step = CheckPower(step);
      }
        xQueueSend(QPower, &step, portMAX_DELAY);
        xQueueSend(QPowerLCD, &step, portMAX_DELAY);
        ms=0;
      }      
    }
  }
}
void vTaskBtn(void *pvParameters) { //задача обработки кнопки
  Serial.println("vTaskBtn Start");
  buttonstate_t state;
  while(true) {
    if (xQueueReceive(QBtn, &state, portMAX_DELAY) == pdTRUE) {
    // Serial.print("Button ");
    switch (state) {
      // case BTN_RELEASED:
      //   Serial.println("released");
      //   break;
      // case BTN_PRESSED:
      //   Serial.println("pressed");
      //   break;
      case BTN_CLICK:
        Serial.println("clicked");
        xQueueSend(QPower, &MIN_POWER, portMAX_DELAY);
        xQueueSend(QPowerLCD, &MIN_POWER, portMAX_DELAY);
        break;
      case BTN_LONGCLICK:
        Serial.println("long clicked");
        xQueueSend(QPower, &MAX_POWER, portMAX_DELAY);
        xQueueSend(QPowerLCD, &MAX_POWER, portMAX_DELAY);
        break;
      }
    }
  }
}
void vTaskDallas(void *pvParameters) { //задача получения значений температур
  Serial.println("vTaskDallas Start");
  OneWire oneWire(ONEWIREBUS);
  DallasTemperature sensors(&oneWire);
  DS18b20 tempSensor[2];
  // uint8_t sensor1[8] = { 0x28, 0xEE, 0x66, 0x4F, 0x25, 0x16, 0x02, 0x88 };
  // uint8_t sensor2[8] = {0x28, 0xEE, 0xFF, 0x02, 0x28, 0x16, 0x01, 0x43 };
  const uint8_t sensor1[8] = { 0x28, 0xEE, 0x48, 0x67, 0x25, 0x16, 0x02, 0xBE };
  const uint8_t sensor2[8] = { 0x28, 0xEE, 0x15, 0x57, 0x28, 0x16, 0x01, 0x7D };
  sensors.begin();
  sensors.setResolution(12); // Точность датчик в битах 9-12 bit
  while(true) {
    sensors.requestTemperatures();
    vTaskDelay(pdMS_TO_TICKS(1000));
    tempSensor[0].number = 1;
    tempSensor[0].temp = sensors.getTempC(sensor1);
    tempSensor[1].number = 2;
    tempSensor[1].temp = sensors.getTempC(sensor2);    
    for(int i=0; i<2; i++) {
      xQueueSend(QDallasLCD, &tempSensor[i], portMAX_DELAY);
      xQueueSend(QDallas, &tempSensor[i], portMAX_DELAY);}
  }
}
void vTaskLCD(void *pvParameters){ //задача вывода информации на дисплей
  #define FLIP
  const uint8_t BL_PIN = 11;
  ST7735S<6, 7, 10> lcd;
  ledcSetup(5, 1000, 8);
  ledcAttachPin(BL_PIN, 5);
  ledcWrite(5, 64);
  SPI.begin(2, 12, 3, 7);
  lcd.begin();
#ifdef FLIP
  lcd.flip(true);
#endif
  lcd.print(0, 0, "Power:", lcd.GREEN, 0);
  lcd.print(0, 30, "Temp1:  0", lcd.GREEN, 0);
  lcd.print(0, 60, "Temp2:  0", lcd.GREEN, 0);
  while(true)
  {
    static u_int32_t Power=0;
    static DS18b20 temperature;
    static bool flag=false;
    if(Power==0 && !flag) {
      lcd.print(60, 0, "      ", lcd.GREEN, 0);
      lcd.print(60, 0, "Pause", lcd.GREEN, 0);
      flag=true;      
    }
    if (xQueueReceive(QPowerLCD, &Power, 0) == pdTRUE) {
      char str[5];
      sprintf(str, "%u",Power);
      lcd.print(60, 0, "      ", lcd.GREEN, 0);
      lcd.print(60, 0, str, lcd.GREEN, 0);
      flag=false;
    }
    if (xQueueReceive(QDallasLCD, &temperature, 0) == pdTRUE) {
      char str[5];
      switch (temperature.number)
      {
      case 1:
        sprintf(str, "%.1f",temperature.temp);
        lcd.print(60, 30, "      ", lcd.GREEN, 0);
        lcd.print(60, 30, str, lcd.GREEN, 0);
        break;
      case 2:
        sprintf(str, "%.1f",temperature.temp);
        lcd.print(60, 60, "      ", lcd.GREEN, 0);
        lcd.print(60, 60, str, lcd.GREEN, 0);
        break;
      default:
        break;
      }
    }    
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
void setup() {
  /*Обявление сериал порт*/
Serial.begin(115200);
Serial.println("Main Setup Start");
/*Назначение пинов ввода для энкодеера и кнопки*/
pinMode(BTN_PIN,INPUT_PULLUP);
pinMode(ENC_PINA,INPUT_PULLUP);
pinMode(ENC_PINB,INPUT_PULLUP);
attachInterrupt(BTN_PIN, btnISR, CHANGE);
attachInterrupt(ENC_PINA, encISR, CHANGE);
attachInterrupt(ENC_PINB, encISR, CHANGE);
/*Создание очередей*/
if((QPower = xQueueCreate(1,sizeof(u_int32_t))) == NULL) Serial.println("QueueCreate Fail");
if((QBtn = xQueueCreate(1,sizeof(u_int32_t))) == NULL) Serial.println("QueueCreate Fail");
if((QEnc = xQueueCreate(1,sizeof(u_int32_t))) == NULL) Serial.println("QueueCreate Fail");
if((QDallas = xQueueCreate(2,sizeof(DS18b20))) == NULL) Serial.println("QueueCreate Fail");
if((QPowerLCD = xQueueCreate(1,sizeof(u_int32_t))) == NULL) Serial.println("QueueCreate Fail");
if((QDallasLCD = xQueueCreate(2,sizeof(DS18b20))) == NULL) Serial.println("QueueCreate Fail");
/*Создание задач*/
if(xTaskCreate(vTaskPWM, "PWM", 2048, NULL, 1, &SetPWM_Task) != pdPASS) {
  Serial.println("TaskCreate(vTaskPWM) Fail");}
if(xTaskCreate(vTaskEnc, "Encoder", 1024, NULL, 1, &Encoder_Task) != pdPASS) {
  Serial.println("TaskCreate(vTaskEnc) Fail");}
if(xTaskCreate(vTaskBtn, "Button", 1024, NULL, 1, &Encoder_Task) != pdPASS) {
  Serial.println("TaskCreate(vTaskBtn) Fail");}
if(xTaskCreate(vTaskDallas, "DS18b20", 1024, NULL, 1, &Dallas_Task) != pdPASS) {
  Serial.println("TaskCreate(vTaskDallas) Fail");}
if(xTaskCreate(vTaskLCD, "LCD", 2048, NULL, 1, &LCD_Task) != pdPASS) {
  Serial.println("TaskCreate(vTaskLCD) Fail");}
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1));
}