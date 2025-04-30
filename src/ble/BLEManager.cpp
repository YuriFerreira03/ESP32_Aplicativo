#include "BLEManager.h"
#include <Arduino.h>

BLEManager::BLEManager(PlacarPack &packRef)
    : pack_(packRef), deviceConnected_(false) {}

void BLEManager::begin()
{
    BLEDevice::init("ESP32_BLE_Placar");
    pServer_ = BLEDevice::createServer();
    pServer_->setCallbacks(new ServerCallbacks(*this));
    BLEService *pService = pServer_->createService(SERVICE_UUID);
    pCharacteristic_ = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);
    pCharacteristic_->setCallbacks(new CharCallbacks(pack_));
    pService->start();

    BLEAdvertising *pAdv = BLEDevice::getAdvertising();
    pAdv->addServiceUUID(SERVICE_UUID);
    pAdv->setScanResponse(true);
    pAdv->setMinPreferred(0x06);
    pAdv->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
}

// Notifica pacote via BLE se conectado
void BLEManager::notifyIfConnected()
{
    if (deviceConnected_)
    {
        pCharacteristic_->setValue(pack_.data(), pack_.size());
        pCharacteristic_->notify();
    }
}

bool BLEManager::isConnected() const
{
    return deviceConnected_;
}

void BLEManager::ServerCallbacks::onConnect(BLEServer *srv)
{
    parent_.deviceConnected_ = true;
    parent_.pack_.incrementarGols(); // zera/inicializa se quiser
}
void BLEManager::ServerCallbacks::onDisconnect(BLEServer *srv)
{
    parent_.deviceConnected_ = false;
    srv->startAdvertising();
}

void BLEManager::CharCallbacks::onWrite(BLECharacteristic *chr)
{
    std::string v = chr->getValue();
    if (v.length() == 0)
        return;

    uint8_t cmd = (uint8_t)v[0];
    switch (cmd)
    {
    case 0x01:
        pack_.incrementarGols();
        break;
    case 0x02:
        pack_.decrementarGols();
        break;
    case 0x04:
        pack_.incrementarPedidoTempoA();
        break;
    case 0x05:
        pack_.toggleServicoA();
        break;
    case 0x06:
        pack_.incrementarSetFaltasA();
        break;
    case 0x03:
        pack_.incrementarGolsB();
        break;
    case 0x0a:
        pack_.decrementarGolsB();
        break;
    case 0x07:
        pack_.incrementarSetFaltasB();
        break;
    case 0x08:
        pack_.toggleServicoB();
        break;
    case 0x09:
        pack_.incrementarPedidoTempoB();
        break;
    case 0x0b:
        pack_.toggleCronometro();
        break;
    case 0x0d:
        pack_.zerar();
        break;
    default: /* comando desconhecido */
        break;
    }
}
