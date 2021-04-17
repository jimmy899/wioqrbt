#include <rpcBLEDevice.h>
#include <BLEServer.h>

#include <TFT_eSPI.h>
#include <qrcode.h>
 
#define SERVICE_UUID        "180f"
#define CHARACTERISTIC_UUID "2a19"
#define DESCRIPTOR_UUID     "4545"

TFT_eSPI tft;

static int pixel_size = 3;
static int lcd_width = 320;
static int lcd_height = 240;

const int capacity_for_medium[] = {14, 26,42, 62, 84, 106, 122, 152, 180, 213, 251, 287, 331, 362, 412, 450, 504, 560, 624, 666, 771, 779, 857, 911, 997};

uint8_t qrcodeData[1024];
int qrcodeDatalen = 0;
int printed = 0;


int qrcodeDraw(uint8_t *data)
{
  int qrv = 1;
  int x;
  int y;
  QRCode qrcode;
  int i;
  int datalen = strlen((char *) data);

  for (i = 0; i < sizeof(capacity_for_medium) / sizeof(capacity_for_medium[0]); i++) {
    if (capacity_for_medium[i] > datalen) {
      qrv = i + 1;
      break;
    }
  }

  if (i == sizeof(capacity_for_medium) / sizeof(capacity_for_medium[0])) {
    return -1;
  }
  
  uint8_t qrcodeBytes[qrcode_getBufferSize(qrv)];

  if (qrcode_initBytes(&qrcode, qrcodeBytes, qrv, ECC_MEDIUM, data, datalen) < 0) {
    return -1;
  }

  int offset_x = (lcd_width - qrcode.size * pixel_size) / 2;
  int offset_y = (lcd_height - qrcode.size * pixel_size) / 2;

  if (qrcode.size > 80) {
    return -1;
  }

  tft.fillRect(offset_x - pixel_size, offset_y - pixel_size, (qrcode.size + 2) * pixel_size, (qrcode.size + 2) * pixel_size, TFT_WHITE);

  for (y = 0; y < qrcode.size; y++) {
    for (x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        // tft.drawPixel(x, y, TFT_BLACK);
        tft.fillRect(offset_x + x * pixel_size, offset_y + y * pixel_size, pixel_size, pixel_size, TFT_BLACK);
      } else {
        // tft.drawPixel(x, y, TFT_WHITE);
        tft.fillRect(offset_x + x * pixel_size, offset_y + y * pixel_size, pixel_size, pixel_size, TFT_WHITE);
      }
    }
  }
  return 0;
  
}

void qrcodeInit() {
 
  tft.begin();
  tft.setRotation(3);

  tft.fillScreen(TFT_WHITE);
}
 
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
 
      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);
 
        Serial.println();
        Serial.println("*********");
        strcpy((char *) qrcodeData, rxValue.c_str());
        qrcodeDatalen = rxValue.size();
        qrcodeDraw(qrcodeData);
      }
    }

    void onRead(BLECharacteristic *pCharacteristic) {
      pCharacteristic->setValue(qrcodeData, qrcodeDatalen);
      Serial.println("*********");
      Serial.println((char *)qrcodeData);
      Serial.println("*********");
    }
};
 
void setup() {
  pinMode(WIO_5S_PRESS, INPUT);
  Serial.begin(115200);
  // while(!Serial){};
  Serial.println("Starting BLE work!");

  qrcodeInit();
  
  BLEDevice::init("WIO Terminal");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );
  pCharacteristic->setAccessPermissions(GATT_PERM_READ | GATT_PERM_WRITE);
  BLEDescriptor *pDescriptor = pCharacteristic->createDescriptor(
                                         DESCRIPTOR_UUID,
                                          ATTRIB_FLAG_VOID | ATTRIB_FLAG_ASCII_Z,
                                         GATT_PERM_READ | GATT_PERM_WRITE,
                                         2
                                         );
  pCharacteristic->setValue("Hello World says Neil");
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();
 
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

#if 1

void loop() {

  int c = Serial.read();

  if (digitalRead(WIO_5S_PRESS) == LOW) {
    tft.fillScreen(TFT_WHITE);
    if (qrcodeDraw(qrcodeData)) {
      Serial.println("qrcode failed");
    }
  }

  if (c == 10) {
    Serial.println("");
    qrcodeData[qrcodeDatalen] = '\0';    
    if (qrcodeDraw(qrcodeData)) {
      Serial.println("qrcode failed");
    }
    qrcodeDatalen = 0;
    printed = 0;
    return;
  }
  if (c == -1) {
    return;
  }
  if (qrcodeDatalen == sizeof(qrcodeData) - 1) {
    if (!printed) {
      Serial.println("\nbuffer is full");
      printed = 1;
    }
    return;
  }
  
  qrcodeData[qrcodeDatalen++] = c;
  Serial.print(c);
  Serial.print(",");
}
#else
void loop() { 
  // put your main code here, to run repeatedly:
  Serial.println("1");
  delay(2000);
}
#endif
