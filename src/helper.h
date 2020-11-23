#include <Arduino.h>
#include <ArduinoOTA.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <fxController.h>
#include <settings.h>

#define PIN 2 // pin to connect led strip, ignored for esp8266
#define PIXELCOUNT 10 //default number of pixels in led strip

#define PORT_IN 6454
#define PORT_OUT 6455
#define PORT_OUT_UPD 6457
#define VERSION "v_0.1.1"
#define UNI 10
#define MAIN_FILE "/mainfile"
#define NAME_FILE "/namefile"
#define SSID_FILE "/ssidfile"
#define PASS_FILE "/passfile"
#define PLAYLIST_FILE "/playlist"

extern bool toAnswer;
extern bool isFading;
extern WiFiUDP wifiUDP;

extern FxController FX;

extern uint8_t hData[5];
extern uint8_t hData1[13];
extern uint8_t hData2[17];
extern char command;
extern char option; 
extern int mask;
extern IPAddress sourceIP;
extern uint8_t uni;

//playlist item setting type
typedef struct {
    uint8_t dimmer;
    RgbColor color;
    RgbColor fxColor;
    uint8_t strobe;
    uint8_t fxNumber;
    double fxSpeed;
    uint8_t fxSize;
    uint8_t fxParts;
    uint8_t fxFade;
    uint8_t fxParams;
    uint8_t fxSpread;
    uint8_t fxWidth;
    boolean fxReverse;
    boolean fxAttack;
    boolean fxSymm;
    boolean fxRnd;
    boolean fxRndColor;
} ledsettings_t;

//NeoPixelBus
extern NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip;
//extern RgbColor *fxData; //array, containing data of current fx
//extern double *rgbData; //for rgb effect
//extern RgbTemp_t *fxTemp; //sinus effect
//extern RgbTemp_t *attackTemp; //sinus effect
extern ledsettings_t *playlist; //array, containing data for settings to be played
extern ledsettings_t *playlist_temp; ////
extern NeoPixelAnimator animations;
extern NeoPixelAnimator animations2;
extern uint8_t playlistPeriod;
extern unsigned long playlistPeriodMs; 
extern unsigned long playlistLastTime;
extern uint8_t playlist_counter;



extern settings_t settings; //main settings
extern settings_t request; //variable for storing data, received via UDP from controll App

//predefined colors
extern RgbColor black;
extern RgbColor red;
extern RgbColor green;
extern RgbColor blue;
extern RgbColor highliteColor;
extern RgbColor beforeHLColor;
extern bool _highlite;

void OTA_Func(); //For uploading firmware via OTA
void loadSettingsFromFs(); //implement
void saveSettingsToFs(boolean first); //implement
void readUDP(); //reads data from UDP packet
void processRequest();//implment //process data received
void processData(); //implement
void processSetCommand();
void processGetCommand();
void formAnswerInfo(int portOUT);
void setMainSettings();
void setHighliteMode();
void unsetHighliteMode();
void setReset();
void setFormatFS();
void setStaticColor(RgbColor color);
void showStrip();
void setPixelColor(int i, RgbColor color); //wrapper to set sum of color and fx
void outToStrip();
void saveNetworkDataToFs(bool first);
void saveNameToFs(bool first);
void loadNetworkDataFromFs();
void initSettings();
void test2();

void startUdpServer();
void setRandomSsidName();
void processFx();
double speedToDouble(uint8_t speed);
uint8_t speedToInt(double speed);
double widthToDouble(uint8_t parts);
uint8_t widthToInt(double parts);

void moveAnim(const AnimationParam& param);
void fadeAnim(const AnimationParam& param);
void animCyclon(const AnimationParam& param);
void sinus();
void sinusRGB();

void setRandomSeed();
void setupAnimations();
void setupAnimationsCyclon();
void savePlaylist();
void loadPlaylist();
void processPlaylist();
void copyPlaylistSettings(settings_t &set, ledsettings_t &plset);
