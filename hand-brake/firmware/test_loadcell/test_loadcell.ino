// Test du load cell + HX711
// Marche sur TOUTES les cartes Arduino (Uno, Nano, Pro Micro, Leonardo...).
//
// 1. Ouvre ce fichier dans Arduino IDE
// 2. Outils > Carte : choisis ta carte
// 3. Outils > Port : choisis le port USB de l'Arduino
// 4. Croquis > Téléverser
// 5. Outils > Moniteur série, 115200 bauds
// 6. Laisse le frein à main au repos 2 secondes (tare auto)
// 7. Tire à fond, note la valeur "force" MAX. C'est RAW_MAX pour le firmware.

const int HX711_DOUT = 2;  // DT du HX711
const int HX711_SCK = 3;   // SCK du HX711

// Inverse si la valeur descend quand tu tires
const bool INVERT = false;

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
  if (!hx711Ready(1000)) {
    return 0x7FFFFFFF;  // timeout = câblage HS
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
  // 25e pulse = canal A, gain 128
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
  for (int i = 0; i < samples; i++) {
    long v = readHX711();
    if (v == 0x7FFFFFFF) {
      return v;
    }
    sum += v;
  }
  return sum / samples;
}

void setup() {
  pinMode(HX711_DOUT, INPUT);
  pinMode(HX711_SCK, OUTPUT);
  digitalWrite(HX711_SCK, LOW);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
  }

  Serial.println(F("Tare... ne touche pas au levier"));
  delay(500);
  offset = readAverage(15);
  if (offset == 0x7FFFFFFF) {
    Serial.println(F("ERREUR: HX711 ne repond pas. Verifie DT/SCK/5V/GND."));
    while (true) {
      digitalWrite(LED_BUILTIN, millis() % 200 < 100);
    }
  }

  Serial.print(F("Offset: "));
  Serial.println(offset);
  Serial.println(F("Tire le levier. Note la valeur FORCE au max."));
}

void loop() {
  long raw = readHX711();
  if (raw == 0x7FFFFFFF) {
    Serial.println(F("Timeout HX711"));
    delay(200);
    return;
  }

  long force = raw - offset;
  if (INVERT) {
    force = -force;
  }

  Serial.print(F("brut="));
  Serial.print(raw);
  Serial.print(F("  force="));
  Serial.println(force);

  digitalWrite(LED_BUILTIN, force > 200);
  delay(80);
}
