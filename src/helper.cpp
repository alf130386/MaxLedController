#include <helper.h>

bool toAnswer = false;
bool isFading = false;
uint8_t fred = 0;
uint8_t fgreen = 0;
uint8_t fblue = 0;
uint8_t ffdim = 0;
int fade_ms = 600;
uint8_t fade_frame_dur = 30;

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(200, PIN);

FxController FX;
ledsettings_t *playlist;////
ledsettings_t *playlist_temp;////
uint8_t playlistPeriod = 5;
unsigned long playlistPeriodMs = 5000;
unsigned long playlistLastTime = 0;
uint8_t playlist_counter = 0;

settings_t settings = {
    "esp001\0",
    netMode: 1,
    pixelCount: PIXELCOUNT,
    "SolidNet2\0", //ssid for client mode
    "kmdsdobyf3kmdsdobyf3\0", //password for client mode
    playlistSize: 0,
    playlistMode: false,
    dimmer: 255,
    color: black,
    fxColor: blue,
    strobe: 255,
    fxNumber: 0,
    fxSpeed: 0.3,
    fxSize: 100,
    fxParts: 1,
    fxFade: 0,
    fxParams: 0,
    fxSpread: 1,
    fxWidth: 1,
    startPixel: 0,
    endPixel: PIXELCOUNT-1,
    fxReverse: false,
    fxAttack: false,
};

settings_t request;

RgbColor black(0, 0, 0);
RgbColor red(255, 0, 0);
RgbColor green(0, 255, 0);
RgbColor blue(0, 0, 255);
RgbColor highliteColor(150, 150, 150);
RgbColor beforeHLColor;
bool _highlite = false;

uint8_t hData[5];
uint8_t hData1[13];
uint8_t hData2[17];
char command;
char option;
int mask;
IPAddress sourceIP;
uint8_t uni = UNI;

void sinus() {
    //printf("sinus\n");
    double speed = (settings.fxSpeed/4)*((2*PIP)/FX.fps);
    if(FX.needRecalculate) FX.recalculateTemp();
    for(int i = 0; i < settings.pixelCount; i++) {
        FX.fxTemp[i] = RgbTemp_t(FX.fxTemp[i].R+speed, FX.fxTemp[i].G+speed, FX.fxTemp[i].B+speed);
        RgbTemp_t res(settings.fxColor.R*sin(FX.fxTemp[i].R), settings.fxColor.G*sin(FX.fxTemp[i].G), settings.fxColor.B*sin(FX.fxTemp[i].B));
        if(settings.fxAttack) { //RRRRRRRRRRR
        if(FX.fxData[i].R < res.R) {
        FX.attackTemp[i].R = res.R;
        res.R = settings.fxColor.R;
        }
        else {
            if(res.R > FX.attackTemp[i].R) {
            FX.attackTemp[i].R = res.R;
            res.R = settings.fxColor.R;
            }
        } //GGGGGGGGGGGG
        if(FX.fxData[i].G < res.G) {
        FX.attackTemp[i].G = res.G;
        res.G = settings.fxColor.G;
        }
        else {
            if(res.G > FX.attackTemp[i].G) {
            FX.attackTemp[i].G = res.G;
            res.G = settings.fxColor.G;
            }
        } //BBBBBBBBBBBBBBBBBb
        if(FX.fxData[i].B < res.B) {
        FX.attackTemp[i].B = res.B;
        res.B = settings.fxColor.B;
        }
        else {
            if(res.B > FX.attackTemp[i].B) {
            FX.attackTemp[i].B = res.B;
            res.B = settings.fxColor.B;
            }
        }
        }
        FX.fxData[i] = RgbColor(res.R > 0 ? res.R : 0, res.G > 0 ? res.G : 0, res.B > 0 ? res.B : 0);
    }
    }

void sinusRGB() {
    double speed = settings.fxSpeed;
    if(FX.needRecalculate) FX.recalculateTempRGB();
    for(int i = 0; i < settings.pixelCount; i++) {
        FX.rgbData[i] = FX.rgbData[i] + speed;
        double t = ((int)FX.rgbData[i])%360;
        RgbColor col = HslColor(t/360.0, 1.0f, 0.5);
        FX.fxData[i] = col;
    }
}

void moveAnim(const AnimationParam& param) {
    uint16_t nextPixel;
    float progress = NeoEase::SinusoidalInOut(param.progress);
        if (FX.moveDir > 0)
        {
            nextPixel = progress * (settings.pixelCount-1);
        }
        else
        {
            nextPixel = (1.0f - progress) * (settings.pixelCount-1);
        }

    if (FX.lastPixel != nextPixel)
        {
            for (uint16_t i = FX.lastPixel + FX.moveDir; i != nextPixel; i += FX.moveDir)
            {
            if(i >= settings.pixelCount) break;
                FX.fxData[i] = settings.fxColor;
            }
        }
        //printf("np: %d\n", nextPixel);
        FX.fxData[nextPixel] = settings.fxColor;

        FX.lastPixel = nextPixel;

        if(param.state == AnimationState_Completed) {
        FX.moveDir *= -1;
        FX.animations.RestartAnimation(param.index);
        }
}

    void fadeAnim(const AnimationParam& param) {
       if(param.state == AnimationState_Completed) {
          FX.fadeAll();
           if(FX.animations.IsAnimating()) {
            FX.animations.RestartAnimation(param.index);
           }
           if(FX.animations2.IsAnimating()) {
            FX.animations2.RestartAnimation(param.index);
           }
        }
    }

  void animCyclon(const AnimationParam& param) {
    float progress = NeoEase::Linear(param.progress);
    //float progress_step = progress/settings.fxParts;
    uint16_t fxParts_tmp = settings.fxParts == 1 ? settings.pixelCount : settings.fxParts;
    float progress_step = 1.0f/fxParts_tmp;
    size_t progresses_size = (int)ceil(1/progress_step) +1;
    uint16_t indexes[settings.pixelCount];
    float progresses[102];
    uint16_t* ind_t = indexes;
    float* progr_t = progresses;
    uint16_t j = 0;
    uint16_t tmpind = 0;
    RgbColor colo;
    if(settings.fxRndColor) {
        colo = HslColor(random(360)/360.0f, 1.0f, 0.5);
    }
    else {
        colo = settings.fxColor;
    }
    //printf("progr: %f, fxPartsT: %d, progrSize: %d, indexSize: %d, progrStep: %f\n",progress, fxParts_tmp, progresses_size, indexes_size, progress_step);
    if(settings.fxRnd) {
        if(FX.rndShouldGo == -1) {
        FX.rndShouldGo = 0;
        uint16_t count = settings.fxSpread;
        while (count > 0) {
        uint16_t pixel = random(settings.pixelCount);
        *ind_t = pixel;
        ind_t++; 
        tmpind++;
        count--;
        }
        }

    }
    else{
        while (j < progresses_size)  {
        *progr_t = j * progress_step;
        j++;
        progr_t++;
    }
    *progr_t = 1.0f;
    uint16_t i = 0;
    while(i < (progresses_size - 1)) {
        if((progress >= progresses[i]) && (progress <= progresses[i+1])) {
        for(int k = 0; k < settings.pixelCount; k++) {
            if((k%fxParts_tmp) == i) {
            uint16_t cc = k + fxParts_tmp - (i*2+1);
            //printf("k: %d, cc: %d\n", k, cc);
            if(!settings.fxSymm) {
                *ind_t = settings.fxReverse ? (settings.pixelCount - k - 1) : k;
                ind_t++;
                tmpind++;
            }
            else {
                if(cc < settings.pixelCount && cc >= 0) {
                if(settings.fxReverse) {
                    if(cc >= k) {
                        *ind_t = k;
                        ind_t++;
                        tmpind++;
                        *ind_t = cc;
                        ind_t++;
                        tmpind++;
                    }
                }
                else {
                    if(cc <= k) {
                        *ind_t = k;
                        ind_t++;
                        tmpind++;
                        *ind_t = cc;
                        ind_t++;
                        tmpind++;
                    }
                }
            }
            }
            }
        }
        }
        i++;
    }
    }
    ind_t = indexes;
    uint16_t h = 0;
    //printf("tmpind: %d\n", tmpind);
    if(*indexes != FX.prevIndex) {
        while (h < tmpind)
    {
        //printf("h: %d, ind: %d\n",h, *ind_t);
        if(*ind_t < settings.pixelCount && *ind_t >= 0) {
        FX.fxData[*ind_t] = colo;
        }
        ind_t++;
        h++;
    }
    FX.prevIndex = *indexes;
    }
    if(param.state == AnimationState_Completed) {
        FX.rndShouldGo = -1;
        FX.animations2.RestartAnimation(param.index);
        }
    }

void setupAnimations() {
     FX.animations.StartAnimation(0, 15, fadeAnim);
     FX.animations.StartAnimation(1, ((SPEED_MAX_DOUBLE - settings.fxSpeed)*1000+5), moveAnim);
}

void setupAnimationsCyclon() {
    FX.animations2.StartAnimation(0, 15, fadeAnim);
    FX.animations2.StartAnimation(1, ((SPEED_MAX_DOUBLE - settings.fxSpeed)*1000+5), animCyclon);
}

void setRandomSeed() {
  uint32_t seed;
  seed = analogRead(0);
  delay(1);
  for(int shifts = 3; shifts < 31; shifts += 3) {
    seed ^= analogRead(0) << shifts;
  }
  randomSeed(seed);
}

void initSettings() {
  if(!LittleFS.exists(MAIN_FILE)) {
    printf("**** main_file not exists, creating...\n");
    saveSettingsToFs(true);
  }
  if(!LittleFS.exists(NAME_FILE)) {
    printf("***namefile not exists, creating...\n");
    saveNameToFs(true);
  }
  if(!LittleFS.exists(SSID_FILE)) {
    printf("*** ssid or pass file not exists, creating...\n");
    saveNetworkDataToFs(true);
  }
  if(!LittleFS.exists(PLAYLIST_FILE)) {
    printf("*** playlist file not exists, creating...\n");
    savePlaylist();
  }
    printf("Reading values from mode_file\n");
    loadSettingsFromFs();
    printf("Readed settings\n");
    FX.setSettings(&settings);////
}

void saveSettingsToFs(boolean first) {
  File f;
  if(first) {
    f = LittleFS.open(MAIN_FILE, "w+");
  }
  else {
    f = LittleFS.open(MAIN_FILE, "w");
  }
  if(!f) {
      printf("**** Open mainfile for writing fails\n");
  }
  else {
    uint8_t pixelCountLow = settings.pixelCount;
    uint8_t pixelCountHigh = settings.pixelCount >> 8;
    f.write(settings.netMode); //0
    f.write(pixelCountLow); //1
    f.write(pixelCountHigh); //2
    f.write(settings.playlistSize); //3
    f.write(settings.dimmer); //4
    f.write(settings.color.R); //5
    f.write(settings.color.G); //6
    f.write(settings.color.B); //7
    f.write(settings.fxColor.R); //8
    f.write(settings.fxColor.G); //9
    f.write(settings.fxColor.B); //10
    f.write(settings.strobe); //11
    f.write(settings.fxNumber); //12
    f.write(speedToInt(settings.fxSpeed)); //13
    f.write(settings.fxSize); //14
    f.write(settings.fxParts); //15
    f.write(settings.fxFade); //16
    f.write(settings.fxParams); //17
    printf("***********writing, params: %d\n", settings.fxParams);
    f.write(settings.fxSpread); //18
    f.write(settings.fxWidth); //19
    f.write(settings.startPixel); //20
    f.write(settings.startPixel>>8); //21
    f.write(settings.endPixel); //22
    f.write(settings.endPixel>>8); //23
    delay(50);
    f.close();
  }
  printf("Saved Settings\n");
}

void saveNameToFs(bool first) {
  File f_name = LittleFS.open(NAME_FILE, "w");
  if(!f_name) {
    printf("**** Open namefile for writing fails\n");
  }
  else {
    f_name.print(settings.name);
    delay(50);
    f_name.close();
    printf("name saved\n");
  }
}

void saveNetworkDataToFs(boolean first) {
  File ssidfile = LittleFS.open(SSID_FILE, "w");
  if(!ssidfile) {
    printf("**** Open ssidfile for writing fails\n");
  }
  else{
    ssidfile.print(settings.network);
    delay(50);
    ssidfile.close();
  }

  File passfile = LittleFS.open(PASS_FILE, "w");
  if(!passfile) {
    printf("**** Open passfile for writing fails\n");
  }
  else {
    passfile.print(settings.password);
    delay(50);
    passfile.close();
  }
  printf("Network settings saved\n");
}

void loadSettingsFromFs() {

  File f = LittleFS.open(MAIN_FILE, "r");
  uint8_t temp[24];
  f.read(temp, 24);
  f.close();

  File namefile = LittleFS.open(NAME_FILE, "r");
  size_t nSize = namefile.size();
  settings.name = new char[nSize+1];
  namefile.readBytes(settings.name, nSize);
  namefile.close();
  settings.name[nSize] = '\0';

  File ssidfile = LittleFS.open(SSID_FILE, "r");
  size_t sSize = ssidfile.size();
  settings.network = new char[sSize+1];
  ssidfile.readBytes(settings.network, sSize);
  ssidfile.close();
  settings.network[sSize] = '\0';

  File passfile = LittleFS.open(PASS_FILE, "r");
  size_t pSize = passfile.size();
  settings.password = new char[pSize+1];
  passfile.readBytes(settings.password, pSize);
  passfile.close();
  settings.password[pSize] = '\0';

  settings.netMode = temp[0];
  settings.pixelCount = temp[1] + (temp[2]<<8);
  settings.playlistSize = temp[3];
  settings.dimmer = temp[4];
  settings.color = RgbColor(temp[5], temp[6], temp[7]);
  settings.fxColor = RgbColor(temp[8], temp[9], temp[10]);
  settings.strobe = temp[11];
  settings.fxNumber = temp[12];
  if(settings.fxNumber == 0) 
    FX.fxRunning = false;
  else
  {
    FX.fxRunning = true;
  }
  //FX.previousFxNum = settings.fxNumber;
  settings.fxSpeed = speedToDouble(temp[13]);
  settings.fxSize = temp[14];
  settings.fxParts = temp[15];
  settings.fxFade = temp[16];
  settings.fxParams = temp[17];
  printf("********************** readed: %d\n", temp[17]);
  settings.fxSpread = temp[18];
  settings.fxWidth = temp[19];
  settings.fxReverse = settings.fxParams&1;
  settings.fxAttack = (settings.fxParams>>1)&1;
  settings.fxSymm = (settings.fxParams>>2)&1;
  settings.fxRnd = (settings.fxParams>>3)&1;
  settings.fxRndColor = (settings.fxParams>>7)&1;
  settings.playlistMode = (settings.fxParams>>6)&1;
  settings.startPixel = temp[20] + (temp[21]<<8);
  settings.endPixel = temp[22] + (temp[23]<<8);
  loadPlaylist();
}

void processGetCommand() {
  switch (option)
  {
  case 'S':
    formAnswerInfo(PORT_OUT);
    toAnswer = true;
    break;
  
  default:
    printf("GET: unknown option\n");
    break;
  }
}

void formAnswerInfo(int portOUT) {
  //fillSettingsFromFs(&temp_set);
  //printf("FormAnswerInfo... uni: %d\n", uni);
  wifiUDP.beginPacket(sourceIP, portOUT);
  wifiUDP.write("CP"); //0-1
  wifiUDP.write(uni);
  wifiUDP.write(VERSION);
  wifiUDP.write(2);
  wifiUDP.write(2);
  wifiUDP.write(0);
  wifiUDP.write(0);
  wifiUDP.write(settings.fxNumber);
  wifiUDP.write(settings.fxNumber);
  wifiUDP.write(speedToInt(settings.fxSpeed));
  wifiUDP.write(speedToInt(settings.fxSpeed));
  wifiUDP.write(settings.color.R);
  wifiUDP.write(settings.color.G);
  wifiUDP.write(settings.color.B);
  wifiUDP.write(settings.color.R);
  wifiUDP.write(settings.color.G);
  wifiUDP.write(settings.color.B); //23
  wifiUDP.write(settings.dimmer); //24
  wifiUDP.write(settings.dimmer); //25
  wifiUDP.write(UNI); //26
  wifiUDP.write(0); //27
  wifiUDP.write(0); //28
  wifiUDP.write(0); //29
  wifiUDP.write(settings.pixelCount); //30
  wifiUDP.write(settings.startPixel); //31
  wifiUDP.write(settings.endPixel); //32
  wifiUDP.write(1); //33
  //NEW
  wifiUDP.write(settings.pixelCount >> 8); //34
  wifiUDP.write(settings.startPixel >> 8); //35
  wifiUDP.write(settings.endPixel >> 8); //36
  wifiUDP.write(settings.fxColor.R); //37
  wifiUDP.write(settings.fxColor.G); //38
  wifiUDP.write(settings.fxColor.B); //39
  wifiUDP.write(settings.strobe); //40
  wifiUDP.write(settings.fxSize); //41
  wifiUDP.write(settings.fxParts); //42
  wifiUDP.write(settings.fxFade); //43
  wifiUDP.write(settings.fxParams); //44
  wifiUDP.write(settings.fxSpread); //45
  wifiUDP.write(settings.fxWidth); //46
  wifiUDP.write(settings.netMode); //47
  wifiUDP.write(strlen(settings.name)); //48
  wifiUDP.write(strlen(settings.network)); //49
  wifiUDP.write(strlen(settings.password)); //50
  wifiUDP.write(settings.name);
  wifiUDP.write(settings.network);
  wifiUDP.write(settings.password);
  wifiUDP.endPacket();
}

void processSetCommand() {
  switch(option)
  {
    case 'S':
      setMainSettings();
      break;
    case 'H': 
      setHighliteMode();
      break;
    case 'h':
      unsetHighliteMode();
      break;
    case 'R':
      setReset();
      break;
    case 'F':
      setFormatFS();
      break;

    default:
      printf("SET: unknown option\n");
      break;
  }
}

void savePlaylist() {
  printf("save playlist start\n");
  playlistPeriodMs = playlistPeriod*60*1000;
  ledsettings_t *plset = playlist;
  size_t plSize = settings.playlistSize;
  File f = LittleFS.open(PLAYLIST_FILE, "w");
  playlist_counter = 0;
  if(!f) {
    printf("**** fail to open playlistFile for writing\n");
  }
  else {
    f.write(plSize);
    f.write(playlistPeriod);
    for(int i = 0; i < settings.playlistSize; i++) {
      ledsettings_t set = *plset;
      f.write(set.dimmer);
      f.write(set.color.R);
      f.write(set.color.G);
      f.write(set.color.B);
      f.write(set.fxColor.R);
      f.write(set.fxColor.G);
      f.write(set.fxColor.B);
      f.write(set.strobe);
      f.write(set.fxNumber);
      f.write(speedToInt(set.fxSpeed));
      f.write(set.fxSize);
      f.write(set.fxParts);
      f.write(set.fxFade);
      f.write(set.fxParams);
      f.write(set.fxSpread);
      f.write(set.fxWidth);
      plset++;
    }
    delay(10);
    f.close();
    printf("**** playlist saved\n");
  }
}

void loadPlaylist() {
  printf("loading playlist: size: %d\n", settings.playlistSize);
  File f = LittleFS.open(PLAYLIST_FILE, "r");
  printf("playlist file size: %d\n", f.size());
  if(!f) {
    printf("**** Error playlist file, while loading\n");
  }
  else {
    settings.playlistSize = f.read();
    playlist = new ledsettings_t[settings.playlistSize];
    playlist_temp = playlist;
    ledsettings_t *pset = playlist;
    playlistPeriod = f.read();
    playlistPeriodMs = playlistPeriod*60*1000;
    for(int i = 0; i < settings.playlistSize; i++) {
      ledsettings_t set;
      byte temp[16];
      f.read(temp, 16);
      set.dimmer = temp[0];
      set.color = RgbColor(temp[1], temp[2], temp[3]);
      set.fxColor = RgbColor(temp[4], temp[5], temp[6]);
      set.strobe = temp[7];
      set.fxNumber = temp[8];
      set.fxSpeed = speedToDouble(temp[9]);
      set.fxSize = temp[10];
      set.fxParts = temp[11];
      set.fxFade = temp[12];
      set.fxParams = temp[13];
      set.fxSpread = temp[14];
      set.fxWidth = temp[15];
      set.fxReverse = (set.fxParams)&1;
      set.fxAttack = (set.fxParams>>1)&1;
      set.fxSymm = (set.fxParams>>2)&1;
      set.fxRnd = (set.fxParams>>3)&1;
      set.fxRndColor = (set.fxParams>>7)&1;
      *pset = set;
      pset++;
    }
    f.close();
    //printf("**** loaded playlist items\n");
  }
}

void copyPlaylistSettings(settings_t &set, ledsettings_t &pset) {
  set.dimmer = pset.dimmer;
  set.color = pset.color;
  set.fxColor = pset.fxColor;
  set.strobe = pset.strobe;
  set.fxNumber = pset.fxNumber;
  set.fxSpeed = pset.fxSpeed;
  set.fxSize = pset.fxSize;
  set.fxParts = pset.fxParts;
  set.fxFade = pset.fxFade;
  set.fxParams = pset.fxParams;
  set.fxSpread = pset.fxSpread;
  set.fxWidth = pset.fxWidth;
  set.fxReverse = pset.fxReverse;
  set.fxAttack = pset.fxAttack;
  set.fxSymm = pset.fxSymm;
  set.fxRnd = pset.fxRnd;
  set.fxRndColor = pset.fxRndColor;
}

void setMainSettings() {
  printf("Setting remote settings\n");
  printf("mask: %d\n", mask);
  switch (mask)
  {
  case 1:
    settings.dimmer = request.dimmer;
    break;
  case 2:
    settings.color = request.color;
    break;
  case 4:
    settings.fxSpeed = request.fxSpeed;
    break;
  case 64:
    settings.pixelCount = request.pixelCount;
    settings.startPixel = request.startPixel;
    settings.endPixel = request.endPixel;
    //saveSettingsToFs();
    break;
  case 128:
    isFading = true;
    fred = request.color.R;
    fgreen = request.color.G;
    fblue = request.color.B;
    ffdim = request.dimmer;
    ///////
    settings.fxColor = request.fxColor;
    settings.fxFade = request.fxFade;
    settings.fxNumber = request.fxNumber;
    settings.fxParts = request.fxParts;
    settings.fxParams = request.fxParams;
    settings.fxSize = request.fxSize;
    settings.fxSpeed = request.fxSpeed;
    settings.fxSpread = request.fxSpread;
    if(settings.fxReverse != (request.fxParams&1)) {
      FX.needRecalculate = true;
    }
    settings.fxReverse = request.fxParams&1;
    settings.fxAttack = (request.fxParams>>1)&1;
    if(settings.fxSymm != ((request.fxParams>>2)&1)) {
      FX.needRecalculate = true;
    }
    settings.fxSymm = (request.fxParams>>2)&1;
     if(settings.fxRnd != ((request.fxParams>>3)&1)) {
      FX.needRecalculate = true;
    }
    settings.fxRnd = (request.fxParams>>3)&1;
    settings.fxRndColor = (request.fxParams>>7)&1;
    break;
  case 129:
    settings.fxColor = request.fxColor;
    break;
  case 130:
    settings.fxFade = request.fxFade;
    settings.fxNumber = request.fxNumber;
    settings.fxParts = request.fxParts;
    settings.fxParams = request.fxParams;
    settings.fxSize = request.fxSize;
    if(settings.fxSpeed != request.fxSpeed) {
      FX.speedChanged = true;
    }
    settings.fxSpeed = request.fxSpeed;
    settings.fxSpread = request.fxSpread;
    settings.fxWidth = request.fxWidth;
    if(settings.fxReverse != (request.fxParams&1)) {
      FX.needRecalculate = true;
    }
    settings.fxReverse = request.fxParams&1;
    settings.fxAttack = (request.fxParams>>1)&1;
    if(settings.fxSymm != ((request.fxParams>>2)&1)) {
      FX.needRecalculate = true;
    }
    settings.fxSymm = (request.fxParams>>2)&1;
    if(settings.fxRnd != ((request.fxParams>>3)&1)) {
      FX.needRecalculate = true;
    }
    settings.fxRnd = (request.fxParams>>3)&1;
    settings.fxRndColor = (request.fxParams>>7)&1;
    break;
  case 131: 
    settings.fxParams = request.fxParams;
    settings.playlistMode = (request.fxParams>>6)&1;
    break;
  case 255:
    saveSettingsToFs(false);
    break;
  default:
    printf("**** Unknown mask\n");
    break;
  }
    formAnswerInfo(PORT_OUT_UPD);
}

void setHighliteMode() {
  beforeHLColor = strip.GetPixelColor(1);
  setStaticColor(highliteColor);
  _highlite = true;
}

void unsetHighliteMode() {
  _highlite = false;
  setStaticColor(beforeHLColor);
}

void setReset() {
  ESP.restart();
}

void setFormatFS() {
  LittleFS.format();
  ESP.restart();
}

void processRequest() {
  if(command == 'S') {
    processSetCommand();
  }
  if(command == 'G') {
    processGetCommand();
  }
}

void setStaticColor(RgbColor color) {
  for(int i = 0; i < settings.pixelCount; i++) {
    if(i < settings.startPixel || i > settings.endPixel) {
      strip.SetPixelColor(i, black);
    }
    else {
      //strip.SetPixelColor(i, color);
      setPixelColor(i, color);
    }
  }
  showStrip();
}

void setStaticColorDimmed(uint8_t dimmer, RgbColor col) {
   float koeff = dimmer*1.0/255;
   RgbColor tmp_color(col.R*koeff, col.G*koeff, col.B*koeff);
   setStaticColor(tmp_color);
 }

 void setStaticColorDimmedFaded() {
   double temp_red = settings.color.R;
   double temp_green = settings.color.G;
   double temp_blue = settings.color.B;
   double temp_dim = settings.dimmer;
   double r_step = (fred - temp_red)/(fade_ms/fade_frame_dur);
   double g_step = (fgreen - temp_green)/(fade_ms/fade_frame_dur);
   double b_step = (fblue - temp_blue)/(fade_ms/fade_frame_dur);
   double d_step = (ffdim - temp_dim)/(fade_ms/fade_frame_dur);
   for(int i = 0; i < fade_ms/fade_frame_dur; i++) {
     temp_dim = temp_dim + d_step;
     temp_red = temp_red + r_step;
     temp_green = temp_green + g_step;
     temp_blue = temp_blue + b_step;
     RgbColor tmp_color(temp_red, temp_green, temp_blue);
     setStaticColorDimmed(temp_dim, tmp_color);
     delay(fade_frame_dur);
   }
   settings.dimmer = ffdim;
   settings.color = RgbColor(fred, fgreen, fblue);
   isFading = false;
   formAnswerInfo(PORT_OUT_UPD);
 }

void showStrip() {
  if(!_highlite) {
    strip.Show();
  }
}

void setPixelColor(int i, RgbColor A) {
 // RgbColor A(settings.color.R, settings.color.G, settings.color.B);
  RgbColor B(FX.fxData[i].R, FX.fxData[i].G, FX.fxData[i].B);
  //RgbColor ccolor(RgbColor((B.B == 0 ? A.R : 0) + B.R, (B.B == 0 ? A.G : 0) + B.G, (B.B == 0 ? A.B : 0) + B.B));
  double fxDim = FX.normToDouble(settings.fxSize, 0, 100, 0.0, 1.0);
    RgbColor ccolor(RgbColor(A.R + fxDim*B.R, A.G + fxDim*B.G, A.B + fxDim*B.B));
  //printf("r: %d, g: %d, b: %d\n", ccolor.R, ccolor.G, ccolor.B);
  strip.SetPixelColor(i, ccolor);
}

void outToStrip() {
    if(!isFading) {
      setStaticColorDimmed(settings.dimmer, settings.color);
    }
    else {
      setStaticColorDimmedFaded();
    }
}


void test2() {
  RgbColor blu = RgbColor(0, 0, 50);
  for(int i = 0; i < 3; i++) {
    strip.SetPixelColor(settings.startPixel, blu);
    strip.SetPixelColor(settings.endPixel, blu);
    showStrip();
    delay(100);
    strip.SetPixelColor(settings.startPixel, black);
    strip.SetPixelColor(settings.endPixel, black);
    showStrip();
    delay(200);
  }
}

uint8_t speedToInt(double speed) {
  double result;
  double scale = (1.0*(SPEED_MAX_INT - SPEED_MIN_INT))/(SPEED_MAX_DOUBLE - SPEED_MIN_DOUBLE);
  if((SPEED_MIN_DOUBLE == 0 && SPEED_MIN_INT == 0) || (SPEED_MIN_DOUBLE != 0 && SPEED_MIN_INT != 0)) {
    result = speed*scale;
  }
  else if(SPEED_MIN_DOUBLE == 0) {
    result = speed*scale + SPEED_MIN_INT;
  }
  else if(SPEED_MIN_INT == 0) {
    result = speed*scale + SPEED_MAX_INT;
  }
  return result;
}

double speedToDouble(uint8_t speed) {
  double result;
  double scale = (SPEED_MAX_DOUBLE - SPEED_MIN_DOUBLE)/(SPEED_MAX_INT - SPEED_MIN_INT);
   if((SPEED_MIN_DOUBLE == 0 && SPEED_MIN_INT == 0) || (SPEED_MIN_DOUBLE != 0 && SPEED_MIN_INT != 0)) {
    result = speed*scale;
  }
  else if(SPEED_MIN_INT == 0) {
    result = speed*scale + SPEED_MIN_DOUBLE;
  }
  else if(SPEED_MIN_DOUBLE == 0) {
    result = speed*scale + SPEED_MAX_DOUBLE;
  }
  return result;
}

uint8_t widthToInt(double width) {
  double result;
  double scale = (1.0*(WIDTH_MAX_INT - WIDTH_MIN_INT))/(WIDTH_MAX_DOUBLE - WIDTH_MIN_DOUBLE);
  if((WIDTH_MIN_DOUBLE == 0 && WIDTH_MIN_INT == 0) || (WIDTH_MIN_DOUBLE != 0 && WIDTH_MIN_INT != 0)) {
    result = width*scale;
  }
  else if(WIDTH_MIN_DOUBLE == 0) {
    result = width*scale + WIDTH_MIN_INT;
  }
  else if(WIDTH_MIN_INT == 0) {
    result = width*scale + WIDTH_MAX_INT;
  }
  return result;
}

double widthToDouble(uint8_t width) {
  double result;
  double scale = (WIDTH_MAX_DOUBLE - WIDTH_MIN_DOUBLE)/(WIDTH_MAX_INT - WIDTH_MIN_INT);
   if((WIDTH_MIN_DOUBLE == 0 && WIDTH_MIN_INT == 0) || (WIDTH_MIN_DOUBLE != 0 && WIDTH_MIN_INT != 0)) {
    result = width*scale;
  }
  else if(WIDTH_MIN_INT == 0) {
    result = width*scale + WIDTH_MIN_DOUBLE;
  }
  else if(WIDTH_MIN_DOUBLE == 0) {
    result = width*scale + WIDTH_MAX_DOUBLE;
  }
  return result;
}



//OTA - Flashing over Air
void OTA_Func() {
    ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  }