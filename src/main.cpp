#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include <Base64.h>

// --- Configuração do serviço e característica BLE ---
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer *pServer;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;

// --- Estrutura de dados do placar ---
typedef struct
{
  uint8_t inicio[2];
  uint8_t equipeA[3]; // bytes 2,3,4
  uint8_t periodo[1];
  uint8_t equipeB[3];
  uint8_t setfaltas[2];
  uint8_t cronometro[4];
  uint8_t setfaltasb[2];
  uint8_t tempoaA[1]; // byte X (você define qual índice)
  uint8_t tempoaB[1];
  uint8_t prog[2];
  uint8_t alarme[1];
  uint8_t servico[1];
  uint8_t reservado[1];
  uint8_t crc[1]; // penúltimo byte
  uint8_t fim[3];
} placar_info_t;

// Tabela de conversão de dígitos para códigos do display
const uint8_t cnum[] = {
    0xb0, 0x31, 0x32, 0xb3, 0x34, 0xb5, 0xb6, 0x37, 0x38, 0xb9};

// Pacote inicial (preencha com seus valores iniciais)
placar_info_t pack = {
    {0x02, 0x92},             // inicio
    {0xbf, 0xb0, 0xb0},       // equipeA
    {0x45},                   // periodo
    {0x31, 0xb9, 0xb9},       // equipeB
    {0xbf, 0x32},             // setfaltas A
    {0xb0, 0xb0, 0xb0, 0xb0}, // cronometro
    {0xbf, 0x34},             // setfaltas B
    {0xb0},                   // tempo pedido A
    {0x32},                   // tempo pedido B
    {0x34, 0x32},             // prog
    {0xb0},                   // alarme
    {0x00},                   // servico
    {0x00},                   // reservado
    {0x00},                   // crc (vai ser calculado)
    {0xd9, 0x02, 0x21}        // fim
};

int gols = 0; // Contador de gols equipe A

// --- Função para calcular CRC8 ---
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

// --- Função para atualizar gols no pack.equipeA ---
void atualizarGols()
{
  uint8_t c = gols / 100; // centena
  pack.equipeA[0] = (c == 0) ? 0xbf : cnum[c];
  pack.equipeA[1] = cnum[(gols % 100) / 10];
  pack.equipeA[2] = cnum[gols % 10];
  gols++;
  if (gols > 199)
    gols = 0;
}

// --- Função auxiliar para imprimir o pacote completo em formato hex ---
void printPack(const char *msg)
{
  Serial.println(msg);
  uint8_t *p = (uint8_t *)&pack;
  for (size_t i = 0; i < sizeof(placar_info_t); i++)
  {
    if (i % 8 == 0)
      Serial.println(); // quebra de linha a cada 8 bytes
    Serial.printf("0x%02X ", p[i]);
  }
  Serial.println("\n------------------------------------");
}

// --- Callbacks do servidor BLE ---
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer) override
  {
    deviceConnected = true;
    Serial.println("BLE conectado");
  }
  void onDisconnect(BLEServer *pServer) override
  {
    deviceConnected = false;
    Serial.println("BLE desconectado, reiniciando advertising...");
    pServer->startAdvertising();
  }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pChar) override
  {
    std::string v = pChar->getValue();
    if (v.length() < 1)
      return;

    uint8_t cmd;
    // Se o comando for sempre "AQ=="
    if (v == "AQ==")
    {
      cmd = 0x01; // converte explicitamente "AQ==" para 0x01
    }
    else
    {
      Serial.printf("Comando desconhecido: %s\n", v.c_str());
      return;
    }

    Serial.printf("Comando convertido: 0x%02X\n", cmd);

    // Imprime o pacote antes de modificar (para debug)
    // printPack("Pacote antes de atualizar o comando:");

    // Processa o comando conforme a lógica do placar
    switch (cmd)
    {
    case 0x01: // +1 ponto A
      atualizarGols();
      break;
    // Adicione outros cases conforme necessário
    default:
      Serial.println("Comando não reconhecido");
      return;
    }

    // Calcula o CRC. Considere os primeiros 24 bytes, ou seja, até antes do campo 'crc'
    pack.crc[0] = calcularCRC8((uint8_t *)&pack, sizeof(placar_info_t) - 4);
    Serial.printf("CRC calculado: 0x%02X\n", pack.crc[0]);

    // Imprime o pacote após o cálculo do CRC
    printPack("Pacote após atualizar e recalcular o CRC:");

    // Envia o pacote atualizado via notificação BLE
    pCharacteristic->setValue((uint8_t *)&pack, sizeof(placar_info_t)); // aqui ele envia ao placar
    pCharacteristic->notify();
    Serial.println("Pacote notificado via BLE");
  }
};

void setup()
{
  Serial.begin(115200);
  Serial.println("Inicializando BLE + placar...");

  // Inicializa BLE
  BLEDevice::init("ESP32_BLE_Placar");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Cria serviço e característica (com READ, WRITE e NOTIFY)
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->setCallbacks(new MyCallbacks());

  // Calcula e define o valor inicial do pacote (calculando o CRC)
  pack.crc[0] = calcularCRC8((uint8_t *)&pack, sizeof(placar_info_t) - 4);
  pCharacteristic->setValue((uint8_t *)&pack, sizeof(placar_info_t));

  pService->start();

  // Configura o Advertising
  BLEAdvertising *pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->setScanResponse(true);
  pAdv->setMinPreferred(0x06);
  pAdv->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE pronto, aguardando conexões...");
}

void loop()
{
  // Ações são executadas no callback onWrite
  delay(1000);
}