/*
 * power_control.h
 *
 *  Created on: Sep 1, 2026
 *      Author: josef
 */

#ifndef INC_POWER_CONTROL_H_
#define INC_POWER_CONTROL_H_

#include <stdint.h>

// Definice pracovních režimů
typedef enum {
    MODE_DAC_DIRECT = 0, // Výstup 0-3.3V přímo z F303 (bez zpětné vazby)
    MODE_POWER_LM317 = 1 // Výkonový výstup přes LM317 (PI regulace zapnuta)
} ChannelMode_t;

// 1. Kalibrační hodnoty (Zadáte pouze to, co fyzicky změříte)
typedef struct {
    uint32_t adc_v_ref_mv;       // Naměřené napětí 3.3V větve (např. 3295)
    uint32_t r_shunt_mohm;       // Odpor bočníku (např. 1000 mOhm)

    // Rezistory děliče pro měření Výstupního napětí (za bočníkem)
    uint32_t r_v_top_ohm;        // Horní rezistor (např. 50120 Ohm)
    uint32_t r_v_bot_ohm;        // Dolní rezistor (např. 9980 Ohm)

    // Rezistory děliče pro měření Napětí před bočníkem (pro výpočet proudu)
    uint32_t r_a_top_ohm;        // Horní rezistor (např. 50050 Ohm)
    uint32_t r_a_bot_ohm;        // Dolní rezistor (např. 10010 Ohm)

    // --- Interní předpočítané hodnoty (nevyplňujete) ---
    float _v_multiplier;         // Násobič pro finální napětí
    float _a_multiplier;         // Násobič pro napětí před bočníkem
} ChannelCalibration_t;

// 2. Data pro PI Regulátor
typedef struct {
    int32_t Kp;                  // Proporcionální konstanta
    int32_t Ki;                  // Integrační konstanta
    int32_t integral_sum;        // Akumulátor integrační složky
    int32_t integral_max;        // Anti-windup strop (zabrání obrovským přetečením)
    int32_t integral_min;        // Anti-windup dno
} PI_Controller_t;

// 3. Hlavní objekt kanálu zdroje
typedef struct {
    // Uživatelské nastavení
    uint8_t enabled;             // 0 = Vypnuto (relé rozepnuto), 1 = Zapnuto
    ChannelMode_t mode;          // Aktuální režim z enumu výše
    int32_t target_voltage_mv;   // Požadavek z enkodéru (0 - 15000 mV)

    // Fyzikální realita z ADC
    int32_t actual_voltage_mv;   // Reálně naměřené napětí na svorce
    int32_t actual_current_ma;   // Reálně naměřený proud tekoucí přes LM317

    // Limity SOA (Safe Operating Area z grafu)
    int32_t current_soa_limit_mv; // Maximální napětí, které trafo v danou chvíli zvládne
    uint8_t is_limited;           // Příznak pro displej (1 = narazili jsme na strop trafa)

    // HW výstupy
    uint32_t current_dac_val;    // Surová hodnota právě odesílaná do DAC (0-4095)

    // Vyhlazené (zprůměrované) hodnoty výhradně pro displej
		int32_t ui_voltage_mv;
		int32_t ui_current_ma;

    // Objekty uvnitř kanálu
    ChannelCalibration_t calib;
    PI_Controller_t pi;
} PowerChannel_t;

// Funkce inicializace a konfigurace
void PowerChannel_Init(PowerChannel_t *ch, uint32_t v_ref, uint32_t r_shunt,
                       uint32_t rv_top, uint32_t rv_bot,
                       uint32_t ra_top, uint32_t ra_bot);
void PowerSystem_Init(void);

// Funkce regulační smyčky a čtení
void PowerControl_ProcessADC(PowerChannel_t *ch, uint16_t adc_raw_v, uint16_t adc_raw_a);
void PowerControl_UpdateSOALimit(PowerChannel_t *ch);
void PowerControl_ProcessControlLoop(PowerChannel_t *ch);

// Globální instance pro CH1 a CH2, ke kterým budeme přistupovat z mainu
extern PowerChannel_t ch1;
extern PowerChannel_t ch2;

#endif /* INC_POWER_CONTROL_H_ */
