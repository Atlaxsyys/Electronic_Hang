#include <Adafruit_NeoPixel.h>

#define LED_PIN     52
#define NUM_LEDS    120

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

int sensorLEDs[][20] = {
  {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, -1},            
  {17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, -1},       
  {29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, -1},       
  {41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, -1},       
  {53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, -1},       
  {65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, -1},       
  {77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, -1},       
  {89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, -1},      
  {101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, -1} 
};

int sensorColors[][3] = {
  {0, 80, 255},
  {0, 200, 255},
  {180, 0, 255},
  {0, 255, 150},
  {150, 255, 0},
  {255, 220, 0},
  {255, 120, 0},
  {255, 0, 80},
  {255, 50, 200}
};

struct NotePhysics {
  float energy;    
  float decay;     
} notes[9];

byte midiNotes[] = {41, 43, 45, 36, 38, 42, 46, 49, 51};

int trigInner[3] = {2, 4, 6};
int echoInner[3] = {3, 5, 7};
int trigMiddle[6] = {8, 10, 12, 14, 16, 18};
int echoMiddle[6] = {9, 11, 13, 15, 17, 19};

const int ACTIVATION_THRESHOLD = 20; 
const unsigned long DEBOUNCE_TIME = 90; 
unsigned long lastTriggerTime[9] = {0};

void setup() {
  Serial.begin(115200);

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

  strip.begin();
  strip.setBrightness(200);
  strip.clear();
  
  for(int i=0; i<5; i++) strip.setPixelColor(i, 0);
  
  for(int i = 0; i < 9; i++) {
    notes[i].energy = 0.0;
    notes[i].decay = 0.95;
  }
}

void loop() {
  for(int i = 0; i < 3; i++) checkSensor(trigInner[i], echoInner[i], i);
  for(int i = 0; i < 6; i++) checkSensor(trigMiddle[i], echoMiddle[i], i + 3);

  updateVisuals();
  
  delay(2);
}

void checkSensor(int trigPin, int echoPin, int sensorIndex) {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH, 5000); 
  if(duration == 0) return; 
  
  long distance = duration * 0.034 / 2;

  if(distance > 1 && distance < ACTIVATION_THRESHOLD) {
    unsigned long currentTime = millis();
    if(currentTime - lastTriggerTime[sensorIndex] > DEBOUNCE_TIME) {
      
      int velocity = mapDistanceToVelocity(distance);
      
      sendMidiNote(sensorIndex, velocity);
      triggerLight(sensorIndex, velocity);
      
      lastTriggerTime[sensorIndex] = currentTime;
    }
  }
}

void triggerLight(int index, int velocity) {
  notes[index].energy = 0.6; 
  notes[index].decay = 0.90 + ((float)velocity / 127.0) * 0.07;
}

void updateVisuals() {
  strip.clear();
  
  for(int k=0; k<5; k++) strip.setPixelColor(k, 0);

  float breath = (sin(millis() / 1500.0) + 1.0) * 0.03 + 0.02; 
  
  for(int i = 0; i < 9; i++) {
    notes[i].energy *= notes[i].decay;
    if(notes[i].energy < 0.001) notes[i].energy = 0;
    
    float totalBrightness = notes[i].energy + breath;
    if(totalBrightness > 1.0) totalBrightness = 1.0;

    int r = sensorColors[i][0];
    int g = sensorColors[i][1];
    int b = sensorColors[i][2];
    
    int flashAmount = (int)(notes[i].energy * notes[i].energy * 40);
    
    r = constrain(r + flashAmount, 0, 255);
    g = constrain(g + flashAmount, 0, 255);
    b = constrain(b + flashAmount, 0, 255);
    
    uint32_t finalColor = strip.Color(
      (int)(r * totalBrightness),
      (int)(g * totalBrightness),
      (int)(b * totalBrightness)
    );

    for(int j = 0; j < 20; j++) { 
      int ledIdx = sensorLEDs[i][j];
      if(ledIdx == -1) break;
      strip.setPixelColor(ledIdx, finalColor);
    }
  }
  
  strip.show();
}

byte mapDistanceToVelocity(int distance) {
  int vel = map(distance, 2, ACTIVATION_THRESHOLD, 127, 50);
  return constrain(vel, 1, 127);
}

void sendMidiNote(int sensorIndex, int velocity) {
  Serial.print("NOTE:");
  Serial.print(midiNotes[sensorIndex]);
  Serial.print(":");
  Serial.println(velocity);
}
