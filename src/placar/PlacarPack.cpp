#include "PlacarPack.h"

const uint8_t PlacarPack::cnum[] = {0xb0, 0x31, 0x32, 0xb3, 0x34, 0xb5, 0xb6, 0x37, 0x38, 0xb9};

PlacarPack::PlacarPack()
    : gols_(0)
{
    // inicializa pack_ como estava na main (sem alterar)
    placar_info_t init = {
        0x02, 0x92, 0xBF, 0xB0, 0xB0, 0x31, 0xBF, 0xB0, 0xB0, 0xBF,
        0xB0, 0xB0, 0xB0, 0xB0, 0xB0, 0xBF, 0xB0, 0xB0, 0xB0, 0x32,
        0x34, 0x32, 0xB0, 0x83, 0x26, 0x02, 0x21, 0x23};
    pack_ = init;
}

void PlacarPack::incrementarGols()
{
    gols_ = (gols_ + 1) % 200;
    atualizarBytesGols();
}

void PlacarPack::decrementarGols()
{
    if (gols_ > 0)
    {
        gols_--;
    }
    else
    {
        gols_ = 0; // já está em zero, não deixa negativo
    }
    atualizarBytesGols();
}

void PlacarPack::atualizarBytesGols()
{
    uint8_t c = gols_ / 100;
    pack_.equipeA[0] = (c == 0) ? 0xBF : cnum[c];
    pack_.equipeA[1] = cnum[(gols_ % 100) / 10];
    pack_.equipeA[2] = cnum[gols_ % 10];
}

// SET/FALTAS A (2 dígitos), tens=0 use 0xBF
void PlacarPack::incrementarSetFaltasA()
{
    // Limita a 20 e reinicia em 0 após atingir o máximo
    setA_ = (setA_ + 1) % 21;

    int tens = setA_ / 10;
    int ones = setA_ % 10;

    // Atualiza conforme o protocolo do Python
    pack_.setfaltas[0] = (tens == 0 ? 0xBF : cnum[tens]); // 16º byte (dezenas)
    pack_.setfaltas[1] = cnum[ones];                      // 17º byte (unidades)
}

// P. TEMP A (1 dígito), use cnum e 0xB0 para zero
void PlacarPack::incrementarPedidoTempoA()
{
    static int tempoA = 0;
    tempoA = (tempoA + 1) % 3;
    pack_.tempoaA[0] = cnum[tempoA]; // cnum[0]==0xB0, cnum[1]==0x31…
}

// SERVIÇO A toggle: “sem”=0xB0, “A”=0x31
void PlacarPack::toggleServicoA()
{
    pack_.servico[0] = (pack_.servico[0] == cnum[1] // cnum[1]==0x31
                            ? cnum[0]               // voltar para “sem” 0xB0
                            : cnum[1]);             // colocar “A” 0x31
}

// EQUIPE B

// SET/FALTAS B (2 dígitos)
void PlacarPack::incrementarSetFaltasB()
{
    // Limita a 20 e reinicia em 0 após atingir o máximo
    setB_ = (setB_ + 1) % 21;

    int tens = setB_ / 10;
    int ones = setB_ % 10;

    // Atualiza conforme o protocolo do Python
    pack_.setfaltasb[0] = (tens == 0 ? 0xBF : cnum[tens]); // 16º byte (dezenas)
    pack_.setfaltasb[1] = (ones == 0) ? 0xB0 : cnum[ones]; // 17º byte (unidades)
}

// SERVIÇO B toggle: “sem”=cnum[0], “B”=cnum[2]
void PlacarPack::toggleServicoB()
{
    pack_.servico[0] = (pack_.servico[0] == cnum[2] ? cnum[0] : cnum[2]);
}

// P. TEMP B (1 dígito)
void PlacarPack::incrementarPedidoTempoB()
{
    static int tempoB = 0;
    tempoB = (tempoB + 1) % 3;
    pack_.tempoaB[0] = cnum[tempoB];
}

// Gols B +1/–1 (mesma lógica de A, mas em equipeB)
void PlacarPack::incrementarGolsB()
{
    golsB_ = (golsB_ + 1) % 200;
    // atualize bytes de equipeB igual ao A:
    uint8_t c = golsB_ / 100;
    pack_.equipeB[0] = (c == 0 ? 0xBF : cnum[c]);
    pack_.equipeB[1] = cnum[(golsB_ % 100) / 10];
    pack_.equipeB[2] = cnum[golsB_ % 10];
}
void PlacarPack::decrementarGolsB()
{
    if (golsB_ > 0)
        --golsB_;
    uint8_t c = golsB_ / 100;
    pack_.equipeB[0] = (c == 0 ? 0xBF : cnum[c]);
    pack_.equipeB[1] = cnum[(golsB_ % 100) / 10];
    pack_.equipeB[2] = cnum[golsB_ % 10];
}

// cronômetro

// Ativa/desativa o cronômetro
void PlacarPack::toggleCronometro()
{
    cronometroRunning_ = !cronometroRunning_;
    if (cronometroRunning_)
        lastCronoMillis_ = millis();
}

// Deve ser chamado todo loop para avançar o timer
void PlacarPack::updateCronometro()
{
    if (!cronometroRunning_)
        return;
    unsigned long now = millis();
    unsigned long delta = now - lastCronoMillis_;
    if (delta < 1000)
        return;
    unsigned int secs = delta / 1000;
    lastCronoMillis_ += secs * 1000;

    if (countdownMode_)
    {
        if (cronoSeconds_ == 0)
        {
            cronometroRunning_ = false;
            countdownMode_ = false;
            return;
        }
        if (secs > cronoSeconds_)
            secs = cronoSeconds_;
        cronoSeconds_ -= secs; // conta-para-trás
    }
    else
    {
        cronoSeconds_ += secs; // conta-para-frente
    }

    unsigned int m = cronoSeconds_ / 60;
    unsigned int s = cronoSeconds_ % 60;
    pack_.cronometro[0] = cnum[m / 10];
    pack_.cronometro[1] = cnum[m % 10];
    pack_.cronometro[2] = cnum[s / 10];
    pack_.cronometro[3] = cnum[s % 10];
}

// zerar placar

void PlacarPack::zerar()
{
    // Zera gols e cronômetro
    gols_ = 0;
    golsB_ = 0;
    cronoSeconds_ = 0;
    cronometroRunning_ = false;
    countdownMode_ = false;

    setA_ = 0; // Zera set/faltas A
    setB_ = 0; // Zera set/faltas B

    // Zera set/faltas (equipe A e B)
    pack_.setfaltas[0] = 0xBF;    // Dezena (0xBF = vazio)
    pack_.setfaltas[1] = cnum[0]; // Unidade (0xB0 = zero)
    pack_.setfaltasb[0] = 0xBF;
    pack_.setfaltasb[1] = cnum[0];

    // Zera pedidos de tempo (equipe A e B)
    pack_.tempoaA[0] = cnum[0]; // 0xB0
    pack_.tempoaB[0] = cnum[0];

    // Zera serviço (volta para "sem")
    pack_.servico[0] = cnum[0]; // 0xB0

    // Zera placar principal (equipe A e B)
    pack_.equipeA[0] = 0xBF;
    pack_.equipeA[1] = cnum[0];
    pack_.equipeA[2] = cnum[0];
    pack_.equipeB[0] = 0xBF;
    pack_.equipeB[1] = cnum[0];
    pack_.equipeB[2] = cnum[0];

    // Zera cronômetro (00:00)
    pack_.cronometro[0] = cnum[0];
    pack_.cronometro[1] = cnum[0];
    pack_.cronometro[2] = cnum[0];
    pack_.cronometro[3] = cnum[0];

    // zerar alarme
    alarmeLigado_ = false;
    pack_.alarme[0] = 0xBA; // 0xBA = alarme desligado

    // Zera período (1)
    periodoAtual_ = 1;
    pack_.periodo[0] = cnum[1]; // 0x31 = 1
}

// alarme
void PlacarPack::alarme()
{
    alarmeLigado_ = !alarmeLigado_;
    pack_.alarme[0] = alarmeLigado_ ? 0xB3 : 0xBA;
}

// periodo

void PlacarPack::avancarPeriodo()
{
    if (penaltis_)
    {
        // Se já está em pênaltis, volta para o 1º período
        periodoAtual_ = 1;
        tempoExtra_ = false;
        penaltis_ = false;
        pack_.periodo[0] = cnum[1];
    }
    else if (tempoExtra_)
    {
        // Se estava em tempo extra, vai para pênaltis
        penaltis_ = true;
        pack_.periodo[0] = 0xD0; // Código para "PENALTIS"
    }
    else if (periodoAtual_ >= 5)
    {
        // Depois do 5º período, vai para tempo extra
        tempoExtra_ = true;
        pack_.periodo[0] = 0x45; // Código para "TEMPO EXTRA"
    }
    else
    {
        // Avança período normal (1-5)
        periodoAtual_++;
        pack_.periodo[0] = cnum[periodoAtual_]; // Usa o array cnum
    }
}

void PlacarPack::setCronometroPreset(uint8_t minutos)
{
    cronoSeconds_ = minutos * 60; // carrega MM:00
    countdownMode_ = true;        // modo regressivo
    cronometroRunning_ = false;   // só começa quando receber 0x0B

    pack_.cronometro[0] = cnum[minutos / 10];
    pack_.cronometro[1] = cnum[minutos % 10];
    pack_.cronometro[2] = cnum[0];
    pack_.cronometro[3] = cnum[0];
}

void PlacarPack::calcularCRC()
{
    pack_.crc[0] = calcularCRC8(reinterpret_cast<uint8_t *>(&pack_), sizeof(placar_info_t) - 4);
}

uint8_t PlacarPack::calcularCRC8(uint8_t *data, size_t length, uint8_t poly, uint8_t init)
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
