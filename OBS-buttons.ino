#include "Keyboard.h"
#include <elapsedMillis.h>

const int TRANSITION_PIN  =  2;
const int CUT_PIN         =  4;
const int FADE_PIN        =  8;
const int SCENE_1_PIN     = A0;
const int SCENE_2_PIN     = 15;
const int SCENE_3_PIN     = 14;
const int SCENE_4_PIN     = 16;

const int CUT_LED         =  3;
const int FADE_LED        =  5;
const int S1_LED          =  6;
const int S2_LED          =  7;
const int S3_LED          =  9;
const int S4_LED          = 10;

bool cut, fade;
byte scene;

#include "elapsedMillis.h"
 
/*
 * OneShot - wait for a pin to go from HIGH to LOW and back to HIGH as one event, with debouncing
 */

class Switch {
  public:
    enum State   { OFF, PRESS, ON, RELEASE };
    enum OneShot { WAITING, TRIGGERED };
    
    Switch(int pin): _pin(pin), _duration(0), _debounce(WAITING) {
        pinMode(_pin, INPUT_PULLUP);
        check();
    }
    
    State check(void) {
        handle();
        return _state;
    }

    bool triggered(void) {
        handle();
        if (_debounce == TRIGGERED) {
            _debounce = WAITING;
            return true;
        }
        return false;
    }
    
  private:
    void handle(void) {
        byte buttonPos1 = digitalRead(_pin);   delay(1);
        byte buttonPos2 = digitalRead(_pin);
        // debounce: WAITING ------------------> TRIGGERED -> WAITING
        // state:    OFF     -> PRESSED -> ON -> RELEASED  -> OFF
        if (buttonPos1 == buttonPos2) {  // debounce implies both readings are the same
            // oneshot (press followed by release is a single trigger)...
            if (LOW  == buttonPos1) { 
                if (_state == OFF)   {
                    _state = PRESS;
                } else {
                    _state = ON;
                }
            }
            if (HIGH == buttonPos1) {
                if (_state == ON) {
                  _state    = RELEASE;
                } else {
                    _state = OFF;
                }
            }
        }
        if (_state == RELEASE) {
            _debounce = TRIGGERED;
        }
    }

    int _pin;
    State _state;
    OneShot _debounce;
    unsigned long _duration;
    elapsedMillis _downTime;
};

Switch *Transition;
Switch *Cut;
Switch *Fade;
Switch *Sc1;
Switch *Sc2;
Switch *Sc3;
Switch *Sc4;

enum  TransitionType   { T_UNKNOWN, CUT, FADE, T_NONE};
enum  TransitionSource { S_UNKNOWN, S1, S2, S3, S4, S_NONE };

// Current Values
TransitionType     ttype   = FADE;
TransitionSource   tsource = S_NONE;

// Previous values
TransitionType   t = T_UNKNOWN;
TransitionSource s = S_UNKNOWN;

#define DARK LOW
#define LIT  HIGH

void display(TransitionType ttype,TransitionSource tsource) {
    // turn all off...
    digitalWrite(CUT_LED,  DARK); 
    digitalWrite(FADE_LED, DARK); 
    digitalWrite(S1_LED,   DARK); 
    digitalWrite(S2_LED,   DARK); 
    digitalWrite(S3_LED,   DARK); 
    digitalWrite(S4_LED,   DARK); 

    // then turn on the right ones
    if (ttype == CUT)  digitalWrite(CUT_LED,  LIT); else 
    if (ttype == FADE) digitalWrite(FADE_LED, LIT);
                       
    if (tsource == S1) digitalWrite(S1_LED,   LIT); else
    if (tsource == S2) digitalWrite(S2_LED,   LIT); else
    if (tsource == S3) digitalWrite(S3_LED,   LIT); else
    if (tsource == S4) digitalWrite(S4_LED,   LIT);
}

void setup() {  
  Keyboard.begin();
#if SERIAL_DEBUG
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println("Keyboard Buttons Test");
#endif

  Transition = new Switch(TRANSITION_PIN);
  Cut        = new Switch(CUT_PIN);
  Fade       = new Switch(FADE_PIN);
  Sc1        = new Switch(SCENE_1_PIN);
  Sc2        = new Switch(SCENE_2_PIN);
  Sc3        = new Switch(SCENE_3_PIN);
  Sc4        = new Switch(SCENE_4_PIN);

  pinMode(TRANSITION_PIN,   INPUT_PULLUP);
  
  pinMode(CUT_LED,   OUTPUT);
  pinMode(FADE_LED,  OUTPUT);
  pinMode(S1_LED,    OUTPUT);
  pinMode(S2_LED,    OUTPUT);
  pinMode(S3_LED,    OUTPUT);
  pinMode(S4_LED,    OUTPUT); 


  display(T_UNKNOWN, S_UNKNOWN);
}


void select_S1(void) { Keyboard.press(KEY_LEFT_ALT); Keyboard.press('1'); delay(100); Keyboard.releaseAll();}
void select_S2(void) { Keyboard.press(KEY_LEFT_ALT); Keyboard.press('2'); delay(100); Keyboard.releaseAll();}
void select_S3(void) { Keyboard.press(KEY_LEFT_ALT); Keyboard.press('3'); delay(100); Keyboard.releaseAll();}
void select_S4(void) { Keyboard.press(KEY_LEFT_ALT); Keyboard.press('4'); delay(100); Keyboard.releaseAll();}

void do_transition(void) {
  switch (tsource) {
      case S1:  select_S1(); break;
      case S2:  select_S2(); break;
      case S3:  select_S3(); break;
      case S4:  select_S4(); break;
      default: break;
  }
  if (ttype == CUT) {
      Keyboard.press(KEY_LEFT_ALT); Keyboard.press('C'); delay(100); Keyboard.releaseAll();
  } else if (ttype == FADE) {
      Keyboard.press(KEY_LEFT_ALT); Keyboard.press('F'); delay(100); Keyboard.releaseAll();
  }
  tsource = S_NONE;  // after we transition to a source, revert to a general transition
}


void loop() {
    if (Transition->triggered())   { do_transition(); }
    
    if (Cut       ->triggered())   { ttype = CUT;  }
    if (Fade      ->triggered())   { ttype = FADE; }

    // selecting a source twice deselects it...
    if (Sc1       ->triggered())   { if (tsource == S1) tsource = S_NONE; else tsource = S1; }
    if (Sc2       ->triggered())   { if (tsource == S2) tsource = S_NONE; else tsource = S2; }
    if (Sc3       ->triggered())   { if (tsource == S3) tsource = S_NONE; else tsource = S3; }
    if (Sc4       ->triggered())   { if (tsource == S4) tsource = S_NONE; else tsource = S4; }
    
#if SERIAL_DEBUG
    if (t != ttype) {
        t = ttype;
        Serial.print("type: "); Serial.println( (ttype == CUT) ? "CUT" : (ttype == FADE) ? "FADE" : "NONE"); 
    }
    if (s != tsource) {
        s = tsource;
        Serial.print("source: ");
        if (tsource == S1) Serial.println("S1"); else
        if (tsource == S2) Serial.println("S2"); else
        if (tsource == S3) Serial.println("S3"); else
        if (tsource == S4) Serial.println("S4"); else
        Serial.println("DEFAULT");
    }
#endif

    display(ttype, tsource);
}
