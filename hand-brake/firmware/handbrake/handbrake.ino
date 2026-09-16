// Frein a main analogique (load cell + HX711)
// Carte : Arduino Pro Micro / Leonardo / Micro (ATmega32U4).
// Une Uno ne peut PAS se faire passer pour une manette USB avec ce sketch.
//
// Cablage HX711 -> Arduino :
//   VCC -> 5V
//   GND -> GND
//   DT  -> pin 2
//   SCK -> pin 3
//
// Load cell -> HX711 :
//   Rouge -> E+
//   Noir  -> E-
//   Vert  -> A+
//   Blanc -> A-
//
// Apres flash : Windows = joy.cpl (axe Throttle).
// Dans le jeu, bind Handbrake sur l'axe Throttle de cette manette.

#include <Joystick.h>

const int HX711_DOUT = 2;
const int HX711_SCK = 3;

// Inverse si le throttle monte au repos et baisse quand tu tires
const bool INVERT = false;

// Valeur FORCE max vue dans test_loadcell.ino (levier tire a fond).
// Trop grand = tu n'atteins jamais 100%. Trop petit = saturation trop tot.
long RAW_MAX = 30000;

// Ignore le bruit au repos. Monte un peu si l'axe tremble a 0.
const long DEADZONE = 400;

long offset = 0;

bool hx711Ready(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (digitalRead(HX711_DOUT) == HIGH) {
    if (millis() - start > timeoutMs) {
      return false;
    }
  }
  return true;
}

long readHX711() {
  if (!hx711Ready(250)) {
    return 0x7FFFFFFF;
  }

  long value = 0;
  noInterrupts();
  for (int i = 0; i < 24; i++) {
    digitalWrite(HX711_SCK, HIGH);
    delayMicroseconds(1);
    value = (value << 1) | digitalRead(HX711_DOUT);
    digitalWrite(HX711_SCK, LOW);
    delayMicroseconds(1);
  }
  digitalWrite(HX711_SCK, HIGH);
  delayMicroseconds(1);
  digitalWrite(HX711_SCK, LOW);
  interrupts();

  if (value & 0x800000) {
    value |= ~0xFFFFFFL;
  }
  return value;
}

long readAverage(int samples) {
  long sum = 0;
  int ok = 0;
  for (int i = 0; i < samples; i++) {
    long v = readHX711();
    if (v == 0x7FFFFFFF) {
      continue;
    }
    sum += v;
    ok++;
  }
  if (ok == 0) {
    return 0x7FFFFFFF;
  }
  return sum / ok;
}

void setup() {
  pinMode(HX711_DOUT, INPUT);
  pinMode(HX711_SCK, OUTPUT);
  digitalWrite(HX711_SCK, LOW);
  pinMode(LED_BUILTIN, OUTPUT);

  Joystick.begin();
  Joystick.setThrottle(0);

  delay(400);
  offset = readAverage(12);
  if (offset == 0x7FFFFFFF) {
    offset = 0;
  }
}

void loop() {
  long raw = readHX711();
  if (raw == 0x7FFFFFFF) {
    return;
  }

  long force = raw - offset;
  if (INVERT) {
    force = -force;
  }
  if (force < 0) {
    force = 0;
  }

  int throttle = 0;
  if (force > DEADZONE) {
    if (force > RAW_MAX) {
      force = RAW_MAX;
    }
    throttle = map(force, DEADZONE, RAW_MAX, 0, 255);
    if (throttle > 255) {
      throttle = 255;
    }
  }

  Joystick.setThrottle(throttle);
  digitalWrite(LED_BUILTIN, throttle > 0);
}
