#include <Servo.h>

#define NUMBER_OF_FEED_TIME_DATA 5
#define SERVO_PIN 13
enum usage_symbol{feed_time = 0, current_time = 1};
enum usage{up = 0, down = 1, setting = 2, cancel = 3, nothing = 4} button_usage;
enum state{idle = 0, feeding = 1, set = 2, setting_feed_time = 3, setting_current_time = 4};
/********************************************/
class led_clock {
public :
  led_clock () {
    for (int i = 0; i<4; i++){
      pinMode(control_pin[i], OUTPUT);
      digitalWrite(control_pin[i], LOW);
    }
    for (int i = 0; i<7; i++){
      pinMode(led_pin[i], OUTPUT);
      digitalWrite(led_pin[i], HIGH);
    }
  }

  void close_all () {
    for(int i = 0; i < 7; i++) digitalWrite(led_pin[i], HIGH);
    for(int i = 0; i < 4; i++) digitalWrite(control_pin[i], LOW);
  }

  void displayTime(int *time_to_display){
    int number_right = time_to_display[1];
    int number_left = time_to_display[2];
    setLedNumber(0, number_right%10);
    setLedNumber(1, number_right/10);
    setLedNumber(2, number_left%10);
    setLedNumber(3, number_left/10);
  }

  void displayNumber(int number){
    int temp = abs(number);
    for(int i = 0; i < 3; i++){
      if (number < 0) setLedWord (3, minus_sign);
      setLedNumber(i, temp%10);
      temp = temp / 10;
    }
  }

  void showSymbol (int which_led, usage_symbol symbol) {
    if (symbol == feed_time) {
      setLedWord(which_led, display_symbol[0]);
    }else if(symbol == current_time){
      setLedWord(which_led, display_symbol[1]);
    }
  }

  void setLedNumber (int which_led, int number) {
    setLedWord(which_led, display_pin[number]);
  }

  void setLedWord(int which_led, int led_method){
    int mask = 0x01;
    int temp = led_method;
    int pin_value;

    close_all();
    
    digitalWrite(control_pin[which_led], HIGH);
    for(int i = 0; i<7; i++) {
      pin_value = temp & mask;
      if (pin_value == 1) {
        digitalWrite(led_pin[i], LOW);
        delay(1);
      }
      temp = temp >> 1;
    }
  }
private :
  int control_pin[4] = {3, 2, 5, 4};
  int led_pin[7] = {6, 7, 8, 9, 10, 11, 12};
  int display_pin[10] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7c, 0x07, 0x7f, 0x67};
  int display_symbol[2] = {0x71, 0x39};
  int minus_sign = 0x40;
} *led_display;

/********************************************/
class timer {
public :  
  timer() {
    period[0] = 60;
    period[1] = 60;
    period[2] = 24;

    time_display = new led_clock();

    for (int i = 0; i<3; i++) current_time[i] = 0;
    for (int i = 0, temp[3] = {-1, -1, -1}; i < NUMBER_OF_FEED_TIME_DATA; i++) {
      setFeedTime (i, temp);
    }
  }

  void showCurrentTime(){
    update_time(look_time_offset_in_second());
    time_display->displayTime(current_time);
  }

  void setCurrentTime(int *time_input){
    for (int i = 0; i < 3; i++) current_time[i] = time_input[i];   
  }

  bool isFeedTime () {
    bool result = false;
    for (int i = 0; i < NUMBER_OF_FEED_TIME_DATA; i++) {
      result = compareTime(feed_time[i], current_time);
      if (result == true) {
        which_feed_data = i;
        return true;
      }
    }
    if (which_feed_data != -1) which_feed_data = -1;
    return false;
  }

  void setFeedTime (int assign_time_data, int *time_input) {
    for (int i = 0; i < 3; i ++) feed_time[assign_time_data][i] = time_input[i];
  }

  void setTime(int *time_original, int *time_input){
    for(int i = 0; i < 3; i++) time_original[i] = time_input[i];
  }

  int getWhichFeedData () {
    return which_feed_data;
  }
  
  int period[3];
  
private :
  int which_feed_data = -1;
  unsigned long reset_millis = 0;
  int current_time[3];/*0:second 1:minute  2:hour */
  int feed_time[NUMBER_OF_FEED_TIME_DATA][3];
  led_clock *time_display;

  int look_time_offset_in_second() {
    static unsigned long last_update;
    unsigned long current_update = millis();

    int millis_diff = current_update - last_update;
    last_update = current_update - (millis_diff % 1000);  //Should not discard milliseconds which is insufficient to be a second, add to last_update for next calculation
    return (millis_diff / 1000);
  }

  void update_time(int second) {
    current_time[0] += second;
    for(int i = 0; i<3; i++){
      if(current_time[i] >= period[i]) {
        if(i+1 < 3) {
          current_time[i+1] += current_time[i] / period[i];
          current_time[i] %= period[i];
        } else {
          current_time[2] = 0;
        }
      }
    }
  }
  
  bool compareTime (int *feed_time, int *current_time) {
    for (int i = 0; i < 3; i++) {
      if (feed_time[i] != current_time[i]) 
        return false;
    }
    return true;
  }
} *system_timer;
/********************************************/
class button{
public:
  button(int pin, int max_voltage, int min_voltage, usage set_button_usage){
    using_pin = pin;
    button_usage = set_button_usage;
    button_judgment_max_voltage = max_voltage;
    button_judgment_min_voltage = min_voltage;
  }
  bool isPressing(){
    int voltage = analogRead(using_pin);
    if(voltage <= button_judgment_max_voltage && voltage >= button_judgment_min_voltage){
      return true;
    }else{
      return false;
    }
  }

  usage get_usage(){
    return button_usage;
  }

  bool debounce () {
    if (isPressing() == true && pressing == false) {
      pressing = true;
      return true;
    }else if (isPressing() == true && pressing == true) {
      //do nothing
    }else{
      pressing = false;
    }
    return false;
  }

private:
  bool pressing = false;
  int using_pin;
  int button_judgment_max_voltage;
  int button_judgment_min_voltage;
  usage button_usage;
};
/********************************************/
class control_panel{
public:
  control_panel(){
    for (int i = 0; i < 4; i++) {
      control_button[i] = new button(input_pin, button_judgment_max_voltage[i], button_judgment_min_voltage[i], (usage)i);
    }
    led_display = new led_clock();
    current_state = idle;
  }
  usage press_detection(){
    for(int i = 0; i < 4; i++){
      if (control_button[i]->debounce() == true){
        return control_button[i]->get_usage();
      }
    }
    return nothing;
  }

  void setState(state input_state) {
      current_state = input_state;
  }

  void processSetting (timer *system_timer) {
    usage_symbol led_state = feed_time;
    while (true) {
      led_display->showSymbol(3, led_state);
      switch (press_detection()) {
        case up :
        case down :
          led_state = (led_state == feed_time) ? current_time:feed_time;
          break;
        case setting :
          if (led_state == current_time) return setCurrentTime(system_timer);
          else if (led_state == feed_time) return chooseTimeData(system_timer);
          else Serial.println("State not handled");
          break;
        case cancel :
          return;
      }
    }
  }

  void chooseTimeData (timer *system_timer) {
    int assign_time_data = 0;
    while (true) {
      led_display->showSymbol(3, feed_time);
      led_display->setLedNumber(2, assign_time_data);
      switch (press_detection()) {
        case up :
          assign_time_data += 1;
          if (assign_time_data >= NUMBER_OF_FEED_TIME_DATA) assign_time_data = 0;
          break;
        case down :
          assign_time_data -= 1;
          if (assign_time_data < 0) assign_time_data = NUMBER_OF_FEED_TIME_DATA - 1;
          break;
        case setting :
          setFeedTime(system_timer, assign_time_data);
          setFeedAmount(assign_time_data);
          return;
        case cancel :
          return;
      }
    }
  }

  void setFeedAmount (int feed_data) {
    int feedAmountMinus = 0;
    while (true) {
      led_display->displayNumber(feedAmountMinus);
      switch (press_detection()) {
        case up :
          if (feedAmountMinus > -60) feedAmountMinus -= 1;
          break;
        case down :
          if (feedAmountMinus < 0) feedAmountMinus += 1;
          break;
        case setting :
          feed_amount_minus[feed_data] = feedAmountMinus;
          return;
        case cancel :
          feed_amount_minus[feed_data] = 0;
          return;
      }
    }
  }

  int getFeedAmount (int which_feed_data) {
    return feed_amount_minus[which_feed_data];
  }

  void setFeedTime (timer *system_timer, int assign_time_data) {
    int input_time[3] = {0, 0, 0};
    int setting_pointer = 2;
    while (true) {
      led_display->displayTime(input_time);
      switch(press_detection()) {
        case up :
          input_time[setting_pointer] += 1;
          if (input_time[setting_pointer] >= system_timer->period[setting_pointer]) input_time[setting_pointer] = 0;
          break;
        case down :
          input_time[setting_pointer] -= 1;
          if (input_time[setting_pointer] < 0) input_time[setting_pointer] = system_timer->period[setting_pointer] - 1;
          break;
        case setting :
          setting_pointer -= 1;
          if (setting_pointer == 0){
            system_timer->setFeedTime(assign_time_data, input_time);
            return;
          }
          break;
        case cancel :
          for (int i = 0; i < 3; i++) input_time[i] = -1;
          system_timer->setFeedTime(assign_time_data, input_time);
          return;
      }
    }
  }

  void setCurrentTime (timer *system_timer) {
    int input_time[3] = {0, 0, 0};
    int setting_pointer = 2;
    while (true) {
      led_display->displayTime(input_time);
      switch(press_detection()) {
        case up :
          input_time[setting_pointer] += 1;
          if (input_time[setting_pointer] >= system_timer->period[setting_pointer]) input_time[setting_pointer] = 0;
          break;
        case down :
          input_time[setting_pointer] -= 1;
          if (input_time[setting_pointer] < 0) input_time[setting_pointer] = system_timer->period[setting_pointer] - 1;
          break;
        case setting :
          setting_pointer -= 1;
          if (setting_pointer == 0){
            system_timer->setCurrentTime(input_time);
            return;
          }
          break;
        case cancel :
          return;
      }
    }
  }

  int feed_amount_minus[NUMBER_OF_FEED_TIME_DATA] = {0};
private:
  led_clock *led_display;
  state current_state;
  int input_pin = 1;
  button *control_button[4];
  int button_judgment_max_voltage[4] = {260, 345, 517, 1023};
  int button_judgment_min_voltage[4] = {250, 335, 507, 1013};
  //int feed_amount_minus[NUMBER_OF_FEED_TIME_DATA] = {0};
} *panel;
/********************************************/
class feed_servo {
public :
  feed_servo (int supplement_position, int drop_position) {
    servo.attach(SERVO_PIN);
    supplementPosition = supplement_position;
    dropPosition = drop_position;
    rotate_direction = (dropPosition - supplementPosition)/abs(supplementPosition - dropPosition);
    servo.write(drop_position);
    current_position = supplementPosition;
  }

  void rotate (int destination_degree, int cost_time_millis) {
    int offset = current_position - destination_degree;
    int delay_time = cost_time_millis / abs(offset);

    if (offset >= 0) {
      for (int i = current_position; i > destination_degree; i -= 1) {
        servo.write(i);
        delay(delay_time);
      }
    } else if (offset < 0) {
      for (int i = current_position; i < destination_degree; i += 1) {
        servo.write(i);
        delay(delay_time);
      }
    }
    servo.write(destination_degree);
    current_position = destination_degree;
  }
  
  void feed (int feedAmountMinus) {
    delay(1000);
    rotate(supplementPosition - (feedAmountMinus * rotate_direction), 500);
    delay(500);
    rotate(dropPosition, 3000);
    delay(2000);
  }

private :
  int supplementPosition = 0;
  int dropPosition = 0;
  int rotate_direction;
  int current_position = 0;
  Servo servo;
} *servo;
/********************************************/
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  system_timer = new timer();
  led_display = new led_clock();
  panel = new control_panel();
  servo = new feed_servo(47, 108);
}

void loop() {
  // put your main code here, to run repeatedly:
  system_timer->showCurrentTime();
  if (system_timer->isFeedTime()) {
    servo->feed(panel->getFeedAmount(system_timer->getWhichFeedData()));
  }
  button_usage = panel->press_detection();

  if (button_usage == setting){
    panel->setState(set);
    panel->processSetting(system_timer);
  }
}
