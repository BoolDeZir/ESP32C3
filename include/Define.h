const uint8_t GPIO_PWM = 8;
const uint8_t GPIO_TEN2 = 4;
const uint8_t GPIO_TEN3 = 5;
const uint8_t CHANNAL_PWM = 0;
const uint8_t RESULT_BITS = 14;
const uint32_t START_FREQ = 2000;
#define POWER_PWM_TEN 1500
#define POWER_TEN_2 1500
#define POWER_TEN_3 1500
const uint8_t STEP_COUNT = (POWER_PWM_TEN/100);
const uint8_t ONEWIREBUS = 9;

const uint8_t BTN_PIN = 13;
const bool BTN_LEVEL = LOW;
const uint8_t ENC_PINA = 0;
const uint8_t ENC_PINB = 1;
enum buttonstate_t : uint8_t { BTN_RELEASED, BTN_PRESSED, BTN_CLICK, BTN_LONGCLICK };

#ifndef POWER_TEN_2
#define POWER_TEN_2 0
#endif
#ifndef POWER_TEN_3
#define POWER_TEN_3 0
#endif
const uint32_t MAX_POWER = (POWER_PWM_TEN + POWER_TEN_2 + POWER_TEN_3);
const uint32_t MIN_POWER =0;
typedef struct
{
  uint8_t number;
  float temp;
} DS18b20;