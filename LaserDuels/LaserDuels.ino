/*
  Laser Duels - Arduino Mega 2560

  Hardware is intentionally disabled until the as-built servo endpoints,
  phototransistor polarity/threshold, and laser-driver polarity are confirmed.
  See HARDWARE_CONFIGURATION_CONFIRMED and the configuration section below.

  Required libraries:
    - Servo (included with the Arduino AVR board package)
    - LiquidCrystal (included with Arduino)
    - Optional LiquidCrystal_I2C when LCD_USE_I2C is set to 1
*/

// ---------------------------------------------------------------------------
// Chunk 1 of 4: Libraries, hardware configuration, data types, and input scan
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define LCD_USE_I2C 1

#if LCD_USE_I2C
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#endif

namespace Config {
  // Change to true only after reviewing the wiring and calibrating hardware.
  const bool HARDWARE_CONFIGURATION_CONFIRMED = false;

  const uint8_t LCD_RS = 22;
  const uint8_t LCD_ENABLE = 23;
  const uint8_t LCD_D4 = 24;
  const uint8_t LCD_D5 = 25;
  const uint8_t LCD_D6 = 26;
  const uint8_t LCD_D7 = 27;
#if LCD_USE_I2C
  const uint8_t LCD_I2C_ADDRESS = 0x27;  // Replace with the address you verified.
#endif

  const uint8_t PLAYER_COUNT = 2;
  const uint8_t KEYPAD_ROWS = 4;
  const uint8_t KEYPAD_COLS = 3;
  // Adafruit 3x4 keypad connector order: COL2, ROW1, COL1, ROW4,
  // COL3, ROW3, ROW2. Pins remain wired left-to-right in ascending order.
  const uint8_t PLAYER_1_ROW_PINS[KEYPAD_ROWS] = {23, 28, 27, 25};
  const uint8_t PLAYER_1_COL_PINS[KEYPAD_COLS] = {24, 22, 26};
  const uint8_t PLAYER_2_ROW_PINS[KEYPAD_ROWS] = {35, 40, 39, 37};
  const uint8_t PLAYER_2_COL_PINS[KEYPAD_COLS] = {36, 34, 38};
  const char KEYPAD_LAYOUT[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}
  };

  const uint8_t BUTTON_PINS[PLAYER_COUNT] = {46, 47};
  const uint8_t SERVO_PINS[PLAYER_COUNT] = {8, 9};
  const uint8_t LASER_DRIVER_PINS[PLAYER_COUNT] = {10, 11};
  const uint8_t SENSOR_PINS[PLAYER_COUNT] = {A0, A1};
  const uint8_t BUZZER_PIN = 12;

  // Calibrate each servo mechanically before enabling the game.
  const uint8_t SERVO_REAR_DEGREES[PLAYER_COUNT] = {90, 90};
  const uint8_t SERVO_FACING_DEGREES[PLAYER_COUNT] = {90, 90};

  // Set these only after measuring the real circuits.
  const bool LASER_DRIVER_ACTIVE_HIGH = true;
  const bool SENSOR_BRIGHTER_IS_HIGHER = true;
  const int16_t SENSOR_HIT_DELTA = 120;
  const uint16_t SENSOR_REARM_DELTA = 50;

  const uint32_t BUTTON_DEBOUNCE_MS = 25;
  const uint32_t KEYPAD_DEBOUNCE_MS = 25;
  const uint32_t SERVO_TURN_MS = 1000;
  const uint32_t SERVO_COOLDOWN_MS = 3000;
  const uint32_t LASER_PULSE_MS = 250;
  const uint32_t SENSOR_SAMPLE_MS = 5;
  const uint32_t SENSOR_CONFIRM_MS = 20;
  const uint32_t SENSOR_CALIBRATION_MS = 1800;
  const uint32_t SENSOR_CALIBRATION_SAMPLE_MS = 10;
  const uint32_t SIMULTANEOUS_HIT_WINDOW_MS = 100;
  const uint32_t ROUND_RESULT_MS = 1500;
  const uint32_t QUESTION_PAGE_MS = 1800;
  const uint32_t DISPLAY_REFRESH_MS = 100;
  const uint32_t FEEDBACK_MS = 1000;
  const uint8_t SCORE_TO_WIN = 5;
  const uint8_t MAX_ANSWER_DIGITS = 6;
  const uint8_t ARITHMETIC_WEIGHT_PERCENT = 50;
  const uint16_t SENSOR_MAX_CALIBRATION_SPREAD = 180;
}

#if LCD_USE_I2C
LiquidCrystal_I2C lcd(Config::LCD_I2C_ADDRESS, 16, 2);
#else
LiquidCrystal lcd(Config::LCD_RS, Config::LCD_ENABLE,
                  Config::LCD_D4, Config::LCD_D5,
                  Config::LCD_D6, Config::LCD_D7);
#endif

enum GameMode : uint8_t {
  MODE_BOOT,
  MODE_CALIBRATING,
  MODE_ACTIVE,
  MODE_ROUND_RESULT,
  MODE_MATCH_WON,
  MODE_ERROR
};

enum ServoPhase : uint8_t {
  SERVO_READY,
  SERVO_TURNING,
  SERVO_COOLDOWN
};

struct PlayerInput {
  char answer[Config::MAX_ANSWER_DIGITS + 1];
  uint8_t answerLength;
};

struct ServoState {
  Servo motor;
  ServoPhase phase;
  uint32_t phaseStarted;
};

struct SensorState {
  int16_t baseline;
  uint16_t calibrationMin;
  uint16_t calibrationMax;
  uint32_t calibrationSum;
  uint16_t calibrationSamples;
  bool armed;
  bool candidate;
  uint32_t candidateStarted;
};

struct Question {
  char prompt[48];
  int32_t answer;
};

struct ButtonState {
  uint8_t pin;
  bool rawState;
  bool stableState;
  uint32_t changedAt;
};

class MatrixKeypad {
 public:
  MatrixKeypad(const uint8_t *rowPins, const uint8_t *colPins)
      : rowIndex_(0),
        keyCount_(0),
        sampledKey_('\0'),
        candidateKey_('\0'),
        stableKey_('\0'),
        candidateStarted_(0),
        eventKey_('\0') {
    for (uint8_t i = 0; i < Config::KEYPAD_ROWS; ++i) {
      rowPins_[i] = rowPins[i];
    }
    for (uint8_t i = 0; i < Config::KEYPAD_COLS; ++i) {
      colPins_[i] = colPins[i];
    }
  }

  void begin() {
    for (uint8_t i = 0; i < Config::KEYPAD_ROWS; ++i) {
      pinMode(rowPins_[i], OUTPUT);
      digitalWrite(rowPins_[i], HIGH);
    }
    for (uint8_t i = 0; i < Config::KEYPAD_COLS; ++i) {
      pinMode(colPins_[i], INPUT_PULLUP);
    }
  }

  char service(uint32_t now) {
    eventKey_ = '\0';

    for (uint8_t i = 0; i < Config::KEYPAD_ROWS; ++i) {
      digitalWrite(rowPins_[i], HIGH);
    }
    digitalWrite(rowPins_[rowIndex_], LOW);

    for (uint8_t col = 0; col < Config::KEYPAD_COLS; ++col) {
      if (digitalRead(colPins_[col]) == LOW) {
        ++keyCount_;
        sampledKey_ = Config::KEYPAD_LAYOUT[rowIndex_][col];
      }
    }

    ++rowIndex_;
    if (rowIndex_ >= Config::KEYPAD_ROWS) {
      rowIndex_ = 0;
      const char matrixKey = (keyCount_ == 1) ? sampledKey_ : '\0';
      keyCount_ = 0;
      sampledKey_ = '\0';
      updateDebounce(matrixKey, now);
    }

    const char result = eventKey_;
    eventKey_ = '\0';
    return result;
  }

 private:
  uint8_t rowPins_[Config::KEYPAD_ROWS];
  uint8_t colPins_[Config::KEYPAD_COLS];
  uint8_t rowIndex_;
  uint8_t keyCount_;
  char sampledKey_;
  char candidateKey_;
  char stableKey_;
  uint32_t candidateStarted_;
  char eventKey_;

  void updateDebounce(char key, uint32_t now) {
    if (key != candidateKey_) {
      candidateKey_ = key;
      candidateStarted_ = now;
      return;
    }

    if (key != stableKey_ &&
        static_cast<uint32_t>(now - candidateStarted_) >=
            Config::KEYPAD_DEBOUNCE_MS) {
      stableKey_ = key;
      if (stableKey_ != '\0') {
        eventKey_ = stableKey_;
      }
    }
  }
};

MatrixKeypad player1Keypad(Config::PLAYER_1_ROW_PINS,
                           Config::PLAYER_1_COL_PINS);
MatrixKeypad player2Keypad(Config::PLAYER_2_ROW_PINS,
                           Config::PLAYER_2_COL_PINS);
MatrixKeypad *keypads[Config::PLAYER_COUNT] = {
  &player1Keypad, &player2Keypad
};

PlayerInput playerInputs[Config::PLAYER_COUNT] = {};
ServoState servos[Config::PLAYER_COUNT];
SensorState sensors[Config::PLAYER_COUNT] = {};
ButtonState buttons[Config::PLAYER_COUNT] = {};

GameMode gameMode = MODE_BOOT;
uint8_t scores[Config::PLAYER_COUNT] = {0, 0};
bool laserOn[Config::PLAYER_COUNT] = {false, false};
uint32_t laserStarted[Config::PLAYER_COUNT] = {0, 0};
bool sensorEvents[Config::PLAYER_COUNT] = {false, false};

Question currentQuestion = {};
char previousPrompt[sizeof(currentQuestion.prompt)] = "";
int8_t previousTriviaIndex = -1;

bool pendingHit = false;
uint8_t pendingShooter = 0;
uint32_t pendingHitAt = 0;
uint32_t roundResultStarted = 0;
uint8_t roundWinner = 0;

bool feedbackActive = false;
char feedbackText[17] = "";
uint32_t feedbackStarted = 0;
uint32_t feedbackDuration = 0;

uint32_t calibrationStarted = 0;
uint32_t lastCalibrationSample = 0;
uint32_t lastSensorSample = 0;
uint32_t lastQuestionPage = 0;
uint32_t lastDisplayUpdate = 0;
uint8_t questionPage = 0;

char previousDisplayLine0[17] = "";
char previousDisplayLine1[17] = "";
char configurationError[17] = "CONFIG ERROR";

bool elapsed(uint32_t now, uint32_t started, uint32_t duration) {
  return static_cast<uint32_t>(now - started) >= duration;
}

void setLaser(uint8_t player, bool enabled) {
  const uint8_t activeLevel =
      Config::LASER_DRIVER_ACTIVE_HIGH ? HIGH : LOW;
  const uint8_t inactiveLevel =
      Config::LASER_DRIVER_ACTIVE_HIGH ? LOW : HIGH;

  digitalWrite(Config::LASER_DRIVER_PINS[player],
               enabled ? activeLevel : inactiveLevel);
  laserOn[player] = enabled;
  if (enabled) {
    laserStarted[player] = millis();
  }
}

void allLasersOff() {
  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    setLaser(player, false);
  }
}

void setFeedback(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vsnprintf(feedbackText, sizeof(feedbackText), format, args);
  va_end(args);
  feedbackText[16] = '\0';
  feedbackStarted = millis();
  feedbackDuration = Config::FEEDBACK_MS;
  feedbackActive = true;
}

void setLongFeedback(const char *text, uint32_t duration) {
  snprintf(feedbackText, sizeof(feedbackText), "%s", text);
  feedbackStarted = millis();
  feedbackDuration = duration;
  feedbackActive = true;
}

void clearFeedback() {
  feedbackActive = false;
  feedbackText[0] = '\0';
}

void initializeButton(ButtonState &button, uint8_t pin, uint32_t now) {
  button.pin = pin;
  pinMode(pin, INPUT_PULLUP);
  button.rawState = digitalRead(pin) == HIGH;
  button.stableState = button.rawState;
  button.changedAt = now;
}

bool serviceButton(ButtonState &button, uint32_t now) {
  const bool raw = digitalRead(button.pin) == HIGH;
  if (raw != button.rawState) {
    button.rawState = raw;
    button.changedAt = now;
  }

  if (button.stableState != button.rawState &&
      elapsed(now, button.changedAt, Config::BUTTON_DEBOUNCE_MS)) {
    button.stableState = button.rawState;
    return !button.stableState;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Chunk 2 of 4: Display, question selection, and answer entry
// ---------------------------------------------------------------------------

const char *const triviaPrompts[] = {
  "Minutes in hour?",
  "Sides on hexagon?",
  "Weeks in year?",
  "US states?",
  "Leap-year days?",
  "Sum triangle angles?",
  "Largest digit?",
  "Planets in Solar Sys?"
};

const int32_t triviaAnswers[] = {60, 6, 52, 50, 366, 180, 9, 8};
const uint8_t TRIVIA_COUNT =
    sizeof(triviaAnswers) / sizeof(triviaAnswers[0]);

void writeDisplayLine(uint8_t row, const char *text,
                      char previousLine[17]) {
  char line[17];
  memset(line, ' ', 16);
  line[16] = '\0';
  const size_t textLength = strlen(text);
  const size_t copyLength = (textLength < 16) ? textLength : 16;
  memcpy(line, text, copyLength);

  if (memcmp(line, previousLine, sizeof(line)) != 0) {
    lcd.setCursor(0, row);
    lcd.print(line);
    memcpy(previousLine, line, sizeof(line));
  }
}

void clearDisplayCache() {
  previousDisplayLine0[0] = '\0';
  previousDisplayLine1[0] = '\0';
}

void drawDisplay(uint32_t now) {
  if (!elapsed(now, lastDisplayUpdate, Config::DISPLAY_REFRESH_MS)) {
    return;
  }
  lastDisplayUpdate = now;

  char line0[17];
  char line1[17];

  switch (gameMode) {
    case MODE_BOOT:
      snprintf(line0, sizeof(line0), "LASER DUELS");
      snprintf(line1, sizeof(line1), "Starting...");
      break;

    case MODE_CALIBRATING:
      snprintf(line0, sizeof(line0), "LASER DUELS");
      snprintf(line1, sizeof(line1), "Calibrating...");
      break;

    case MODE_ACTIVE:
      snprintf(line0, sizeof(line0), "P1:%u      P2:%u",
               static_cast<unsigned int>(scores[0]),
               static_cast<unsigned int>(scores[1]));
      if (feedbackActive &&
          !elapsed(now, feedbackStarted, feedbackDuration)) {
        snprintf(line1, sizeof(line1), "%s", feedbackText);
      } else {
        feedbackActive = false;
        const size_t promptLength = strlen(currentQuestion.prompt);
        const uint8_t pageCount =
            (promptLength <= 16) ? 1 :
            static_cast<uint8_t>((promptLength + 15) / 16);
        const size_t start =
            static_cast<size_t>(questionPage % pageCount) * 16;
        for (uint8_t i = 0; i < 16; ++i) {
          const size_t promptIndex = start + i;
          line1[i] = (promptIndex < promptLength)
                         ? currentQuestion.prompt[promptIndex]
                         : ' ';
        }
        line1[16] = '\0';
      }
      break;

    case MODE_ROUND_RESULT:
      snprintf(line0, sizeof(line0), "P1:%u      P2:%u",
               static_cast<unsigned int>(scores[0]),
               static_cast<unsigned int>(scores[1]));
      if (feedbackActive &&
          !elapsed(now, feedbackStarted, feedbackDuration)) {
        snprintf(line1, sizeof(line1), "%s", feedbackText);
      } else {
        snprintf(line1, sizeof(line1), "Next question");
      }
      break;

    case MODE_MATCH_WON:
      snprintf(line0, sizeof(line0), "P1:%u      P2:%u",
               static_cast<unsigned int>(scores[0]),
               static_cast<unsigned int>(scores[1]));
      snprintf(line1, sizeof(line1), "P%u WINS  # NEW",
               static_cast<unsigned int>(roundWinner + 1));
      break;

    case MODE_ERROR:
      snprintf(line0, sizeof(line0), "SETUP REQUIRED");
      snprintf(line1, sizeof(line1), "%s", configurationError);
      break;
  }

  writeDisplayLine(0, line0, previousDisplayLine0);
  writeDisplayLine(1, line1, previousDisplayLine1);
}

void advanceQuestionPage(uint32_t now) {
  if (gameMode != MODE_ACTIVE ||
      !elapsed(now, lastQuestionPage, Config::QUESTION_PAGE_MS)) {
    return;
  }
  lastQuestionPage = now;
  const size_t promptLength = strlen(currentQuestion.prompt);
  const uint8_t pageCount =
      (promptLength <= 16) ? 1 :
      static_cast<uint8_t>((promptLength + 15) / 16);
  questionPage = static_cast<uint8_t>((questionPage + 1) % pageCount);
}

void chooseTriviaQuestion() {
  uint8_t index = static_cast<uint8_t>(random(TRIVIA_COUNT));
  if (TRIVIA_COUNT > 1 && index == previousTriviaIndex) {
    index = static_cast<uint8_t>((index + 1) % TRIVIA_COUNT);
  }
  previousTriviaIndex = static_cast<int8_t>(index);
  snprintf(currentQuestion.prompt, sizeof(currentQuestion.prompt), "%s",
           triviaPrompts[index]);
  currentQuestion.answer = triviaAnswers[index];
}

void chooseArithmeticQuestion() {
  const uint8_t operation = static_cast<uint8_t>(random(4));
  int32_t left = 0;
  int32_t right = 0;
  int32_t answer = 0;
  char prompt[sizeof(currentQuestion.prompt)];

  switch (operation) {
    case 0:
      left = random(0, 21);
      right = random(0, 21);
      answer = left + right;
      snprintf(prompt, sizeof(prompt), "%ld+%ld=?",
               static_cast<long>(left), static_cast<long>(right));
      break;

    case 1:
      left = random(0, 21);
      right = random(0, 21);
      if (right > left) {
        const int32_t swapValue = left;
        left = right;
        right = swapValue;
      }
      answer = left - right;
      snprintf(prompt, sizeof(prompt), "%ld-%ld=?",
               static_cast<long>(left), static_cast<long>(right));
      break;

    case 2:
      left = random(0, 13);
      right = random(0, 13);
      answer = left * right;
      snprintf(prompt, sizeof(prompt), "%ld*%ld=?",
               static_cast<long>(left), static_cast<long>(right));
      break;

    default: {
      right = random(1, 13);
      answer = random(0, 13);
      left = right * answer;
      snprintf(prompt, sizeof(prompt), "%ld/%ld=?",
               static_cast<long>(left), static_cast<long>(right));
      break;
    }
  }

  snprintf(currentQuestion.prompt, sizeof(currentQuestion.prompt), "%s",
           prompt);
  currentQuestion.answer = answer;
}

void chooseQuestion() {
  const bool useArithmetic =
      random(0, 100) < Config::ARITHMETIC_WEIGHT_PERCENT;

  if (useArithmetic) {
    chooseArithmeticQuestion();
    for (uint8_t attempt = 0;
         attempt < 10 &&
         strcmp(currentQuestion.prompt, previousPrompt) == 0;
         ++attempt) {
      chooseArithmeticQuestion();
    }
  } else {
    chooseTriviaQuestion();
  }

  snprintf(previousPrompt, sizeof(previousPrompt), "%s",
           currentQuestion.prompt);
  questionPage = 0;
  lastQuestionPage = millis();
}

void clearPlayerAnswer(uint8_t player) {
  playerInputs[player].answerLength = 0;
  playerInputs[player].answer[0] = '\0';
}

void triggerLaser(uint8_t player, uint32_t now) {
  if (laserOn[player]) {
    setFeedback("P%u LASER BUSY",
                static_cast<unsigned int>(player + 1));
    return;
  }
  setLaser(player, true);
  laserStarted[player] = now;
  tone(Config::BUZZER_PIN, player == 0 ? 1200 : 1500, 55);
}

void submitAnswer(uint8_t player, uint32_t now) {
  if (playerInputs[player].answerLength == 0) {
    setFeedback("P%u NO ANSWER",
                static_cast<unsigned int>(player + 1));
    return;
  }

  int32_t value = 0;
  for (uint8_t i = 0; i < playerInputs[player].answerLength; ++i) {
    value = value * 10 +
            (playerInputs[player].answer[i] - '0');
  }

  clearPlayerAnswer(player);
  if (value == currentQuestion.answer) {
    setFeedback("P%u CORRECT",
                static_cast<unsigned int>(player + 1));
    triggerLaser(player, now);
  } else {
    setFeedback("P%u INCORRECT",
                static_cast<unsigned int>(player + 1));
    tone(Config::BUZZER_PIN, 350, 80);
  }
}

void beginSensorCalibration(uint32_t now);

void processKey(uint8_t player, char key, uint32_t now) {
  if (gameMode == MODE_MATCH_WON) {
    if (key == '#') {
      clearFeedback();
      scores[0] = 0;
      scores[1] = 0;
      previousTriviaIndex = -1;
      previousPrompt[0] = '\0';
      beginSensorCalibration(now);
    }
    return;
  }

  if (gameMode != MODE_ACTIVE) {
    return;
  }

  if (key >= '0' && key <= '9') {
    PlayerInput &input = playerInputs[player];
    if (input.answerLength >= Config::MAX_ANSWER_DIGITS) {
      setFeedback("P%u ANSWER FULL",
                  static_cast<unsigned int>(player + 1));
      return;
    }
    input.answer[input.answerLength++] = key;
    input.answer[input.answerLength] = '\0';
    setFeedback("P%u: %s", static_cast<unsigned int>(player + 1),
                input.answer);
    return;
  }

  if (key == '*') {
    clearPlayerAnswer(player);
    setFeedback("P%u CLEARED",
                static_cast<unsigned int>(player + 1));
  } else if (key == '#') {
    submitAnswer(player, now);
  }
}

// ---------------------------------------------------------------------------
// Chunk 3 of 4: Servo, laser, and phototransistor controllers
// ---------------------------------------------------------------------------

void parkServos(uint32_t now) {
  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    servos[player].motor.write(Config::SERVO_REAR_DEGREES[player]);
    servos[player].phase = SERVO_COOLDOWN;
    servos[player].phaseStarted = now;
  }
}

void serviceServos(uint32_t now) {
  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    ServoState &servo = servos[player];
    if (servo.phase == SERVO_TURNING &&
        elapsed(now, servo.phaseStarted, Config::SERVO_TURN_MS)) {
      servo.motor.write(Config::SERVO_REAR_DEGREES[player]);
      servo.phase = SERVO_COOLDOWN;
      servo.phaseStarted = now;
    } else if (servo.phase == SERVO_COOLDOWN &&
               elapsed(now, servo.phaseStarted,
                       Config::SERVO_COOLDOWN_MS)) {
      servo.phase = SERVO_READY;
    }
  }
}

void startServoTurn(uint8_t player, uint32_t now) {
  if (gameMode != MODE_ACTIVE || servos[player].phase != SERVO_READY) {
    return;
  }
  servos[player].motor.write(Config::SERVO_FACING_DEGREES[player]);
  servos[player].phase = SERVO_TURNING;
  servos[player].phaseStarted = now;
  tone(Config::BUZZER_PIN, 900, 45);
}

void serviceLasers(uint32_t now) {
  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    if (laserOn[player] &&
        elapsed(now, laserStarted[player], Config::LASER_PULSE_MS)) {
      setLaser(player, false);
    }
  }
}

void startSensorCalibration(uint32_t now) {
  allLasersOff();
  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    sensors[player].calibrationMin = 1023;
    sensors[player].calibrationMax = 0;
    sensors[player].calibrationSum = 0;
    sensors[player].calibrationSamples = 0;
    sensors[player].armed = false;
    sensors[player].candidate = false;
  }
  calibrationStarted = now;
  lastCalibrationSample = now - Config::SENSOR_CALIBRATION_SAMPLE_MS;
  gameMode = MODE_CALIBRATING;
  clearFeedback();
  clearDisplayCache();
}

void beginSensorCalibration(uint32_t now) {
  parkServos(now);
  startSensorCalibration(now);
}

void failConfiguration(const char *message) {
  allLasersOff();
  if (message != configurationError) {
    snprintf(configurationError, sizeof(configurationError), "%s", message);
  }
  gameMode = MODE_ERROR;
  clearFeedback();
  clearDisplayCache();
}

bool validateConfiguration() {
  if (!Config::HARDWARE_CONFIGURATION_CONFIRMED) {
    snprintf(configurationError, sizeof(configurationError),
             "Confirm HW setup");
    return false;
  }

  if (Config::SENSOR_HIT_DELTA <= 0 ||
      Config::SENSOR_HIT_DELTA > 1023 ||
      Config::SENSOR_REARM_DELTA == 0 ||
      Config::SENSOR_REARM_DELTA >= Config::SENSOR_HIT_DELTA) {
    snprintf(configurationError, sizeof(configurationError),
             "Sensor settings");
    return false;
  }

  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    if (Config::SERVO_REAR_DEGREES[player] > 180 ||
        Config::SERVO_FACING_DEGREES[player] > 180 ||
        Config::SERVO_REAR_DEGREES[player] ==
            Config::SERVO_FACING_DEGREES[player]) {
      snprintf(configurationError, sizeof(configurationError),
               "Servo endpoints");
      return false;
    }
  }
  return true;
}

void sampleCalibration(uint32_t now) {
  if (!elapsed(now, lastCalibrationSample,
               Config::SENSOR_CALIBRATION_SAMPLE_MS)) {
    return;
  }
  lastCalibrationSample = now;

  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    const uint16_t reading = analogRead(Config::SENSOR_PINS[player]);
    SensorState &sensor = sensors[player];
    sensor.calibrationSum += reading;
    ++sensor.calibrationSamples;
    if (reading < sensor.calibrationMin) {
      sensor.calibrationMin = reading;
    }
    if (reading > sensor.calibrationMax) {
      sensor.calibrationMax = reading;
    }
  }
}

bool finishCalibration() {
  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    SensorState &sensor = sensors[player];
    if (sensor.calibrationSamples < 50) {
      snprintf(configurationError, sizeof(configurationError),
               "No sensor data");
      return false;
    }

    const uint16_t average =
        static_cast<uint16_t>(sensor.calibrationSum /
                              sensor.calibrationSamples);
    if (average <= 2 || average >= 1021 ||
        sensor.calibrationMax - sensor.calibrationMin >
            Config::SENSOR_MAX_CALIBRATION_SPREAD) {
      snprintf(configurationError, sizeof(configurationError),
               "Check sensor light");
      return false;
    }

    sensor.baseline = static_cast<int16_t>(average);
    sensor.armed = true;
    sensor.candidate = false;
  }
  return true;
}

bool sensorIsHit(uint8_t player, uint16_t reading,
                 uint16_t requiredDelta) {
  const int16_t difference =
      Config::SENSOR_BRIGHTER_IS_HIGHER
          ? static_cast<int16_t>(reading) - sensors[player].baseline
          : sensors[player].baseline - static_cast<int16_t>(reading);
  return difference >= static_cast<int16_t>(requiredDelta);
}

void serviceSensors(uint32_t now) {
  sensorEvents[0] = false;
  sensorEvents[1] = false;

  if (gameMode == MODE_CALIBRATING) {
    sampleCalibration(now);
    if (elapsed(now, calibrationStarted,
                Config::SENSOR_CALIBRATION_MS)) {
      if (!finishCalibration()) {
        failConfiguration(configurationError);
        return;
      }

      scores[0] = 0;
      scores[1] = 0;
      clearPlayerAnswer(0);
      clearPlayerAnswer(1);
      pendingHit = false;
      chooseQuestion();
      gameMode = MODE_ACTIVE;
      lastSensorSample = now - Config::SENSOR_SAMPLE_MS;
      clearDisplayCache();
      tone(Config::BUZZER_PIN, 1400, 100);
    }
    return;
  }

  if (gameMode == MODE_ERROR ||
      !elapsed(now, lastSensorSample, Config::SENSOR_SAMPLE_MS)) {
    return;
  }
  lastSensorSample = now;

  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    SensorState &sensor = sensors[player];
    const uint16_t reading = analogRead(Config::SENSOR_PINS[player]);
    const bool aboveRearm =
        sensorIsHit(player, reading, Config::SENSOR_REARM_DELTA);

    if (!sensor.armed) {
      if (!aboveRearm) {
        sensor.armed = true;
        sensor.candidate = false;
      }
      continue;
    }

    const bool hit = sensorIsHit(
        player, reading, static_cast<uint16_t>(Config::SENSOR_HIT_DELTA));
    if (!hit) {
      sensor.candidate = false;
      continue;
    }

    if (!sensor.candidate) {
      sensor.candidate = true;
      sensor.candidateStarted = now;
    } else if (elapsed(now, sensor.candidateStarted,
                       Config::SENSOR_CONFIRM_MS)) {
      sensorEvents[player] = true;
      sensor.armed = false;
      sensor.candidate = false;
    }
  }
}

// ---------------------------------------------------------------------------
// Chunk 4 of 4: Round/match state machine, setup, and non-blocking main loop
// ---------------------------------------------------------------------------

void resetRoundInputs() {
  clearPlayerAnswer(0);
  clearPlayerAnswer(1);
  pendingHit = false;
}

void startNextRound(uint32_t now) {
  allLasersOff();
  resetRoundInputs();
  chooseQuestion();
  gameMode = MODE_ACTIVE;
  clearFeedback();
  lastSensorSample = now - Config::SENSOR_SAMPLE_MS;
  clearDisplayCache();
}

void finishScoredHit(uint8_t shooter, uint32_t now) {
  if (gameMode != MODE_ACTIVE) {
    return;
  }

  pendingHit = false;
  allLasersOff();
  roundWinner = shooter;
  if (scores[shooter] < Config::SCORE_TO_WIN) {
    ++scores[shooter];
  }

  parkServos(now);
  tone(Config::BUZZER_PIN, 1700, 120);

  if (scores[shooter] >= Config::SCORE_TO_WIN) {
    gameMode = MODE_MATCH_WON;
    char message[17];
    snprintf(message, sizeof(message), "P%u WINS!",
             static_cast<unsigned int>(shooter + 1));
    setLongFeedback(message, 60000);
  } else {
    gameMode = MODE_ROUND_RESULT;
    roundResultStarted = now;
    char message[17];
    snprintf(message, sizeof(message), "P%u SCORES!",
             static_cast<unsigned int>(shooter + 1));
    setLongFeedback(message, Config::ROUND_RESULT_MS);
  }
  clearDisplayCache();
}

void registerSensorHit(uint8_t sensorPlayer, uint32_t now) {
  const uint8_t shooter = 1 - sensorPlayer;

  if (!pendingHit) {
    pendingHit = true;
    pendingShooter = shooter;
    pendingHitAt = now;
    return;
  }

  if (shooter == pendingShooter) {
    return;
  }

  if (elapsed(now, pendingHitAt,
              Config::SIMULTANEOUS_HIT_WINDOW_MS + 1)) {
    finishScoredHit(pendingShooter, now);
    return;
  }

  pendingHit = false;
  setFeedback("SIMULTANEOUS");
  tone(Config::BUZZER_PIN, 600, 120);
}

void serviceHitResolution(uint32_t now) {
  if (gameMode != MODE_ACTIVE) {
    pendingHit = false;
    return;
  }

  for (uint8_t sensorPlayer = 0;
       sensorPlayer < Config::PLAYER_COUNT; ++sensorPlayer) {
    if (sensorEvents[sensorPlayer]) {
      registerSensorHit(sensorPlayer, now);
      if (gameMode != MODE_ACTIVE) {
        return;
      }
    }
  }

  if (pendingHit &&
      elapsed(now, pendingHitAt,
              Config::SIMULTANEOUS_HIT_WINDOW_MS + 1)) {
    finishScoredHit(pendingShooter, now);
  }
}

void servicePlayerInputs(uint32_t now) {
  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    const char key = keypads[player]->service(now);
    if (key != '\0') {
      processKey(player, key, now);
    }

    if (serviceButton(buttons[player], now)) {
      if (servos[player].phase == SERVO_READY) {
        startServoTurn(player, now);
      } else if (gameMode == MODE_ACTIVE) {
        setFeedback("P%u WAIT",
                    static_cast<unsigned int>(player + 1));
      }
    }
  }
}

void setup() {
  // Establish safe laser levels before configuring the outputs.
  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    const uint8_t inactiveLevel =
        Config::LASER_DRIVER_ACTIVE_HIGH ? LOW : HIGH;
    digitalWrite(Config::LASER_DRIVER_PINS[player], inactiveLevel);
    pinMode(Config::LASER_DRIVER_PINS[player], OUTPUT);
    laserOn[player] = false;
  }

  pinMode(Config::BUZZER_PIN, OUTPUT);
  digitalWrite(Config::BUZZER_PIN, LOW);

#if LCD_USE_I2C
  lcd.init();
  lcd.backlight();
#else
  lcd.begin(16, 2);
#endif

  gameMode = MODE_BOOT;
  clearDisplayCache();
  drawDisplay(millis());

  const uint32_t now = millis();
  player1Keypad.begin();
  player2Keypad.begin();
  initializeButton(buttons[0], Config::BUTTON_PINS[0], now);
  initializeButton(buttons[1], Config::BUTTON_PINS[1], now);

  // A changing, noncritical input is used only to vary question selection.
  randomSeed(static_cast<unsigned long>(analogRead(A15)) ^ micros());

  if (!validateConfiguration()) {
    failConfiguration(configurationError);
    return;
  }

  for (uint8_t player = 0; player < Config::PLAYER_COUNT; ++player) {
    servos[player].motor.attach(Config::SERVO_PINS[player]);
    servos[player].motor.write(Config::SERVO_REAR_DEGREES[player]);
    servos[player].phase = SERVO_READY;
    servos[player].phaseStarted = now;
  }

  startSensorCalibration(now);
}

void loop() {
  const uint32_t now = millis();

  if (gameMode == MODE_ERROR) {
    allLasersOff();
    drawDisplay(now);
    return;
  }

  servicePlayerInputs(now);
  serviceServos(now);
  serviceLasers(now);
  serviceSensors(now);

  if (gameMode == MODE_ACTIVE) {
    serviceHitResolution(now);
    advanceQuestionPage(now);
  } else if (gameMode == MODE_ROUND_RESULT &&
             elapsed(now, roundResultStarted,
                     Config::ROUND_RESULT_MS)) {
    startNextRound(now);
  }

  if (feedbackActive &&
      elapsed(now, feedbackStarted, feedbackDuration) &&
      gameMode != MODE_MATCH_WON) {
    feedbackActive = false;
  }

  drawDisplay(now);
}
