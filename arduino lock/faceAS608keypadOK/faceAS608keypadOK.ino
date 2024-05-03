#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>  // 引用I2C序列顯示器的程式庫
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>

#define RST_PIN         5        
#define SS_PIN          53 //RC522卡上的SDA

MFRC522 mfrc522;   // 建立MFRC522實體

char *reference;

byte uid[]={0x05, 0xD6, 0xCC, 0x43};  //這是我們指定的卡片UID，可由讀取UID的程式取得特定卡片的UID，再修改這行

#define KEY_ROWS 4  // 薄膜按鍵的列數
#define KEY_COLS 4  // 薄膜按鍵的行數
#define LCD_ROWS 2  // LCD顯示器的列數
#define LCD_COLS 16 // LCD顯示器的行數

/*************************************
************
*
   Public Constants
 *************************************************/

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};


#define RELAY 14

// 設置按鍵模組
char keymap[KEY_ROWS][KEY_COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[KEY_ROWS] = {13, 12, 11, 10};
byte colPins[KEY_COLS] = {9, 8, 7, 6};

Keypad keypad = Keypad(makeKeymap(keymap), rowPins, colPins, KEY_ROWS, KEY_COLS);

String passcode = "4321";   // 預設密碼
String inputCode = "";      // 暫存用戶的按鍵字串
bool acceptKey = true;      // 代表是否接受用戶按鍵輸入的變數，預設為「接受」

// LCD顯示器
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 設定模組位址0x27，以及16行, 2列的顯示形式

//SoftwareSerial mySerial(11, 12);   //PIN2是RX，接到AS608的TX；PIN3是TX，接到AS608的RX
//#define mySerial Serial1
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&Serial1);

void clearRow(byte n) {
  byte last = LCD_COLS - n;
  lcd.setCursor(n, 1); // 移動到第2行，"PIN:"之後

  for (byte i = 0; i < last; i++) {
    lcd.print(" ");
  }
  lcd.setCursor(n, 1);
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

// 顯示「歡迎光臨」後，重設LCD顯示文字和輸入狀態。
void resetLocker() {
  lcd.clear();
  lcd.print("Knock, knock...");
  lcd.setCursor(0, 1);  // 切換到第2行
  lcd.print("PIN:");
  lcd.cursor();

  pinMode(53, OUTPUT);
  digitalWrite(53,LOW); 
  acceptKey = true;
  inputCode = "";
}

// 比對用戶輸入的密碼
void checkPinCode() {
  acceptKey = false;  // 暫時不接受用戶按鍵輸入
  clearRow(0);        // 從第0個字元開始清除LCD畫面
  lcd.noCursor();
  lcd.setCursor(0, 1);  // 切換到第2行
  // 比對密碼
  if (inputCode == passcode) {
    lcd.print("Welcome home!");
     for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(37, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(8);
  }
     digitalWrite(RELAY,HIGH);
         digitalWrite(37,LOW);
    delay(3000);
    digitalWrite(RELAY,LOW);
        digitalWrite(37,HIGH);
        
              digitalWrite(2, HIGH);
      delay(2000);
      digitalWrite(2, LOW);

  } else {
    lcd.print("***WRONG!!***");
  }


  delay(3000);
  resetLocker();     // 重設LCD顯示文字和輸入狀態
}

// 如果回傳-1，則是沒有比對到已紀錄的指紋；比對成功則回傳對映的ID
int getFingerprintIDez() {
  //Serial.println("getFingerprintIDez");
  uint8_t p = finger.getImage();  //取得指紋
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK){
    lcd.noBlink();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ACCESS DENIED!");
    Serial.println("DENIED");
    delay(5000);
    lcd.clear();
    lcd.setCursor(0, 0);  //設定游標位置 (字,行)
    lcd.print("Put your finger");
    lcd.setCursor(0, 1); 
    lcd.print("for indentify..");
    lcd.setCursor(15, 1); //第二行最後一個字閃動
    lcd.blink();
    return -1;
  }

  //以下是成功取得配對後才會進行的程序
  //finger.fingerID就是當初該指紋記錄的ID
  //finger.confidence則是比對的分數，0~255
  Serial.print("Found ID #"); Serial.print(finger.fingerID); 
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  //如果finger.confidence分數大於100，表示驗証通過。
  //就在LCD顯示通過的訊息，並播放音效
  if(finger.confidence > 50){

    //設定ID所對映的人名，這部份要自行修改;ID沒有0，所以第一個隨意取
    char idname[][16]={"Nobody","USER"};
    //在LCD顯示通過的人名
    lcd.noBlink();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(idname[finger.fingerID]);
    lcd.setCursor(0, 1);
    lcd.print("ACCESS GRANTED!");
    
     for (int thisNote = 0; thisNote < 8; thisNote++) {
    
        // to calculate the note duration, take one second divided by the note type.
        //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(37, melody[thisNote], noteDuration);
    
        // to distinguish the notes, set a minimum time between them.
        // the note's duration + 30% seems to work well:
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
        // stop the tone playing:
        noTone(8);
      }
      
      digitalWrite(RELAY,HIGH);
      digitalWrite(37,LOW);
      delay(3000);
      digitalWrite(RELAY,LOW);
      digitalWrite(37,HIGH);
      lcd.clear();
      digitalWrite(2, HIGH);
      delay(2000);
      digitalWrite(2, LOW);
  }
  
  return finger.fingerID; 
}




String getUID() {
  String content= "";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) 
  {
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  content.toUpperCase();
  return content.substring(1);
}

void setup() {
  Serial.begin(9600);

  lcd.init();       // 初始化lcd物件
  lcd.backlight();  // 開啟背光

  resetLocker();

  // 設定AS608的Baud Rate
  finger.begin(57600);
  delay(5);

  //判斷是否可以使用指紋模組
  if (finger.verifyPassword()) {
    //Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  //偵測目前有幾組已記錄的指紋
  finger.getTemplateCount();
  //Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");

  //等待指紋
  //Serial.println("Waiting for valid finger...");

  SPI.begin();
  
  mfrc522.PCD_Init(SS_PIN, RST_PIN); // 初始化MFRC522卡
  //Serial.print(F("Reader "));
  //Serial.print(F(": "));
  mfrc522.PCD_DumpVersionToSerial(); // 顯示讀卡設備的版本SPI.begin();

  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);

}

int mode=0;

String s="";
void loop() 
{
if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      // 顯示卡片的UID
      //Serial.print(F("Card UID:"));
      String uid = getUID();
      /*dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size); // 顯示卡片的UID
      //Serial.println();
      //Serial.print(F("PICC type: "));
      MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
      //Serial.println(mfrc522.PICC_GetTypeName(piccType));  //顯示卡片的類型
      
      //把取得的UID，拿來比對我們指定好的UID
      bool they_match = true; // 初始值是假設為真 
      String s="";
      for ( int i = 0; i<4; i++ ) { // 卡片UID為4段，分別做比對
        s=s+mfrc522.uid.uidByte[i];
        if ( uid[i] != mfrc522.uid.uidByte[i] ) { 
          they_match = false; // 如果任何一個比對不正確，they_match就為false，然後就結束比對
          break; 
        }
      }*/
      Serial.print("CARD ");
      Serial.println(s);
      //在監控視窗中顯示比對的結果
      if(uid=="DC 34 02 38"){
        //Serial.print(F("Access Granted!"));
        digitalWrite(RELAY,HIGH);
        digitalWrite(37,LOW);
        delay(3000);
        digitalWrite(RELAY,LOW);
        digitalWrite(37,HIGH);
      }else{
        //Serial.print(F("Access Denied!"));
        
      }
      mfrc522.PICC_HaltA();  // 卡片進入停止模式
      mfrc522.PCD_Init(SS_PIN, RST_PIN);
      digitalWrite(2,HIGH);
      delay(1000);
      digitalWrite(2,LOW);
      
      SPI.begin();  
      
      return;
    }

 if(Serial.available()>0)
 {
  char c = Serial.read();
  if (c=='1')
  {
    Serial.println("open door");
    for (int thisNote = 0; thisNote < 8; thisNote++) {

        // to calculate the note duration, take one second divided by the note type.
        //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
        int noteDuration = 1000 / noteDurations[thisNote];
        tone(37, melody[thisNote], noteDuration);
    
        // to distinguish the notes, set a minimum time between them.
        // the note's duration + 30% seems to work well:
        int pauseBetweenNotes = noteDuration * 1.30;
        delay(pauseBetweenNotes);
        // stop the tone playing:
        noTone(8);
     }
    digitalWrite(RELAY,HIGH);
    digitalWrite(37,LOW);
    delay(5000);
    digitalWrite(RELAY,LOW);
    digitalWrite(37,HIGH);
  }
 }
  
  lcd.setCursor(0, 0);  //設定游標位置 (字,行)
  lcd.print("*MODE");
  if (mode == 0)
  {
    lcd.setCursor(0, 1); 
    lcd.print("INPUT: ");
  }
  else
  {
    lcd.setCursor(0, 1); 
    lcd.print("FINGER: ");
  }
  
  char key = keypad.getKey();

  
    if (mode == 1)
    {
       getFingerprintIDez();
      lcd.clear();
       s="";
    }

  // 若目前接受用戶輸入，而且有新的字元輸入…
  if (acceptKey && key != NO_KEY) 
  {
    
    //Serial.println(key);
    if (key == '*') {   // finger
        if (mode==1)
        {
          mode=0;
        }
        else
        {
          mode=1;
        }
      }
      else if (mode == 0)
    { 
      if (key == '#') {  // 比對輸入密碼
        checkPinCode();
        s="";
      } else {
        inputCode += key;  // 儲存用戶的按鍵字元
        s+="*";
        //Serial.println("key");
        lcd.print(s);
      }
    }
  
    }
  
    

    

  
}
