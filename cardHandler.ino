void cardHandler(int roundNum){
    SPI.begin();               // Init SPI bus
    mfrc522.PCD_Init();        // Init MFRC522 card
    byte count = 0;
    //read a PICC card
    Serial.println("等待读取会员卡");
    delay(10);
    // Look for new cards
    while ( ! mfrc522.PICC_IsNewCardPresent()) 
    {
      delay(1000);
      if(count++>6000)
      {
        Serial.println("等待时间过长…");
        count = 0;
        return;
      }
    }

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) {
      return;
    }

    Serial.println(F("**Card Detected:**"));

    mfrc522.PICC_DumpDetailsToSerial(&(mfrc522.uid)); //dump some details about the card

    //mfrc522.PICC_DumpToSerial(&(mfrc522.uid));      //uncomment this to see all blocks in hex

    len = 18;
    for(uint8_t i = 0; i < sizeof(scoreBlockAddr);i++)
    {
      status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, scoreBlockAddr[i], &key, &(mfrc522.uid)); //line 834 of MFRC522.cpp file
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }

      status = mfrc522.MIFARE_Read(scoreBlockAddr[i], &card.Score[i*16], &len);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Reading failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
    }

    //PRINT FIRST NAME
    Serial.print(F("游戏分数: "));
    Serial.print(roundNum-1);
 
    delay(10);//等待数据全部进入缓冲区
    
    char string[] = "";
    itoa(roundNum,string,10);
    
    for(byte i = 0; i < sizeof(string); i++)
    {
      card.Score[i] = string[i]+card.Score[i];
    }
    
    Serial.println("，积分后总分数：");
    for (uint8_t i = 0; i < sizeof(card.Score); i++)
    {
      Serial.write(card.Score[i]); 
    }

    for(byte i = 0; i < sizeof(scoreBlockAddr); i++)
    {
      status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,  scoreBlockAddr[i], &key, &(mfrc522.uid));
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("PCD_Authenticate() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
      else Serial.println(F("PCD_Authenticate() success: "));

      // Write block
      status = mfrc522.MIFARE_Write(scoreBlockAddr[i], &card.Score[i*16], 16);
      if (status != MFRC522::STATUS_OK) {
        Serial.print(F("MIFARE_Write() failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
      }
      else Serial.println(F("MIFARE_Write() success: "));
    }

    SPI.end();
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    MsTimer2::set(1000,action);//重新开始游戏
    MsTimer2::start();
    
}
