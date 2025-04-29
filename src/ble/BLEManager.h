#pragma once
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLEAdvertising.h>
#include "placar/PlacarPack.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class BLEManager
{
public:
    BLEManager(PlacarPack &packRef);
    void begin();
    void notifyIfConnected();
    bool isConnected() const;

private:
    BLEServer *pServer_;
    BLECharacteristic *pCharacteristic_;
    PlacarPack &pack_;
    bool deviceConnected_;

    class ServerCallbacks : public BLEServerCallbacks
    {
        BLEManager &parent_;

    public:
        ServerCallbacks(BLEManager &p) : parent_(p) {}
        void onConnect(BLEServer *srv) override;
        void onDisconnect(BLEServer *srv) override;
    };

    class CharCallbacks : public BLECharacteristicCallbacks
    {
        PlacarPack &pack_;

    public:
        CharCallbacks(PlacarPack &p) : pack_(p) {}
        void onWrite(BLECharacteristic *chr) override;
    };
};
