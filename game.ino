/*
 * Write and Read personal data of a MIFARE RFID card using a RFID-RC522 reader
 * Uses MFRC522 - Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT. 
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 *
 * Hardware required:
 * Arduino
 * PCD (Proximity Coupling Device): NXP MFRC522 Contactless Reader IC
 * PICC (Proximity Integrated Circuit Card): A card or tag using the ISO 14443A interface, eg Mifare or NTAG203.
 * The reader can be found on eBay for around 5 dollars. Search for "mf-rc522" on ebay.com. 
 */

#include <SPI.h>
#include <MFRC522.h>
#include <MsTimer2.h>
#include <string.h>
#include <stdlib.h>


#define RST_PIN         9           // Configurable, see typical pin layout above
#define SS_PIN          10          // Configurable, see typical pin layout above
MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

//定义一个游戏卡结构体
typedef struct{
  byte Score[48];  //游戏积分
  byte Name[12];    //玩家名称
}Card;

Card card;      //实例化一个游戏卡

byte block;
byte len;

const byte scoreBlockAddr[3] = {1,2,4};   //分数的存储地址

int col[8] = {2,3,4,5,6,7,8,9};
int row[8] = {A2,A3,A4,A5,13,12,11,10};
int px = 4;//记录角色位置，初始时在第五格
int roundNum=0; //记录回合数
int matrix[8][8]={//代表8*8LED的输入矩阵
                {0,0,0,0,0,0,0,0},
                {0,0,0,0,0,0,0,0},
                {0,0,0,0,0,0,0,0},
                {0,0,0,0,0,0,0,0},
                {0,0,0,0,0,0,0,0},
                {0,0,0,0,0,0,0,0},
                {0,0,0,0,0,0,0,0},
                {0,0,0,0,1,0,0,0},
                };
                

                
void setup() {
  //初始化8*8LED
  for(int i=0; i<8; i++){
    pinMode(row[i],OUTPUT);
    pinMode(col[i],OUTPUT);
    digitalWrite(row[i],LOW);
    digitalWrite(col[i],HIGH);
  }
  Serial.begin(9600);
  MsTimer2::set(1000,action);//每1秒更新一次游戏进度
  MsTimer2::start();

}

void loop() {
  show();
}

void show(){
  //将当前矩阵显示在8*8LED上
  for(int i=0; i<8; i++){
    for(int j=0; j<8; j++){
      if(matrix[i][j]==1){
        digitalWrite(row[i],HIGH);
        digitalWrite(col[j],LOW);
        delay(1);
        digitalWrite(row[i],LOW);
        digitalWrite(col[j],HIGH);
      }
    }
  }
}

void action(){
  //游戏内容
  
  runRound();//进行一回合
  if( (roundNum%2)==0){
    newStep();//生成新台阶
  }
  roundNum++;//回合数加1

  //摇杆读数字并判断角色移动
  int rocker  = analogRead(A1); 
  if(rocker <= 600 && rocker>=450 ){
    return;
  }
  if((rocker > 600) && (px!=0) ){
//    Serial.println("左移");  
    matrix[7][px] = 0;
    px--; 
    matrix[7][px] = 1;
  }else if( rocker<450 && px!=7){
//    Serial.println("右移");
    matrix[7][px] = 0;
    px++;
    matrix[7][px] = 1;
  }else if(px==7||px==0){
//    Serial.println("到头");
  }
  
}

void runRound(){
  
  if(isDead()==1){
    //死亡
    
    deathHanlder();
  }else{
    //未死亡，游戏继续
    for(int i=6; i>0; i--){
      for(int j=0; j<8; j++){
        if(matrix[i-1][j] == 1){
          matrix[i][j] = 1;
          matrix[i-1][j]=0;
        }else{
          matrix[i][j] = 0;
        }
      }
    }
  }
}

int isDead(){  
  //判断是否死亡
  if(matrix[6][px]==1){    
    return 1;
  }
  else{    
    return 0;
  }
}

void deathHanlder(){
  //死亡情况处理 
  Serial.print("死亡,");
  MsTimer2::stop();//停止游戏
  MsTimer2::set(500,renew);//每500毫秒进行一次是否复活的指令监听
  MsTimer2::start();
  Serial.print("该局游戏分数:");
  Serial.println(roundNum);
  SPI.end();
}

void newStep(){
  //生成新台阶
  int stepWidth = 2;//台阶宽度
  int xstep = rand()%7;
  for(int i=0; i<stepWidth; i++){
    matrix[0][xstep+i]=1;
  }
}

void renew(){
  //监听复活指令
  int rocker  = analogRead(A0);
  
  if(rocker>900){
    MsTimer2::stop();//停止监听
    //重置矩阵
    for(int i=0; i<8; i++){
      for(int j=0; j<8; j++){
        matrix[i][j] = 0;
      }
    }
    matrix[7][4] = 1;
    px = 4;//重置px
    MsTimer2::set(1000,action);//重新开始游戏
    MsTimer2::start();
  }
  if(rocker<300){
    MsTimer2::stop();//停止监听
    cardHandler(roundNum);//会员卡
    
  }
}
