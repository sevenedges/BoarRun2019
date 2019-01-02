#include <LiquidCrystal.h>

////////////////////////////////////////////////////////////////

// 液晶 LCD1602A
// RS#12, ENABLE#11, D4#6, D5#5, D6#4, D7#3
LiquidCrystal lcd(12, 11, 6, 5, 4, 3);

// ボタン #13
const int BUTTON = 13;

// LED #10
const int LED = 10;

// ブザー #2
const int BUZZER = 2;

////////////////////////////////////////////////////////////////

// キャラクタードットサイズ
const int CHAR_X = 5;
const int CHAR_Y = 8;
// グリッドドットサイズ（キャラクター＋マージン）
const int GRID_X = 6;
const int GRID_Y = 9;

// イノシシ左半身($4,$6)
const byte BOAR_L[4][9] = {
  { B00011, B10111, B11111, B01111, B01111, B00111, B01000, B00000, B00000 },
  { B00000, B10111, B11111, B01111, B01111, B00111, B00101, B00000, B00000 },
  { B00000, B00011, B10111, B11111, B01111, B01111, B00111, B00010, B00000 },
  { B00000, B00011, B00111, B11111, B11111, B01111, B00111, B01000, B00000 }
};

// イノシシ右半身($5,$7)
const byte BOAR_R[4][9] = {
  { B11000, B11100, B10111, B11111, B11011, B11100, B00010, B00000, B00000 },
  { B00000, B11000, B11100, B10111, B11111, B11011, B11100, B00010, B00000 },
  { B00000, B11000, B11100, B10111, B11111, B11011, B11100, B01000, B00000 },
  { B00000, B11100, B10111, B11111, B11011, B11100, B00100, B00000, B00000 }
};

// 昇天イノシシ左($6)
const byte DEAD_L[8] = {
  B00000, B00010, B00111, B01111, B01111, B11111, B10111, B11111
};

// 昇天イノシシ右($7)
const byte DEAD_R[8] = {
  B00000, B01000, B11100, B11011, B11111, B00111, B11100, B11111
};

// 昇天リング左($4,$6)
const byte RING_L[2][7] = {
  { B00000, B00011, B00100, B00011, B00000, B00001, B00000 },
  { B00000, B00011, B00100, B00011, B00000, B00000, B00000 }
};

// 昇天リング右($5,$7)
const byte RING_R[2][7] = {
  { B00000, B11000, B00100, B11000, B00000, B00000, B00000 },
  { B00000, B11000, B00100, B11000, B00000, B10000, B00000 }
};

// 障害物($2,$3)
const byte SNAG[8] = {
  B0000000, B0000000, B0000000, B0000000,
  B0001000, B0011100, B0111110, B1111111
};

// 平地($1)
byte plane[CHAR_Y] = {
  B00000, B00000, B00000, B00000,
  B00000, B00000, B00000, B11111
};

// 空白($0)
byte blank[CHAR_Y]{
  B00000, B00000, B00000, B00000,
  B00000, B00000, B00000, B00000
};

////////////////////////////////////////////////////////////////

// ステージパターン（障害物の間隔[2以上]）
const int NEXT_SNAG[6][6] = {
  { 4, 4, 4, 5, 6, 7 },
  { 3, 4, 4, 4, 5, 6 },
  { 3, 3, 4, 4, 5, 6 },
  { 2, 3, 3, 4, 4, 5 },
  { 2, 2, 3, 3, 4, 5 },
  { 2, 2, 3, 3, 3, 4 }
};
const int STAGE_NEXT_SNAG_LEN = 6;
const int STAGE_NEXT_SNAG_LV_LEN = 6;

unsigned long stageMap;           // ステージマップ（32bit）
int stageScrollH, stageScrollL;   // スクロール位置（High:キャラ単位/Low:ドット単位）
int stageScrollDelay;             // スクロールディレイ（速度）
int stageCount;                   // 最後の障害物からのカウンタ
int stageNextSnag[6];             // 次の障害物までの間隔データ配列
int stageNextSnagLv;              // 配列生成時に考慮するレベル
int stageNextSnagID;              // 配列ID

const int SCROLL_DELAY_SLOW = 10;
const int SCROLL_DELAY_FAST = 2;

////////////////////////////////////////////////////////////////

// ゲームフェイズ
int phase;
const int PHASE_TITLE_START   = 10;
const int PHASE_TITLE_MAIN    = 11;
const int PHASE_TITLE_END     = 12;
const int PHASE_GAME_START    = 20;
const int PHASE_GAME_MAIN     = 21;
const int PHASE_GAME_MISS     = 22;
const int PHASE_GAME_END      = 23;

// 汎用タイマ
int timer;

// タイトル
const String MES_TITLE = "BOAR RUNNER 2019";
const String MES_PRESS = "  PRESS START!  ";
const String MES_START = "HAVE A NICE RUN!";
const String MES_BLANK = "                ";
const int BLINK_DELAY = 500;          // 明滅サイクル（ボタン待機中）
const int BLINK_DELAY_PRESSED = 100;  // 明滅サイクル（ボタン押下後）
const int TIME_PRESSED = 1500;        // 明滅待機（ボタン押下後）
bool visible;                         // 明滅フラグ

// イノシシ
// 登録用パターン（4キャラ分）
byte boarLT[CHAR_Y], boarRT[CHAR_Y], boarLB[CHAR_Y], boarRB[CHAR_Y];
int boarJump = 0;               // ジャンプ加速度
int boarHeight = 0;             // ジャンプ高さ
int boarAnime = 0;              // コマアニメ番号
const int BOAR_POS = 2;         // 表示座標
const int BOAR_HEIGHT_MAX = 9;  // ジャンプ高さ最大
const int BOAR_ANIME_LEN = 4;   // コマアニメ枚数
const int BOAR_JUMP_ANIME = 0;  // ジャンプ時アニメ番号
int ringHeight = 0;             // 昇天高さ

// スコア
unsigned long score;
const int scoreAdd = 1;

// LED
bool isLight;

// SETUP
void setup() {
  pinMode(BUTTON, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(LED, OUTPUT);
  lcd.begin(16, 2);
  lcd.noAutoscroll();
  isLight = false;
  digitalWrite(LED, LOW);
  //
  phase = PHASE_TITLE_START;
}

// LOOP
void loop() {
  switch(phase){
    case PHASE_TITLE_START:
      TitleStart();
      break;
    case PHASE_TITLE_MAIN:
      TitleMain();
      break;
    case PHASE_TITLE_END:
      TitleEnd();
      break;
    case PHASE_GAME_START:
      GameStart();
      break;
    case PHASE_GAME_MAIN:
      GameMain();
      break;
     case PHASE_GAME_MISS:
      GameMiss();
      break;
     case PHASE_GAME_END:
      GameEnd();
      break;
     default:
      break;
  }
}

// タイトル開始
void TitleStart() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(MES_TITLE);
  visible = true;
  timer = 0;
  isLight = false;
  digitalWrite(LED, LOW);
  phase = PHASE_TITLE_MAIN;
}

// タイトルメイン
void TitleMain(){
  if (visible){
    lcd.setCursor(0, 1);
    lcd.print(MES_PRESS);
  }
  else{
    lcd.setCursor(0, 1);
    lcd.print(MES_BLANK);
  }
  visible = !visible;
  delay(BLINK_DELAY);
  //
  if (digitalRead(BUTTON) == HIGH){
    timer = 0;
    phase = PHASE_TITLE_END;
  }
}

// タイトル終了
void TitleEnd(){
  if (visible){
    lcd.setCursor(0, 1);
    lcd.print(MES_START);
  }
  else{
    lcd.setCursor(0, 1);
    lcd.print(MES_BLANK);
  }
  visible = !visible;
  timer += BLINK_DELAY_PRESSED;
  // Sound
  if (timer < BLINK_DELAY_PRESSED * 5)
    tone (BUZZER, 440 + timer / 2, BLINK_DELAY_PRESSED);
  // LED
  isLight = !isLight;
  digitalWrite(LED, isLight ? HIGH : LOW);
  //
  if (timer >= TIME_PRESSED){
  digitalWrite(LED, LOW);
    phase = PHASE_GAME_START;
  }
  delay(BLINK_DELAY_PRESSED);
}

// ゲーム開始
void GameStart() {
  lcd.clear();
  lcd.createChar(1, plane);
  for (int i = 0; i < 4; i++){
    lcd.setCursor(BOAR_POS + i % 2, i < 2 ? 0 : 1);
    lcd.write(byte(i + 4));
  }
  stageScrollDelay = SCROLL_DELAY_SLOW;
  randomSeed(millis());

  CreateStageMap();
  score = 0;
  phase = PHASE_GAME_MAIN;
}

// ゲームメイン
void GameMain(){
  JumpBoar();
  SetBoar();
  ScrollStage();
  DrawGame();
  CountScore();
  CheckSnag();
  // Sound
  tone (BUZZER, 220 - sin(millis()) * 110 + boarHeight * 55, 30);
  delay(stageScrollDelay);
}

// ゲームミス演出
void GameMiss(){
  for (int i = 0; i < CHAR_Y; i++){
    boarLT[i] = boarRT[i] = 0;
    boarLB[i] = DEAD_L[i];
    boarRB[i] = DEAD_R[i];
  }
  DrawGame();
  ringHeight++;
  timer += 40;
  // Sound
  tone (BUZZER, 660 - ringHeight * 22, 40);
  // LED
  isLight = !isLight;
  digitalWrite(LED, isLight ? HIGH : LOW);
  if (timer > 800){
    timer = 0;
    digitalWrite(LED, LOW);
    phase = PHASE_GAME_END;
  }
  delay(40);
}

// ゲーム終了
void GameEnd(){
  timer += 20;
  if (timer > 4000 || digitalRead(BUTTON) == HIGH){
    timer = 0;
    phase = PHASE_TITLE_START;
  }
  delay(20);
}

// コース初期化
void CreateStageMap() {
  stageMap = 0;
  stageScrollH = stageScrollL = 0;
  stageCount = 0;
  stageNextSnagID = 0;
  stageNextSnagLv = 0;
  randomSeed(millis());
  ExtendStage();
}

// ステージマップのスクロールと更新
void ScrollStage() {
  stageScrollL++;
  if (stageScrollL >= GRID_X){
    stageScrollL = 0;
    stageScrollH++;
    stageMap = stageMap << 1;
    //
    stageCount++;
    if (stageCount > stageNextSnag[stageNextSnagID]){
      stageMap |= 1;
      stageCount = 0;
      stageNextSnagID++;
      if (stageNextSnagID >= STAGE_NEXT_SNAG_LEN){
        stageNextSnagID = 0;
        stageNextSnagLv++;
        ExtendStage();
      }
    }
    stageScrollDelay = map(constrain(stageNextSnagLv, 0, 7), 0, 7, SCROLL_DELAY_SLOW, SCROLL_DELAY_FAST);
  }
}

// ステージマップの延伸
void ExtendStage(){
  int lv = constrain(stageNextSnagLv, 0, STAGE_NEXT_SNAG_LV_LEN - 1);
  for (int i = 0; i < STAGE_NEXT_SNAG_LEN; i++){
    stageNextSnag[i] = NEXT_SNAG[lv][i];
  }
  // 配列シャッフル
  for (int i = 0; i < STAGE_NEXT_SNAG_LEN - 1; i++){
    int r = random(i + 1, STAGE_NEXT_SNAG_LEN);
    int tmp = stageNextSnag[i];
    stageNextSnag[i] = stageNextSnag[r];
    stageNextSnag[r] = tmp; 
  }
}

// 障害物衝突判定
void CheckSnag(){
  if (boarHeight > 0) return;
  byte bts = (stageMap >> (14 - BOAR_POS)) & B11;
  if (((bts & B10) && stageScrollL < 6) || ((bts & B01) && stageScrollL > 3)){
    phase = PHASE_GAME_MISS;
    timer = 0;
    ringHeight = 0;
  }
}

// スコアの加算と描画
void CountScore(){
  // スコア加算
  score += scoreAdd;
  // 表示桁数
  int digits = 8;
  unsigned long d = 10;
  for (int i = 0; i < digits; i++){
    lcd.setCursor(15 - i, 0);
    unsigned int s = (score % d) / (d / 10);
    lcd.print(s);
    d *= 10;
  }
}

// ゲーム画面全体の描画
void DrawGame() {
  // 障害物（スクロールに応じてキャラクター登録する）
  byte snagL[CHAR_Y], snagR[CHAR_Y];
  int t = stageScrollL;
  for (int i = 0; i < CHAR_Y - 1; i++){
    snagL[i] = (SNAG[i] >> (GRID_X - t)) & B11111;
    snagR[i] = (SNAG[i] << t) & B11111;
  }
  snagL[CHAR_Y - 1] = snagR[CHAR_Y - 1] = plane[CHAR_Y - 1];
  lcd.createChar(2, snagL);
  lcd.createChar(3, snagR);
  // マップ（平地＋障害物）
  byte chr[16];
  for (int i = 0; i < 16; i++){
    byte bt = (stageMap >> (15 - i)) & B11;
    if (bt & B10){
      chr[i] = 3; // $障害物（右）
    }
    else if (bt & B01){
      chr[i] = 2; // $障害物（左）
    }
    else{
      chr[i] = 1; // $平地
    }
    // イノシシ表示位置（右）にマップを合成する
    if (i == BOAR_POS){
      for (int j = 0; j < 8; j++){
        byte b = chr[i] == 1 ? plane[j] : chr[i] == 2 ? snagL[j] : chr[i] == 3 ? snagR[j] : 0;
        boarLB[j] = boarLB[j] | b;
      }
    }
    // イノシシ表示位置（左）にマップを合成する
    else if (i == BOAR_POS + 1){
      for (int j = 0; j < 8; j++){
        byte b = chr[i] == 1 ? plane[j] : chr[i] == 2 ? snagL[j] : chr[i] == 3 ? snagR[j] : 0;
        boarRB[j] = boarRB[j] | b;
      }
    }
    // それ以外（マップのみ）
    else{
      lcd.setCursor(i, 1);
      lcd.write(chr[i]);
    }
  }
  if (phase == PHASE_GAME_MISS){
    int pt = ringHeight % 2;
    for (int i = 0; i < CHAR_Y; i++){
      int j = i + ringHeight;
      boarLB[i] |= j < 7 ? RING_L[pt][j] : 0;
      boarRB[i] |= j < 7 ? RING_R[pt][j] : 0;
      j = i + ringHeight - GRID_Y;
      boarLT[i] |= (j >= 0 && j < 7) ? RING_L[pt][j] : 0;
      boarRT[i] |= (j >= 0 && j < 7) ? RING_R[pt][j] : 0;
    }
  }
  // イノシシ（マップ合成後）
  lcd.createChar(4, boarLT);
  lcd.createChar(5, boarRT);
  lcd.createChar(6, boarLB);
  lcd.createChar(7, boarRB);
}

// イノシシのジャンプ挙動
void JumpBoar() {
  // ジャンプ中（自由落下）
  if (boarHeight > 0){
    boarHeight = max(0, boarHeight + boarJump / 3); // 調整
    boarJump--;
  }
  // ジャンプ待機
  else{
    // ジャンプ開始
    if (digitalRead(BUTTON) == HIGH){
      boarHeight = 3; // 調整
      boarJump = 7;   // 調整
    }
  }
}

// イノシシのキャラクター登録
void SetBoar() {
  int ht = min(boarHeight, BOAR_HEIGHT_MAX);
  int pt = boarHeight == 0 ? boarAnime : BOAR_JUMP_ANIME;
  for (int i = 0; i < CHAR_Y; i++){
    int j = i + ht + 1;
    boarLB[i] = j < 9 ? BOAR_L[pt][j] : 0;
    boarRB[i] = j < 9 ? BOAR_R[pt][j] : 0;
    j = i + ht - 8;
    boarLT[i] = j >= 0 ? BOAR_L[pt][j] : 0;
    boarRT[i] = j >= 0 ? BOAR_R[pt][j] : 0;
  }
  //コマアニメ
  if (phase == PHASE_GAME_MAIN){
    boarAnime = (boarAnime + 1) % BOAR_ANIME_LEN;
  }
}
