/*
 * power_control.c
 *
 *  Created on: Sep 1, 2026
 *      Author: josef
 */

#include "power_control.h"

// Globální instance našich kanálů
PowerChannel_t ch1;
PowerChannel_t ch2;

// 1. Univerzální inicializátor pro jakýkoliv kanál
void PowerChannel_Init(PowerChannel_t *ch, uint32_t v_ref, uint32_t r_shunt,
                       uint32_t rv_top, uint32_t rv_bot,
                       uint32_t ra_top, uint32_t ra_bot)
{
    // Výchozí stavy
    ch->enabled = 0;
    ch->mode = MODE_POWER_LM317;
    ch->target_voltage_mv = 0;
    ch->current_dac_val = 0;
    ch->actual_voltage_mv = 0;
    ch->actual_current_ma = 0;
    ch->ui_voltage_mv = 0;
		ch->ui_current_ma = 0;
    ch->current_soa_limit_mv = 16000; // Výchozí strop naprázdno
    ch->is_limited = 0;

    // Nastavení PI regulátoru (Výchozí konstanty, doladíme později)
    ch->pi.Kp = 10;
    ch->pi.Ki = 2;
    ch->pi.integral_sum = 0;
    ch->pi.integral_max = 4095000; // Bezpečnostní strop pro anti-windup
    ch->pi.integral_min = 0;

    // Uložení fyzikálních kalibračních hodnot
    ch->calib.adc_v_ref_mv = v_ref;
    ch->calib.r_shunt_mohm = r_shunt;
    ch->calib.r_v_top_ohm = rv_top;
    ch->calib.r_v_bot_ohm = rv_bot;
    ch->calib.r_a_top_ohm = ra_top;
    ch->calib.r_a_bot_ohm = ra_bot;

    // Jednorázový předvýpočet matematiky (aby procesor nemusel dělit ve smyčce)
    float v_ratio = (float)(rv_top + rv_bot) / rv_bot;
    ch->calib._v_multiplier = ((float)v_ref / 4095.0f) * v_ratio;

    float a_ratio = (float)(ra_top + ra_bot) / ra_bot;
    ch->calib._a_multiplier = ((float)v_ref / 4095.0f) * a_ratio;
}

// 2. Hlavní inicializace celého napájecího systému (Volá se z main.c před while(1))
void PowerSystem_Init(void)
{
    // Konstrukce CH1 (Zde zadáte přesně změřené rezistory pro Kanál 1)
    PowerChannel_Init(&ch1, 3295, 1160, 9968, 2000, 9968, 2000);

    // Konstrukce CH2 (Zde zadáte přesně změřené rezistory pro Kanál 2)
    PowerChannel_Init(&ch2, 3295, 1150, 10146, 2000, 10146, 2000);
}

// 3. Převod ADC do reálného světa (Volá se v regulační smyčce)
void PowerControl_ProcessADC(PowerChannel_t *ch, uint16_t adc_raw_v, uint16_t adc_raw_a)
{
    // Spočítáme skutečné napětí na výstupu v milivoltech
    ch->actual_voltage_mv = (int32_t)(adc_raw_v * ch->calib._v_multiplier);

    // Spočítáme skutečné napětí před bočníkem
    int32_t pre_shunt_mv = (int32_t)(adc_raw_a * ch->calib._a_multiplier);

    // Úbytek na bočníku (pokud je vlivem šumu ADC záporný, ořízneme na nulu)
    int32_t voltage_drop_mv = pre_shunt_mv - ch->actual_voltage_mv;
    if (voltage_drop_mv < 0) voltage_drop_mv = 0;

    // Ohmův zákon (I = U / R) -> jelikož je odpor v mOhm, násobíme 1000 pro mA
    ch->actual_current_ma = (voltage_drop_mv * 1000) / ch->calib.r_shunt_mohm;

    // === FILTR PRO DISPLEJ (Exponenciální klouzavý průměr) ===
		// Váha 31:1 (nová hodnota má vliv 1/32). Tím získáme krásně klidný a čitelný
		// text na displeji, ale regulační smyčka (actual_voltage_mv) zůstává blesková!
		ch->ui_voltage_mv = ((ch->ui_voltage_mv * 31) + ch->actual_voltage_mv) / 32;
		ch->ui_current_ma = ((ch->ui_current_ma * 31) + ch->actual_current_ma) / 32;
}

// 4. Výpočet dynamického limitu SOA transformátoru
void PowerControl_UpdateSOALimit(PowerChannel_t *ch)
{
    // Rovnice z grafu: V_limit = 16.0V - 6.5 * I_A
    // Převedeno na celočíselnou matematiku pro MCU (mV a mA):
    // 6.5 * I_mA je to samé jako (13 * I_mA) / 2

    int32_t limit_mv = 16000 - ((13 * ch->actual_current_ma) / 2);

    // Ochrana proti matematickým nesmyslům (nikdy nepůjdeme pod 0 V)
    if (limit_mv < 0) limit_mv = 0;
    if (limit_mv > 16000) limit_mv = 16000;

    ch->current_soa_limit_mv = limit_mv;
}

// 5. Samotný PI regulátor
void PowerControl_ProcessControlLoop(PowerChannel_t *ch)
{
    // A) Bezpečnostní stav - Výstup je vypnutý
    if (ch->enabled == 0) {
        ch->current_dac_val = 0;
        ch->pi.integral_sum = 0; // Vymazání paměti integrátoru
        ch->is_limited = 0;
        return;
    }

    // B) Režim "Čistý DAC" - Ignorujeme PI regulátor a LM317
    if (ch->mode == MODE_DAC_DIRECT) {
        // Přímý přepočet 0 - 3300 mV na 0 - 4095 pro DAC
        int32_t dac_raw = (ch->target_voltage_mv * 4095) / 3300;

        if (dac_raw > 4095) dac_raw = 4095;
        if (dac_raw < 0) dac_raw = 0;

        ch->current_dac_val = (uint32_t)dac_raw;
        ch->pi.integral_sum = 0; // Regulátor odpočívá, nesmí se natahovat
        ch->is_limited = 0;
        return;
    }

    // C) Plnohodnotný režim napájecího zdroje (MODE_POWER_LM317)

    // 1. Ochrana SOA: Zjistíme, co po zdroji reálně chceme
    int32_t safe_target_mv = ch->target_voltage_mv;
    ch->is_limited = 0;

    if (safe_target_mv > ch->current_soa_limit_mv) {
        safe_target_mv = ch->current_soa_limit_mv;
        ch->is_limited = 1; // Fyzikálně narážíme na limit trafa (Rozsvítí 'LIM' na displeji)
    }

    // 2. Výpočet regulační odchylky (Error)
    int32_t error_mv = safe_target_mv - ch->actual_voltage_mv;

    // 3. Proporcionální složka (P)
    int32_t p_term = ch->pi.Kp * error_mv;

    // 4. Integrační složka (I) s chytrou Anti-Windup ochranou
    // Zastavíme integraci, pokud je DAC na doraz (4095) a my stále chceme přidávat (error > 0),
    // nebo pokud je DAC na nule a my chceme ještě ubírat (error < 0).
    uint8_t hold_integral = 0;
    if ((ch->current_dac_val >= 4095 && error_mv > 0) ||
        (ch->current_dac_val == 0 && error_mv < 0)) {
        hold_integral = 1;
    }

    if (!hold_integral) {
        ch->pi.integral_sum += (ch->pi.Ki * error_mv);

        // Pevné hardwarové limity integrátoru
        if (ch->pi.integral_sum > ch->pi.integral_max) ch->pi.integral_sum = ch->pi.integral_max;
        if (ch->pi.integral_sum < ch->pi.integral_min) ch->pi.integral_sum = ch->pi.integral_min;
    }

    // 5. Výpočet výsledku do DAC
    // Výsledek dělíme 1000, protože Kp a Ki pracují v řádech tisícin (Kp = 10 znamená zisk 0.01)
    // To nám umožňuje extrémně jemné ladění regulátoru pouze pomocí celých čísel.
    int32_t dac_output = (p_term + ch->pi.integral_sum) / 1000;

    // 6. Finální ořez (Saturace) a zápis do struktury
    if (dac_output > 4095) dac_output = 4095;
    if (dac_output < 0) dac_output = 0;

    ch->current_dac_val = (uint32_t)dac_output;
}
