#include <Adafruit_NeoPixel.h>
#include <math.h>

#define LED_PIN 52
#define NUM_LEDS 120
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ========== КОНФИГУРАЦИЯ ==========
// Распределение LED по датчикам (вертикальные полосы на цилиндре)
int sensorLEDs[][30] = {
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},     // Датчик 0 (внутри): LED 0-13
  {14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, // Датчик 1 (внутри): LED 14-27
  {28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, // Датчик 2 (внутри): LED 28-41
  {42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, // Датчик 3 (снаружи): LED 42-55
  {56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, // Датчик 4 (снаружи): LED 56-69
  {70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, // Датчик 5 (снаружи): LED 70-83
  {84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, // Датчик 6 (снаружи): LED 84-97
  {98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, // Датчик 7 (снаружи): LED 98-111
  {112, 113, 114, 115, 116, 117, 118, 119, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}  // Датчик 8 (снаружи): LED 112-119
};

// Количество LED для каждого датчика
int sensorLEDCounts[] = {14, 14, 14, 14, 14, 14, 14, 14, 8};

// Цвета для каждого датчика [R, G, B]
int sensorColors[][3] = {
  {0, 0, 255},      // Датчик 0: Синий
  {0, 100, 255},    // Датчик 1: Голубой
  {100, 0, 255},    // Датчик 2: Фиолетовый
  {0, 255, 0},      // Датчик 3: Зеленый
  {100, 255, 0},    // Датчик 4: Салатовый
  {255, 255, 0},    // Датчик 5: Желтый
  {255, 150, 0},    // Датчик 6: Оранжевый
  {255, 0, 0},      // Датчик 7: Красный
  {255, 0, 100}     // Датчик 8: Розовый
};

byte midiNotes[] = {41, 43, 45, 36, 38, 42, 46, 49, 51};

// Настройки датчиков
int trigInner[3] = {2, 4, 6};
int echoInner[3] = {3, 5, 7};
int trigMiddle[6] = {8, 10, 12, 14, 16, 18};
int echoMiddle[6] = {9, 11, 13, 15, 17, 19};

const int ACTIVATION_THRESHOLD = 15;
const unsigned long DEBOUNCE_TIME = 100;
unsigned long lastTriggerTime[9] = {0};

// Переменные для анимации волны
struct WaveAnimation {
  int sensorIndex;
  unsigned long startTime;
  int waveSpeed;
  int maxWaveDistance;
  bool isActive;
} waveAnims[9];

// Режим анимации
int animationMode = 0; // 0=Wave, 1=Spiral, 2=Pulse, 3=Rainbow, 4=Stars

// ========== ОСНОВНОЙ КОД ==========
void setup() {
  Serial.begin(115200);
  
  // Инициализация датчиков
  for(int i = 0; i < 3; i++) {
    pinMode(trigInner[i], OUTPUT);
    pinMode(echoInner[i], INPUT);
    digitalWrite(trigInner[i], LOW);
  }
  for(int i = 0; i < 6; i++) {
    pinMode(trigMiddle[i], OUTPUT);
    pinMode(echoMiddle[i], INPUT);
    digitalWrite(trigMiddle[i], LOW);
  }
  
  // Инициализация LED
  strip.begin();
  strip.setBrightness(150);
  strip.clear();
  strip.show();
  
  // Инициализация анимаций
  for(int i = 0; i < 9; i++) {
    waveAnims[i].isActive = false;
  }
  
  Serial.println("✅ Система готова! Волны активированы!");
  Serial.println("📡 Доступные режимы: 0=ВОЛНА, 1=СПИРАЛЬ, 2=ПУЛЬС, 3=РАДУГА, 4=ЗВЁЗДЫ");
}

void loop() {
  // Опрос внутренних датчиков (0-2)
  for(int i = 0; i < 3; i++) {
    checkSensor(trigInner[i], echoInner[i], i);
  }
  
  // Опрос внешних датчиков (3-8)
  for(int i = 0; i < 6; i++) {
    checkSensor(trigMiddle[i], echoMiddle[i], i + 3);
  }
  
  // Обновляем анимации волн
  updateWaveAnimations();
  
  // Проверяем серийный ввод для переключения режимов
  checkSerialInput();
  
  delay(10);
}

// ========== ФУНКЦИИ ДАТЧИКОВ ==========

void checkSensor(int trigPin, int echoPin, int sensorIndex) {
  long distance = measureDistance(trigPin, echoPin);
  
  if(distance > 0 && distance < ACTIVATION_THRESHOLD) {
    unsigned long currentTime = millis();
    
    if(currentTime - lastTriggerTime[sensorIndex] > DEBOUNCE_TIME) {
      activateWave(sensorIndex, distance);
      sendMidiNote(sensorIndex, distance);
      lastTriggerTime[sensorIndex] = currentTime;
      
      Serial.print("📍 Датчик ");
      Serial.print(sensorIndex);
      Serial.print(" | Расстояние: ");
      Serial.print(distance);
      Serial.print("см | Velocity: ");
      Serial.println(mapDistanceToVelocity(distance));
    }
  }
}

long measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 30000);
  if(duration == 0) return -1;
  
  return duration * 0.034 / 2;
}

void activateWave(int sensorIndex, int distance) {
  int waveSpeed = map(distance, 5, ACTIVATION_THRESHOLD, 20, 80);
  
  waveAnims[sensorIndex].sensorIndex = sensorIndex;
  waveAnims[sensorIndex].startTime = millis();
  waveAnims[sensorIndex].waveSpeed = waveSpeed;
  waveAnims[sensorIndex].maxWaveDistance = sensorLEDCounts[sensorIndex];
  waveAnims[sensorIndex].isActive = true;
}

// ========== ВОЛНОВАЯ АНИМАЦИЯ ==========

void updateWaveAnimations() {
  switch(animationMode) {
    case 0: updateWaveAnimations_Wave(); break;
    case 1: updateWaveAnimations_Spiral(); break;
    case 2: updateWaveAnimations_Pulse(); break;
    case 3: updateWaveAnimations_Rainbow(); break;
    case 4: updateWaveAnimations_Stars(); break;
    default: updateWaveAnimations_Wave();
  }
}

void updateWaveAnimations_Wave() {
  strip.clear();
  
  unsigned long currentTime = millis();
  
  for(int i = 0; i < 9; i++) {
    if(waveAnims[i].isActive) {
      unsigned long elapsedTime = currentTime - waveAnims[i].startTime;
      int wavePosition = (elapsedTime / waveAnims[i].waveSpeed);
      
      int sensorIndex = waveAnims[i].sensorIndex;
      int maxDist = waveAnims[i].maxWaveDistance;
      
      for(int j = 0; j < maxDist; j++) {
        int ledIndex = sensorLEDs[sensorIndex][j];
        int distanceFromWave = abs(j - wavePosition);
        
        if(distanceFromWave <= 3) {
          int brightness = 255 - (distanceFromWave * 85);
          brightness = constrain(brightness, 0, 255);
          
          uint32_t color = makeColor(
            sensorColors[sensorIndex][0],
            sensorColors[sensorIndex][1],
            sensorColors[sensorIndex][2],
            brightness
          );
          
          strip.setPixelColor(ledIndex, color);
        }
      }
      
      if(wavePosition >= maxDist) {
        waveAnims[i].isActive = false;
      }
    }
  }
  
  strip.show();
}

void updateWaveAnimations_Spiral() {
  strip.clear();
  
  unsigned long currentTime = millis();
  
  for(int i = 0; i < 9; i++) {
    if(waveAnims[i].isActive) {
      unsigned long elapsedTime = currentTime - waveAnims[i].startTime;
      int spiralProgress = (elapsedTime / waveAnims[i].waveSpeed);
      
      int sensorIndex = waveAnims[i].sensorIndex;
      int maxDist = waveAnims[i].maxWaveDistance;
      
      for(int j = 0; j < maxDist; j++) {
        int ledIndex = sensorLEDs[sensorIndex][j];
        
        int phase = (j + spiralProgress) % maxDist;
        
        if(phase < 5) {
          int brightness = map(phase, 0, 5, 255, 30);
          brightness = constrain(brightness, 0, 255);
          
          uint32_t color = makeColor(
            sensorColors[sensorIndex][0],
            sensorColors[sensorIndex][1],
            sensorColors[sensorIndex][2],
            brightness
          );
          
          strip.setPixelColor(ledIndex, color);
        }
      }
      
      if(spiralProgress >= maxDist * 2) {
        waveAnims[i].isActive = false;
      }
    }
  }
  
  strip.show();
}

void updateWaveAnimations_Pulse() {
  strip.clear();
  
  unsigned long currentTime = millis();
  
  for(int i = 0; i < 9; i++) {
    if(waveAnims[i].isActive) {
      unsigned long elapsedTime = currentTime - waveAnims[i].startTime;
      
      int sensorIndex = waveAnims[i].sensorIndex;
      int maxDist = waveAnims[i].maxWaveDistance;
      
      float pulsePhase = (float)(elapsedTime % 1000) / 1000.0;
      int brightness = (int)(255 * fabs(sin(pulsePhase * 3.14159)));
      
      for(int j = 0; j < maxDist; j++) {
        int ledIndex = sensorLEDs[sensorIndex][j];
        
        uint32_t color = makeColor(
          sensorColors[sensorIndex][0],
          sensorColors[sensorIndex][1],
          sensorColors[sensorIndex][2],
          brightness
        );
        
        strip.setPixelColor(ledIndex, color);
      }
      
      if(elapsedTime > 2000) {
        waveAnims[i].isActive = false;
      }
    }
  }
  
  strip.show();
}

void updateWaveAnimations_Rainbow() {
  strip.clear();
  
  unsigned long currentTime = millis();
  
  for(int i = 0; i < 9; i++) {
    if(waveAnims[i].isActive) {
      unsigned long elapsedTime = currentTime - waveAnims[i].startTime;
      int wavePosition = (elapsedTime / waveAnims[i].waveSpeed);
      
      int sensorIndex = waveAnims[i].sensorIndex;
      int maxDist = waveAnims[i].maxWaveDistance;
      
      for(int j = 0; j < maxDist; j++) {
        int ledIndex = sensorLEDs[sensorIndex][j];
        
        int distanceFromWave = abs(j - wavePosition);
        
        if(distanceFromWave <= 5) {
          int brightness = 255 - (distanceFromWave * 51);
          brightness = constrain(brightness, 0, 255);
          
          uint32_t rainbowColor = getRainbowColor(j, maxDist, brightness);
          strip.setPixelColor(ledIndex, rainbowColor);
        }
      }
      
      if(wavePosition >= maxDist) {
        waveAnims[i].isActive = false;
      }
    }
  }
  
  strip.show();
}

void updateWaveAnimations_Stars() {
  strip.clear();
  
  unsigned long currentTime = millis();
  
  for(int i = 0; i < 9; i++) {
    if(waveAnims[i].isActive) {
      unsigned long elapsedTime = currentTime - waveAnims[i].startTime;
      int wavePosition = (elapsedTime / (waveAnims[i].waveSpeed * 2));
      
      int sensorIndex = waveAnims[i].sensorIndex;
      int maxDist = waveAnims[i].maxWaveDistance;
      
      for(int j = 0; j < maxDist; j++) {
        int ledIndex = sensorLEDs[sensorIndex][j];
        
        int flickerPhase = (j + wavePosition) % 20;
        int brightness = 0;
        
        if(flickerPhase < 8) {
          brightness = map(flickerPhase, 0, 8, 255, 100);
        } else if(flickerPhase < 12) {
          brightness = map(flickerPhase, 8, 12, 100, 255);
        } else if(flickerPhase < 15) {
          brightness = map(flickerPhase, 12, 15, 255, 50);
        }
        
        uint32_t color = makeColor(
          sensorColors[sensorIndex][0],
          sensorColors[sensorIndex][1],
          sensorColors[sensorIndex][2],
          brightness
        );
        
        strip.setPixelColor(ledIndex, color);
      }
      
      if(wavePosition >= maxDist * 2) {
        waveAnims[i].isActive = false;
      }
    }
  }
  
  strip.show();
}

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

uint32_t makeColor(int r, int g, int b, int alpha) {
  r = (r * alpha) / 255;
  g = (g * alpha) / 255;
  b = (b * alpha) / 255;
  return strip.Color(r, g, b);
}

uint32_t getRainbowColor(int position, int total, int brightness) {
  float hue = (float)position / total * 6.0;
  int hueIndex = (int)hue % 6;
  float phase = hue - hueIndex;
  
  int r, g, b;
  
  switch(hueIndex) {
    case 0: r = 255; g = phase * 255; b = 0; break;
    case 1: r = 255 - phase * 255; g = 255; b = 0; break;
    case 2: r = 0; g = 255; b = phase * 255; break;
    case 3: r = 0; g = 255 - phase * 255; b = 255; break;
    case 4: r = phase * 255; g = 0; b = 255; break;
    case 5: r = 255; g = 0; b = 255 - phase * 255; break;
    default: r = 0; g = 0; b = 0;
  }
  
  r = (r * brightness) / 255;
  g = (g * brightness) / 255;
  b = (b * brightness) / 255;
  
  return strip.Color(r, g, b);
}

void setAnimationMode(int mode) {
  animationMode = constrain(mode, 0, 4);
  Serial.print("🎨 Режим анимации: ");
  switch(animationMode) {
    case 0: Serial.println("ВОЛНА 🌊"); break;
    case 1: Serial.println("СПИРАЛЬ 🌪️"); break;
    case 2: Serial.println("ПУЛЬС 💓"); break;
    case 3: Serial.println("РАДУГА 🌈"); break;
    case 4: Serial.println("ЗВЁЗДЫ ⭐"); break;
  }
}

void checkSerialInput() {
  if(Serial.available() > 0) {
    char input = Serial.read();
    
    if(input >= '0' && input <= '4') {
      setAnimationMode(input - '0');
    } else if(input == '?') {
      Serial.println("\n🎮 КОМАНДЫ УПРАВЛЕНИЯ:");
      Serial.println("0 = ВОЛНА (волна распространяется по полосе)");
      Serial.println("1 = СПИРАЛЬ (торнадо вокруг цилиндра)");
      Serial.println("2 = ПУЛЬС (пульсирующее сердцебиение)");
      Serial.println("3 = РАДУГА (разноцветная волна)");
      Serial.println("4 = ЗВЁЗДЫ (мерцающие звёзды)");
      Serial.println("? = показать это сообщение\n");
    }
  }
}

// ========== MIDI ==========

void sendMidiNote(int sensorIndex, int distance) {
  byte note = midiNotes[sensorIndex];
  byte velocity = mapDistanceToVelocity(distance);
  
  Serial.print("NOTE:");
  Serial.print(note);
  Serial.print(":");
  Serial.println(velocity);
}

byte mapDistanceToVelocity(int distance) {
  byte velocity = map(distance, 5, ACTIVATION_THRESHOLD, 127, 64);
  velocity = constrain(velocity, 64, 127);
  return velocity;
}