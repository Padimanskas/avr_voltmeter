/*
  PB0 (5 нога attiny13) DATA - к выводу 14 74HC595
  PB1 (6 нога attiny13) SH_CP - к выводу 11 74HC595
  PB4 (3 нога attiny13) ST_CP - к выводу 12 74HC595
  PB3 (7 нога attiny13) - измеритель с резестивным делетилем.
  10 kOhm на землю и 100 kOhm к измерителю. средння точка делителя к пину микроконтроллера
  лучше использовать опторазвязку, но пока что и так пойдет :)

  74HC595:

  14 DATA передаваемые данные
  11 SH_CP (SHift Clock Pin) - сохраняет в внутренний регистр по переходу от низкого уровня к высокому
  12 ST_CP (STore Clock Pin) - отображает данные в регистре на выводы по переходу от низкого уровня к высокому
  13 OE (OUTPUT ENABLE) включение выходов. Активируется низким уровенем, то есть ставим на минус
  10 CLEAR очистка регистра. Активируется низким уровнем (лучше посадить на плюс чтобы ничего не сбивалось)

  Q0  15
  Q1  1
  Q2  2
  Q3  3
  Q4  4
  Q5  5
  Q6  6
  Q7  7
  Q7' 9

  8 вывод это минус питания, 16 плюс питания

  Для каскада нужно соединить 9 вывод с 14
  все остальные соединяются друг к другу: 11 к 11; 12 к 12
*/

#define DATA_PIN_OUT    PB0      // Данные Data вывод 14 74HC595
#define CLOCK_PIN_OUT   PB1      // Тактирование SH_CP вывод 11 74HC595
#define LATCH_PIN_OUT   PB4      // Защелка ST_CP вывод 12 74HC595
#define MEASUREMENT_TIMEOUT 600  // Время пропуска измерений

unsigned int voltage_value = 0;       // Значение для хранения результата АЦП
char digit_position = 0;              // Позиция текущей цифры
unsigned int digit_value = 0;         // Значение цифры для отображения
unsigned long timer = 0;              // Таймер в миллисекундах
const uint8_t SEGMENTS[] = {
  0x3F, // 0
  0x06, // 1
  0x5B, // 2
  0x4F, // 3
  0x66, // 4
  0x6D, // 5
  0x7D, // 6
  0x07, // 7
  0x7F, // 8
  0x6F, // 9
  0x40  // .
};

void setup() {
  DDRB |= (1 << DATA_PIN_OUT) | (1 << CLOCK_PIN_OUT) | (1 << LATCH_PIN_OUT);
  PORTB = 0x00;
  setup_ADC1();
  setup_timer0();
}

void setup_ADC1() {
  ACSR |= (1 << ACD);                                    // Выключаем аналоговый компаратор
  DIDR0 |= (1 << ADC1D);                                 // Отключаем неиспользуемые цифровые входы
  ADMUX |= (1 << MUX0);                                  // Выбираем вход ADC1 на PB2 (7 нога мк)
  ADCSRA |= (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1);   // АЦП с предделителем на 64
}

void setup_timer0() {
  TCCR0A = (1 << WGM01);                 // Устанавливаем режим CTC (Clear Timer on Compare Match)
  TCCR0B = (1 << CS01) | (1 << CS00);    // Устанавливаем делитель на 64
  OCR0A = 0x2E;                          // Устанавливаем значение для сравнения, чтобы получить 5 миллисекунд
  TIMSK0 = (1 << OCIE0A);                // Включаем прерывание по совпадению с OCR0A
  sei();                                 // Разрешаем глобальные прерывания
}

// Обработчик прерывания по совпадению OCR0A
ISR(TIM0_COMPA_vect) {
  switch (digit_position) {
    case 4: digit_value = 10;                             break; // точка
    case 3: digit_value = (voltage_value % 10);           break; // четвертая цифра
    case 2: digit_value = (voltage_value % 100) / 10;     break; // третья цифра
    case 1: digit_value = (voltage_value % 1000) / 100;   break; // вторая цифра
    case 0: digit_value = (voltage_value % 10000) / 1000; break; // первая цифра
  }

  display_digit(digit_value, digit_position);    // отображаем цифру или точку
  digit_position = (digit_position + 1) % 5;     // сдвигаем номер активной цифры
}

void write_byte(unsigned char data) {
  for (unsigned char i = 0; i < 8; i++) {
    if (data & 0x80) {
      PORTB |= (1 << DATA_PIN_OUT);  // Устанавливаем DATA в 1
    } else {
      PORTB &= ~(1 << DATA_PIN_OUT); // Устанавливаем DATA в 0
    }
    data <<= 1;                      // Сдвигаем биты влево
    PORTB |= (1 << CLOCK_PIN_OUT);   // CLK 1 (передача данных)
    PORTB &= ~(1 << CLOCK_PIN_OUT);  // CLK 0
  }
}

void display_digit(uint8_t digit, uint8_t position) {
  PORTB &= ~(1 << LATCH_PIN_OUT);   // Латчируем данные LOW
  write_byte(1 << position);        // Активируем разряд
  write_byte(~SEGMENTS[digit]);     // Отправляем сегменты
  PORTB |= (1 << LATCH_PIN_OUT);    // Латчируем данные HIGH
}

int main(void) {
  setup();
  while (1) {
    if (timer > millis()) continue;                               // если таймер не досчитал до конца, то пропускаем итерацию
    ADCSRA |= (1 << ADSC);                                        // Запуск АЦП
    while (ADCSRA & (1 << ADSC));                                 // ожидание его завершения
    voltage_value = ((unsigned long)ADC * 5500) / 1024;           // преобразуем данные с АЦП
    timer = millis() + MEASUREMENT_TIMEOUT;                       // устанавливаем новое значение таймера
  }
}
