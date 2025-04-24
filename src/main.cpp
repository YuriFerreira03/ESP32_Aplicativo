#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>

// Representa o que cada campo do placar gasta de byte em um array
typedef struct
{
  uint8_t inicio[2];
  uint8_t equipeA[3];
  uint8_t periodo[1];
  uint8_t equipeB[3];
  uint8_t setfaltas[2];
  uint8_t cronometro[4];
  uint8_t setfaltasb[2];
  uint8_t tempoaA[1];
  uint8_t tempoaB[1];
  uint8_t prog[2];
  uint8_t alarme[1];
  uint8_t servico[1];
  uint8_t reservado[1];
  uint8_t crc[1];
  uint8_t fim[3];
} placar_info_t;

// Array de números de acordo com o placar
const uint8_t cnum[] = {0xb0, 0x31, 0x32, 0xb3, 0x34, 0xb5, 0xb6, 0x37, 0x38, 0xb9};

// BLE UUIDs
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer *pServer;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

int gols = 0; // Contador de gols da equipe A
unsigned long lastUpdate = 0;
const unsigned long interval = 3000; // 3 segundos para incrementar gols
placar_info_t pack = {0x02, 0x92, 0xBF, 0xB0, 0xB0, 0x31, 0xBF, 0xB0, 0xB0, 0xBF, 0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xBF, 0xB0, 0xB0, 0xB0, 0x32, 0x34, 0x32, 0xB0, 0x83, 0x26, 0x02, 0x21, 0x23};

uint8_t calcularCRC8(uint8_t *data, size_t length, uint8_t poly = 0x01, uint8_t init = 0x80)
{
  uint8_t crc = init;
  for (size_t i = 0; i < length; i++)
  {
    crc ^= data[i];
    for (int j = 0; j < 8; j++)
    {
      if (crc & 0x80)
        crc = (crc << 1) ^ poly;
      else
        crc <<= 1;
    }
  }
  return crc & 0xFF;
}

void atualizarGols()
{
  uint8_t c = gols / 100;
  pack.equipeA[0] = (c == 0) ? 0xbf : cnum[c];
  pack.equipeA[1] = cnum[(gols % 100) / 10];
  pack.equipeA[2] = cnum[gols % 10];
  gols = (gols + 1) % 200;
}

class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *srv)
  {
    deviceConnected = true;
    // Serial.println("BLE conectado");
    // Ao conectar, zera a pontuacao
    gols = 0;
    atualizarGols();
  }
  void onDisconnect(BLEServer *srv)
  {
    deviceConnected = false;
    // Serial.println("BLE desconectado, reiniciando advertising...");
    srv->startAdvertising();
  }
};

class MyCharCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *chr) override
  {
    std::string v = chr->getValue();
    if (v.length() > 0)
    {
      uint8_t cmd = (uint8_t)v[0];
      if (cmd == 0x01)
      {
        atualizarGols();
        // Serial.println("Gols incrementados");
      }
      else
      {
        // Serial.print("Comando desconhecido recebido: ");
        // Serial.println(cmd, HEX);
      }
    }
  }
};

void setup()
{
  Serial.begin(9600);
  // Setup BLE
  BLEDevice::init("ESP32_BLE_Placar");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->setCallbacks(new MyCharCallbacks());
  pService->start();
  BLEAdvertising *pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->setScanResponse(true);
  pAdv->setMinPreferred(0x06);
  pAdv->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  // Serial.println("BLE pronto, aguardando conexoes...");
}

void loop()
{
  // recalcula CRC e envia serial sempre
  pack.crc[0] = calcularCRC8((uint8_t *)&pack, sizeof(placar_info_t) - 4);
  Serial.write((uint8_t *)&pack, sizeof(placar_info_t));

  // notificacao BLE se conectado
  if (deviceConnected)
  {
    pCharacteristic->setValue((uint8_t *)&pack, sizeof(placar_info_t));
    pCharacteristic->notify();
  }

  // incrementa gols periodicamente
  // if (millis() - lastUpdate >= interval)
  // {
  //   lastUpdate = millis();
  //   atualizarGols();
  // }

  delay(3);
}