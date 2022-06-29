#ifndef _UV_BUDGIE_H_
#define _UV_BUDGIE_H_

#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_Soundboard.h>

// Wing positions - adjust as required
// If the servo is chattering at the end positions,
// adjust the min or max value by 5ish
#define WINGS_START 470
#define WINGS_DOWN 480  // Max position
#define WINGS_UP_A_BIT 420
#define WINGS_UP_A_LOT 400
#define PASS_OUT_POS 380
#define DEAD_POS 350  // Min position

// Audio track numbers
#define CHEEP_CHEEP 0
#define NO_PROTECTION 1
#define SOME_PROTECTION 2
#define PROTECTION_ESSENTIAL 3
#define EXTRA_PROTECTION 4

#define DATESTR_LEN 12  // "2022-06-15Z"

enum flap_speeds {VFAST = 1, FAST, SLOW, VSLOW};

struct uvData {
    //char date[DATESTR_LEN];
    char datestr[DATESTR_LEN] = {'\0'};
    char uv[8] = {0};
    char temp[8] = {0};
};

class UVBudgie {
  public:
    uvData * _fcst; // 5 days worth of data
    int state = 0;
    UVBudgie(Adafruit_Soundboard *sfx, Adafruit_PWMServoDriver *pwm, uvData *fcst);
 private:
    Adafruit_PWMServoDriver *_pwm;
    Adafruit_Soundboard *_sfx;
    int _servo = 0;  // default servo 0
    uint16_t _pulselen = WINGS_START;
 public:
    void init(int servo, uint16_t pos);
    const char* dow(int i);
    int doAction(boolean audio);
    int updateState();
    int getUVMax(int day_idx);
    int getTempMax(int day_idx);
    void StartPos(uint16_t start_pos);
    void Flap(uint16_t down_pos, uint16_t up_pos, int speed_idx, int flaps);
    void PassOut(uint16_t end_pos, int speed_idx);
    void Dead(uint16_t end_pos, int speed_idx);
    void Tweet(uint8_t track, boolean audio);
};

#endif
