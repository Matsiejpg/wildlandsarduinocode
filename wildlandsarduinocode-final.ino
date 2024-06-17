#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);

struct MessageValue {
  String message;
  String value;
};

MessageValue getMessage(String inputtedStr) {
  MessageValue result;
  char charArr[50];
  inputtedStr.toCharArray(charArr, 50);
  char* ptr = strtok(charArr, "||");
  result.message = String(ptr);
  ptr = strtok(NULL, "||");
  result.value = (ptr == NULL) ? String("") : String(ptr);
  return result;
}

struct NfcData {
  String uid;
  String naam;
  String koppelCode;
  String nfcCode;
  bool assigned;
  bool assignedAgain;
  bool gekoppeld;
  int punten;
  int questCompleted;
};

NfcData nfcOne = {"09cb77e2", "", "54312", "54312", false, false, false, 0, 0};
NfcData nfcTwo = {"f9f927c3", "", "98275", "98275", false, false, false, 0, 0};

String naam = "";
bool questStarted = false;
int puntenOptellen = 0;
String questStartedNfc = "";
String questStartedNaam = "";
bool questReady = false;
String messageCode = "";
String messageContent = "";

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("NFC SCANNER IS READY");
  Serial.setTimeout(10);
}

void loop() {
  if (Serial.available() > 0) {
    String receivedString = Serial.readStringUntil('\0');
    MessageValue receivedData = getMessage(receivedString);

    messageCode = receivedData.value.substring(0, 5);
    messageContent = receivedData.value.substring(5);

    if (receivedData.message.equals("NAAM")) {
      if (messageCode == nfcOne.nfcCode) {
        nfcOne.naam = messageContent;
        Serial.println("NAAM GEKOPPELD||" + nfcOne.nfcCode + "\n");
        Serial.println(nfcOne.naam + "\n");
      } else if (messageCode == nfcTwo.nfcCode) {
        nfcTwo.naam = messageContent;
        Serial.println("NAAM GEKOPPELD||" + nfcTwo.nfcCode + "\n");
        Serial.println(nfcTwo.naam + "\n");
      }
    } else if (receivedData.message.equals("PUNTEN")) {
      if (messageCode == nfcOne.nfcCode) {
        nfcOne.punten += messageContent.toInt();
        Serial.println("VOUCHER PUNTEN||" + nfcOne.nfcCode + "\n");
        Serial.println(nfcOne.punten + "\n");
        Serial.println("GNAAM||" + nfcOne.naam + "\n");
        Serial.println("GPUNTEN||" + String(nfcOne.punten) + "\n");
        Serial.println("NFCCODE||" + nfcOne.nfcCode + "\n");
      } else if (messageCode == nfcTwo.nfcCode) {
        nfcTwo.punten += messageContent.toInt();
        Serial.println("VOUCHER PUNTEN||" + nfcTwo.nfcCode + "\n");
        Serial.println(nfcTwo.punten + "\n");
        Serial.println("GNAAM||" + nfcTwo.naam + "\n");
        Serial.println("GPUNTEN||" + String(nfcTwo.punten) + "\n");
        Serial.println("NFCCODE||" + nfcTwo.nfcCode + "\n");
      }
    } else if (receivedData.message.equals("KOPPELSUCCESVOL")) {
      if (messageCode == nfcOne.koppelCode) {
        nfcOne.gekoppeld = true;
        nfcOne.assignedAgain = true;
        Serial.print("Gekoppelde medaillon: ");
        Serial.println(nfcOne.koppelCode);
      } else if (messageCode == nfcTwo.koppelCode) {
        nfcTwo.gekoppeld = true;
        nfcTwo.assignedAgain = true;
        Serial.print("Gekoppelde medaillon: ");
        Serial.println(nfcTwo.koppelCode);
      }
    } else if (receivedData.message.equals("ONTKOPPEL")) {
      if (receivedData.value.equals(nfcOne.koppelCode)) {
        nfcOne.gekoppeld = false;
        Serial.print("Ontkoppelde medaillon: ");
        Serial.println(nfcOne.koppelCode);
      } else if (receivedData.value.equals(nfcTwo.koppelCode)) {
        nfcTwo.gekoppeld = false;
        Serial.print("Ontkoppelde medaillon: ");
        Serial.println(nfcTwo.koppelCode);
      }
    } else if (receivedData.message.equals("RESET")) {
      if (receivedData.value.equals(nfcOne.koppelCode)) {
        nfcOne = {"09cb77e2", "", "54312", "54312", false, false, false, 0, 0};
        Serial.println("NFC RESET MEDAILLON 54312");
      } else if (receivedData.value.equals(nfcTwo.koppelCode)) {
        nfcTwo = {"f9f927c3", "", "98275", "98275", false, false, false, 0, 0};
        Serial.println("NFC RESET MEDAILLON 98275");
      }
    } else if (receivedData.message.equals("QUESTREADY")) {
      questReady = true;
    } else if (receivedData.message.equals("QUESTCOMPLETE") && questStarted) {
      if (nfcOne.nfcCode == questStartedNfc) {
        nfcOne.punten += 1000;
        nfcOne.questCompleted = 1;
        questStarted = false;
        questStartedNfc = "";
        questStartedNaam = "";
        questReady = false;
        Serial.println("GPUNTEN||" + String(nfcOne.punten) + "\n");
        Serial.println("NFCCODE||" + nfcOne.nfcCode + "\n");
      } else if (nfcTwo.nfcCode == questStartedNfc) {
        nfcTwo.punten += 1000;
        nfcTwo.questCompleted = 1;
        questStarted = false;
        questStartedNfc = "";
        questReady = false;
        Serial.println("GPUNTEN||" + String(nfcTwo.punten) + "\n");
        Serial.println("NFCCODE||" + nfcTwo.nfcCode + "\n");
      }
    }
  }

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  String uidString = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uidString += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uidString += String(mfrc522.uid.uidByte[i], HEX);
  }

  auto processNfc = [&](NfcData &nfcData) {
    if (uidString.equals(nfcData.uid)) {
      if (!nfcData.assigned && naam.length() > 0) {
        nfcData.naam = naam;
        Serial.println(nfcData.naam);
        naam = "";
        nfcData.assigned = true;
      }

      Serial.println("GNAAM||" + nfcData.naam + "\n");
      Serial.println("GPUNTEN||" + String(nfcData.punten) + "\n");
      Serial.println("NFCCODE||" + nfcData.nfcCode + "\n");
      if (!nfcData.gekoppeld) {
        Serial.println("KOPPELCODE||" + String(nfcData.koppelCode) + "\n");
      }

      if (puntenOptellen != 0) {
        nfcData.punten += puntenOptellen;
        puntenOptellen = 0;
        Serial.println("GPUNTEN||" + String(nfcData.punten) + "\n");
        Serial.println("NFCCODE||" + nfcData.nfcCode + "\n");
      }

      if (!questStarted && nfcData.assignedAgain == true && questReady && nfcData.questCompleted != 1) {
        questStarted = true;
        questStartedNfc = nfcData.nfcCode;
        questStartedNaam = nfcData.naam;
        Serial.println("STARTQUEST||" + questStartedNfc + "\n");
        Serial.println("STARTQUESTNAAM||" + questStartedNaam + "\n");
      }
    }
  };

  processNfc(nfcOne);
  processNfc(nfcTwo);

  mfrc522.PICC_HaltA();
}
