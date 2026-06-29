#include "FaceGlobals.h"

FaceState currentFace = stateNormal;
FaceState targetFace = stateIdle;

volatile int targetEmotionCode = 1; 
int lastEmotionCode = 1; 
volatile unsigned long lastInteractionTime = 0; 

unsigned long winkStartTime = 0;
unsigned long sleepStartTime = 0;
bool winkDirection = false; 
int sleepBlinkCount = 0;

unsigned long stateBlinkStartTime = 0;
bool isStateBlinking = false;

float blinkFactor = 1.0;
float targetBlinkFactor = 1.0;
unsigned long lastBlinkTime = 0;
unsigned long nextBlinkDelay = 3000;

float susWeight = 0.0f;
float smugWeight = 0.0f;
float lerpSpeed = 0.3f; 
