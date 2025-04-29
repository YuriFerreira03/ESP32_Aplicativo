#include <Arduino.h>
#include "placar/PlacarPack.h"
#include "ble/BLEManager.h"

PlacarPack pack;
BLEManager ble(pack);

void setup()
{
  Serial.begin(9600);
  ble.begin();
}

void loop()
{
  // recalcula CRC e envia serial
  pack.updateCronometro();
  pack.calcularCRC();
  Serial.write(pack.data(), pack.size());

  // notifica BLE
  ble.notifyIfConnected();

  delay(3);
}