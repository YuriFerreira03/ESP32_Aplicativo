#pragma once
#include <Arduino.h>

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

class PlacarPack
{
public:
    PlacarPack();
    void incrementarGols();
    void decrementarGols();
    void incrementarSetFaltasA();
    void toggleServicoA();
    void incrementarPedidoTempoA();
    // equipe B
    void incrementarSetFaltasB();
    void toggleServicoB();
    void incrementarPedidoTempoB();
    void incrementarGolsB();
    void decrementarGolsB();
    void calcularCRC();
    // cronômetro
    void toggleCronometro();
    void setCronometroPreset(uint8_t minutos);
    void updateCronometro();
    // zerar
    void zerar();
    uint8_t *data() { return reinterpret_cast<uint8_t *>(&pack_); }
    size_t size() const { return sizeof(pack_); }
    static const uint8_t cnum[];
    // alarme
    void alarme();
    void avancarPeriodo();

private:
    placar_info_t pack_;
    int gols_;
    int setA_ = 0;
    int setB_ = 0;
    int golsB_;
    bool cronometroRunning_{false};
    unsigned long lastCronoMillis_{0};
    unsigned int cronoSeconds_{0};
    uint8_t calcularCRC8(uint8_t *data, size_t length, uint8_t poly = 0x01, uint8_t init = 0x80);
    void atualizarBytesGols();
    bool alarmeLigado_{false};
    int periodoAtual_ = 1; // 1-5 ou códigos especiais
    bool tempoExtra_ = false;
    bool penaltis_ = false;
    bool countdownMode_{false};
};