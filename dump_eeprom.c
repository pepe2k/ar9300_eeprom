#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>

#include "ar9300_eeprom.h"

#define BUFSIZE 1024

#define BIT(nr) (1UL << (nr))

#define le16_to_cpu(x) ((u16)(x))
#define le32_to_cpu(x) ((u32)(x))

#define PR_NL() len += sprintf(buf + len, "\n")
#define PR_HDR(_s) len += sprintf(buf + len, "%20s :\n", _s)
#define PR_DSC(_s, _d) len += sprintf(buf + len, "%20s :%s\n", _s, _d)
#define PR_VAL(_s, _val) len += sprintf(buf + len, "%20s : %10d\n", _s, _val)
#define PR_SEP() len += sprintf(buf + len, "----------------------------------------------------------------------------------------\n")
#define PR_SEPVAL() len += sprintf(buf + len, "---------------------------------\n")

#define PR_BTS(_s, _val) \
do { \
    bits2str(buf2, _val, 8 * sizeof(_val)); \
    len += sprintf(buf + len, "%20s : 0b%s\n", _s, buf2); \
} while (0)

#define PR_HEX(_s, _val) \
do { \
    switch (sizeof(_val)) \
    { \
    case 1: \
        len += sprintf(buf + len, "%20s :       0x%2.2X\n", _s, _val); \
        break; \
    case 2: \
        len += sprintf(buf + len, "%20s :     0x%4.4X\n", _s, le16_to_cpu(_val)); \
        break; \
    case 4: \
        len += sprintf(buf + len, "%20s : 0x%8.8X\n", _s, le32_to_cpu(_val)); \
        break; \
    default: \
        len += sprintf(buf + len, "%20s : 0x????????\n", _s); \
        break; \
    } \
} while (0)

#define PR_HEXS(_s, _val, _step, _start) \
do { \
    int _i; \
    len += sprintf(buf + len, "%20s :", _s); \
    switch (sizeof(_val[0])) \
    { \
    case 1: \
        for (_i = 0; _i < sizeof(_val); _i++) \
            len += sprintf(buf + len, "%*s0x%2.2X", ((_i == 0) ? _step + _start  : _step) - 4, "", _val[_i]); \
        break; \
    case 2: \
        for (_i = 0; _i < sizeof(_val) / 2; _i++) \
            len += sprintf(buf + len, "%*s0x%4.4X", ((_i == 0) ? _step + _start  : _step) - 6, "", le16_to_cpu(_val[_i])); \
        break; \
    case 4: \
        for (_i = 0; _i < sizeof(_val) / 4; _i++) \
            len += sprintf(buf + len, "%*s0x%8.8X", ((_i == 0) ? _step + _start  : _step) - 10, "", le32_to_cpu(_val[_i])); \
        break; \
    default: \
        len += sprintf(buf + len, "0x????????"); \
        break; \
    } \
    len += sprintf(buf + len, "\n"); \
} while (0)

#define PR_VALS(_s, _val, _step, _start) \
do { \
    int _i; \
    len += sprintf(buf + len, "%20s :", _s); \
    switch (sizeof(_val[0])) \
    { \
    case 1: \
        for (_i = 0; _i < sizeof(_val); _i++) \
            len += sprintf(buf + len, "%*d", (_i == 0) ? _step + _start  : _step, _val[_i]); \
        break; \
    case 2: \
        for (_i = 0; _i < sizeof(_val) / 2; _i++) \
            len += sprintf(buf + len, "%*d", (_i == 0) ? _step + _start  : _step, le16_to_cpu(_val[_i])); \
        break; \
    case 4: \
        for (_i = 0; _i < sizeof(_val) / 4; _i++) \
            len += sprintf(buf + len, "%*d", (_i == 0) ? _step + _start  : _step, le32_to_cpu(_val[_i])); \
        break; \
    default: \
        len += sprintf(buf + len, "????????"); \
        break; \
    } \
    len += sprintf(buf + len, "\n"); \
} while (0)

#define PR_LMH(_s, _l, _m, _h, _step) \
do { \
    switch (sizeof(_l)) \
    { \
    case 1: \
        len += sprintf(buf + len, "%20s :%*d%*d%*d\n", _s, _step, _l, _step, _m, _step, _h); \
        break; \
    case 2: \
        len += sprintf(buf + len, "%20s :%*d%*d%*d\n", _s, _step, le16_to_cpu(_l), _step, le16_to_cpu(_m), _step, le16_to_cpu(_h)); \
        break; \
    case 4: \
        len += sprintf(buf + len, "%20s :%*d%*d%*d\n", _s, _step, le32_to_cpu(_l), _step, le32_to_cpu(_m), _step, le32_to_cpu(_h)); \
        break; \
    default: \
        len += sprintf(buf + len, "%20s :????????\n", _s); \
        break; \
    } \
} while (0)

#define PR_LMHs(_s, _l, _m, _h, _step) \
do { \
    int _i; \
    len += sprintf(buf + len, "%20s :", _s); \
    switch (sizeof(_l[0])) \
    { \
    case 1: \
        for (_i = 0; _i < sizeof(_l); _i++) \
            len += sprintf(buf + len, "%*d%*d%*d", _step, _l[_i], _step, _m[_i], _step, _h[_i]); \
        break; \
    case 2: \
        for (_i = 0; _i < sizeof(_l) / 2; _i++) \
            len += sprintf(buf + len, "%*d%*d%*d", _step, le16_to_cpu(_l[_i]), _step, le16_to_cpu(_m[_i]), _step, le16_to_cpu(_h[_i])); \
        break; \
    case 4: \
        for (_i = 0; _i < sizeof(_l) / 4; _i++) \
            len += sprintf(buf + len, "%*d%*d%*d", _step, le32_to_cpu(_l[_i]), _step, le32_to_cpu(_m[_i]), _step, le32_to_cpu(_h[_i])); \
        break; \
    default: \
        len += sprintf(buf + len, "????????"); \
        break; \
    } \
    len += sprintf(buf + len, "\n"); \
} while (0)

#define PR_DMP(_s, _val) \
do { \
    int _i; \
    len += sprintf(buf + len, "%20s :", _s); \
    for (_i = 0; _i < sizeof(_val); _i++) \
        len += sprintf(buf + len, " %2.2X", ((u8*)(_val))[_i]); \
    len += sprintf(buf + len, "\n"); \
} while (0)

#define PR_FREQ(_s, _val, _idx, _is2GHz) \
do { \
    u32 _v = _val[_idx]; \
    sprintf(buf2, "%s %d", _s, _idx); \
    len += sprintf(buf + len, "%20s :", buf2); \
    if (_v == 0 || _v == 0xff) \
        sprintf(buf2, "0x%2.2X\n", _v); \
    else \
    { \
        u32 _freq = FBIN2FREQ(_v, _is2GHz); \
        int _chn = freq2chn(_freq); \
        if (_chn < 0) \
            sprintf(buf2, "%4.4d[%d+]\n", _freq, -_chn); \
        else \
            sprintf(buf2, "%4.4d[%d]\n", _freq, _chn); \
    } \
    len += sprintf(buf + len, "%12s", buf2); \
} while (0)

#define PR_FREQS(_s, _val, _idx, _is2GHz, _step) \
do { \
    int _i; \
    int _size = sizeof((_val)[0]); \
    if (_idx < 0) \
        len += sprintf(buf + len, "%20s :", _s); \
    else \
    { \
        sprintf(buf2, "%s %d", _s, _idx); \
        len += sprintf(buf + len, "%20s :", buf2); \
    } \
    for (_i = (_idx >= 0) ? _idx : 0; _i < sizeof(_val); _i += _size) \
    { \
        u32 _v = ((u8*)(_val))[_i]; \
        if (_v == 0 || _v == 0xff) \
            sprintf(buf2, "0x%2.2X", _v); \
        else \
        { \
            u32 _freq = FBIN2FREQ(_v, _is2GHz); \
            int _chn = freq2chn(_freq); \
            if (_chn < 0) \
                sprintf(buf2, "%4.4d[%d+]", _freq, -_chn); \
            else \
                sprintf(buf2, "%4.4d[%d]", _freq, _chn); \
        } \
        len += sprintf(buf + len, "%*s", _step, buf2); \
    } \
    len += sprintf(buf + len, "\n"); \
} while (0)

#define PR_PIER(_s, _v, _i) \
do { \
    sprintf(buf2, "%s %d", _s, pier); \
    len += sprintf(buf + len, "%20s :%7d%7d%7d\n", buf2, _v[0][pier]._i, _v[1][pier]._i, _v[2][pier]._i); \
} while (0)

#define PR_PWRLEG(_s, _val, _idx) \
do { \
    struct cal_tgt_pow_legacy _v; \
    _v = _val[_idx]; \
    sprintf(buf2, "%s %d", _s, _idx); \
    len += sprintf(buf + len, "%20s :%5d%5d%5d%5d\n", buf2, _v.tPow2x[0], _v.tPow2x[1], _v.tPow2x[2], _v.tPow2x[3]); \
} while (0)

#define PR_PWRHT(_s, _val, _idx) \
do { \
    struct cal_tgt_pow_ht _v; \
    _v = _val[_idx]; \
    sprintf(buf2, "%s %d", _s, _idx); \
    len += sprintf(buf + len, "%20s :%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d%4d\n", buf2, \
        _v.tPow2x[0], _v.tPow2x[1], _v.tPow2x[2], _v.tPow2x[3], _v.tPow2x[4], _v.tPow2x[5], _v.tPow2x[6], _v.tPow2x[7], \
        _v.tPow2x[8], _v.tPow2x[9], _v.tPow2x[10], _v.tPow2x[11], _v.tPow2x[12], _v.tPow2x[13]); \
} while (0)

#define PR_PWRIDX(_s, _val, _idx, _be, _af) \
do { \
    int _i; \
    int _size = sizeof((_val)[0]); \
    sprintf(buf2, "%s %d", _s, _idx); \
    len += sprintf(buf + len, "%20s :", buf2); \
    for (_i = _idx; _i < sizeof(_val); _i += _size) \
    { \
        u32 _v = ((u8*)(_val))[_i]; \
        len += sprintf(buf + len, _be "%2.2d|%1.1d" _af, CTL_EDGE_TPOWER(_v), CTL_EDGE_FLAGS(_v)); \
    } \
    len += sprintf(buf + len, "\n"); \
} while (0)

#define PR_CTL(_s, _val, _step) \
do { \
    int _i; \
    len += sprintf(buf + len, "%20s :", _s); \
    for (_i = 0; _i < sizeof(_val); _i++) \
    { \
        len += sprintf(buf + len, "%*s", _step - 8, ""); \
        len += ctl2str(buf + len, _val[_i]); \
    } \
    len += sprintf(buf + len, "\n"); \
} while (0)

#define MS(_v, _f)  (((_v) & _f) >> _f##_S)

#define PR_RTS(_s, _val, _isHT20, _is2GHz) \
do { \
    int _i; \
    u32 _m; \
    len += sprintf(buf + len, "%20s :", _s); \
    _m = 1 << 0; \
    for (_i = 0; _i < 25; _i++, _m += _m) \
    { \
        if (_val & _m) \
            len += sprintf(buf + len, " %s", rates[_i]); \
    } \
    if (_isHT20) \
    { \
        if (_is2GHz) \
            len += sprintf(buf + len, " SCALE:%d", MS(_val, AR9300_PAPRD_SCALE_1)); \
        else \
            len += sprintf(buf + len, " HIGH_SCALE:%d", MS(_val, AR9300_PAPRD_SCALE_1)); \
        _m = 1 << 28; \
        for (_i = 28; _i < 32; _i++, _m += _m) \
        { \
            if (_val & _m) \
            { \
                if (_is2GHz) \
                    len += sprintf(buf + len, " BIT%d", _i); \
                else \
                    len += sprintf(buf + len, " %s", rates[_i]); \
            } \
        } \
    } \
    else \
        if (_is2GHz) \
        { \
            _m = 1 << 25; \
            for (_i = 25; _i < 32; _i++, _m += _m) \
            { \
                if (_val & _m) \
                    len += sprintf(buf + len, " BIT%d", _i); \
            } \
        } \
        else \
        { \
            len += sprintf(buf + len, " LOW_SCALE:%d", MS(_val, AR9300_PAPRD_SCALE_1)); \
            len += sprintf(buf + len, " MID_SCALE:%d", MS(_val, AR9300_PAPRD_SCALE_2)); \
            if (_val & (1 << 31)) \
                len += sprintf(buf + len, " BIT%d", 31); \
        } \
    len += sprintf(buf + len, "\n"); \
} while (0)

const char *rates[] =
{
    "6_24", "36", "48", "54", "1L_5L", "5S", "11L", "11S", "HT20_0_8_16", "HT20_1_3_9_11_17_19", "HT20_4", "HT20_5", "HT20_6", "HT20_7",
    "HT20_12", "HT20_13", "HT20_14", "HT20_15", "HT20_20", "HT20_21", "HT20_22", "HT20_23", "HT40_0_8_16", "HT40_1_3_9_11_17_19", "HT40_4",
    "BIT25", "BIT26", "BIT27", "LOW_SCALE_DIS", "MID_SCALE_DIS", "HIGH_SCALE_DIS", "BIT31"
};

void bits2str(char *buf, u32 val, int bits)
{
    u32 mask;
    buf[bits] = '\0';
    memset(buf, '0', bits);
    for (mask  = 1 << (bits - 1); val && mask; buf++, mask >>= 1)
    {
        if (val & mask)
        {
            val &= ~mask;
            *buf = '1';
        }
    }
}

enum ctl_group {
	CTL_FCC = 0x10,
	CTL_MKK = 0x40,
	CTL_ETSI = 0x30,
};

static int ctl2str(char *buf, u8 val)
{
    switch (val & ~CTL_MODE_M)
    {
    case CTL_FCC:
        strcpy(buf, "FCC_");
        break;
    case CTL_MKK:
        strcpy(buf, "MKK_");
        break;
    case CTL_ETSI:
        strcpy(buf, "ETSI");
        break;
    default:
        strcpy(buf, "???_");
        break;
    }

    buf += 4;

    switch (val & CTL_MODE_M)
    {
    case CTL_11A:
        strcpy(buf, "_11A");
        break;
    case CTL_11B:
        strcpy(buf, "_11B");
        break;
    case CTL_11G:
        strcpy(buf, "_11G");
        break;
    case CTL_2GHT20:
    case CTL_5GHT20:
        strcpy(buf, "HT20");
        break;
    case CTL_2GHT40:
    case CTL_5GHT40:
        strcpy(buf, "HT40");
        break;
    default:
        strcpy(buf, "_???");
        break;
    }

    return 4 + 4;
}

struct channels { int chn; u32 freq; };
const struct channels chns[] =
{
    {   1, 2412 }, {   2, 2417 }, {   3, 2422 }, {   4, 2427 }, {   5, 2432 }, {   6, 2437 }, {   7, 2442 },
    {   8, 2447 }, {   9, 2452 }, {  10, 2457 }, {  11, 2462 }, {  12, 2467 }, {  13, 2472 }, {  14, 2484 },
    { 184, 4920 }, { 185, 4925 }, { 187, 4935 }, { 188, 4940 }, { 189, 4945 }, { 192, 4960 }, { 196, 4980 },
    {   7, 5035 }, {   8, 5040 }, {   9, 5045 }, {  11, 5055 }, {  12, 5060 }, {  16, 5080 }, {  34, 5170 },
    {  36, 5180 }, {  38, 5190 }, {  40, 5200 }, {  42, 5210 }, {  44, 5220 }, {  46, 5230 }, {  48, 5240 },
    {  52, 5260 }, {  56, 5280 }, {  60, 5300 }, {  64, 5320 }, { 100, 5500 }, { 104, 5520 }, { 108, 5540 },
    { 112, 5560 }, { 116, 5580 }, { 120, 5600 }, { 124, 5620 }, { 128, 5640 }, { 132, 5660 }, { 136, 5680 },
    { 140, 5700 }, { 149, 5745 }, { 153, 5765 }, { 157, 5785 }, { 161, 5805 }, { 165, 5825 } 
};

const int last_chn = (sizeof(chns) / sizeof(chns[0])) - 1;

static int freq2chn(u32 freq)
{
    u32 f;
    int i;
    int b = 0;
    int e = last_chn;
    while (b <= e)
    {
        if ((f = chns[(i = (b + e) >> 1)].freq) == freq)
            return chns[i].chn;

        if (f < freq)
            b = i + 1;
        else
            e = i - 1;
    }
    return -chns[e].chn;
}

static u32 ar9003_dump_modal_eeprom(char *buf, u32 len, struct ar9300_modal_eep_header *modal_hdr,
    struct ar9300_BaseExtension_1 *baseExt1, struct ar9300_BaseExtension_2 *baseExt2, int is2GHz)
{
    char buf2[BUFSIZE];

    PR_SEP();
    if (is2GHz)
    {
        PR_DSC("2.4GHz Modal Header", " Chain0 Chain1 Chain2");
        PR_HEXS("Ant. Control", modal_hdr->antCtrlChain, 7, 0);
        PR_VALS("NF Threshold", modal_hdr->noiseFloorThreshCh, 7, 0);
        PR_VALS("xatten1DB", modal_hdr->xatten1DB, 7, 0);
        PR_VALS("xatten1Margin", modal_hdr->xatten1Margin, 7, 0);
        PR_VAL("Temp Slope", modal_hdr->tempSlope);
        PR_VAL("Quick Drop", modal_hdr->quick_drop);
    }
    else
    {
        PR_DSC("5GHz Modal Header", "    Chain0         Chain1         Chain2");
        PR_HEXS("Ant. Control", modal_hdr->antCtrlChain, 15, -5);
        PR_VALS("NF Threshold", modal_hdr->noiseFloorThreshCh, 15, -5);
        PR_DSC("Range Chain", "  Low  Mid  High Low  Mid  High Low  Mid  High");
        PR_LMHs("xatten1DB", baseExt2->xatten1DBLow, modal_hdr->xatten1DB, baseExt2->xatten1DBHigh, 5);
        PR_LMHs("xatten1Margin", baseExt2->xatten1MarginLow, modal_hdr->xatten1Margin, baseExt2->xatten1MarginHigh, 5);
        PR_DSC("Range", "  Low  Mid  High");
        PR_LMH("Temp Slope", baseExt2->tempSlopeLow, modal_hdr->tempSlope, baseExt2->tempSlopeHigh, 5);
        PR_LMH("Quick Drop", baseExt1->quick_drop_low, modal_hdr->quick_drop, baseExt1->quick_drop_high, 5);
    }

    PR_FREQS("spur Frequency", modal_hdr->spurChans, -1, is2GHz, 11);
    PR_HEX("Ant. Common Control", modal_hdr->antCtrlCommon);
    PR_HEX("Ant. Common Control2", modal_hdr->antCtrlCommon2);
    PR_VAL("Ant. Gain", modal_hdr->antennaGain);
    PR_VAL("Switch Settle", modal_hdr->switchSettling);
    PR_VAL("Volt Slope", modal_hdr->voltSlope);

    PR_VAL("txEndToXpaOff", modal_hdr->txEndToXpaOff);
    PR_VAL("txEndToRxOn", modal_hdr->txEndToRxOn);
    PR_VAL("XPA Bias Level", modal_hdr->xpaBiasLvl);
    PR_VAL("txFrameToDataStart", modal_hdr->txFrameToDataStart);
    PR_VAL("txFrameToPaOn", modal_hdr->txFrameToPaOn);
    PR_VAL("txFrameToXpaOn", modal_hdr->txFrameToXpaOn);
    PR_VAL("txClip", modal_hdr->txClip);
    PR_VAL("ADC Desired Size", modal_hdr->adcDesiredSize);
    PR_VAL("Thresh62", modal_hdr->thresh62);
    PR_RTS("PAPRD Rate Mask", le32_to_cpu(modal_hdr->papdRateMaskHt20), 1, is2GHz);
    PR_RTS("PAPRD Rate Mask HT40", le32_to_cpu(modal_hdr->papdRateMaskHt40), 0, is2GHz);
    PR_VAL("switch com spdt", le16_to_cpu(modal_hdr->switchcomspdt));
    PR_VAL("XLNA Bias Strength", modal_hdr->xlna_bias_strength);
    PR_DMP("Reserved", modal_hdr->reserved);
    PR_DMP("Future Modal", modal_hdr->futureModal);

    return len;
}

static u32 ar9300_cal_data_per_freq_op_loop_eeprom_2G(char *buf, u32 len,
    struct ar9300_cal_data_per_freq_op_loop data[AR9300_MAX_CHAINS][AR9300_NUM_2G_CAL_PIERS], int pier)
{
    char buf2[BUFSIZE];

    PR_PIER("Ref Power", data, refPower);
    PR_PIER("Volt Meas", data, voltMeas);
    PR_PIER("Temp Meas", data, tempMeas);
    PR_PIER("RX Noisefloor Cal", data, rxNoisefloorCal);
    PR_PIER("RX Noisefloor Pwr", data, rxNoisefloorPower);
    PR_PIER("RX Temp Meas", data, rxTempMeas);

    return len;
}

static u32 ar9300_cal_data_per_freq_op_loop_eeprom_5G(char *buf, u32 len,
    struct ar9300_cal_data_per_freq_op_loop data[AR9300_MAX_CHAINS][AR9300_NUM_5G_CAL_PIERS], int pier)
{
    char buf2[BUFSIZE];

    PR_PIER("Ref Power", data, refPower);
    PR_PIER("Volt Meas", data, voltMeas);
    PR_PIER("Temp Meas", data, tempMeas);
    PR_PIER("RX Noisefloor Cal", data, rxNoisefloorCal);
    PR_PIER("RX Noisefloor Pwr", data, rxNoisefloorPower);
    PR_PIER("RX Temp Meas", data, rxTempMeas);

    return len;
}

u32 dump_eeprom(struct ar9300_eeprom *eep, char *buf, u32 len)
{
    int i;
    char buf2[BUFSIZE];
    bool allow2G = true;
    bool allow5G = true;
    struct ar9300_base_eep_hdr *pBase;
    struct ar9300_BaseExtension_1 *pBaseExt1;

    PR_SEP();
    PR_VAL("EEPROM Version", eep->eepromVersion);
    PR_VAL("Template Version", eep->templateVersion);
    PR_DMP("MAC Address", eep->macAddr);
    PR_DMP("Cust Data", eep->custData);

    pBase = &eep->baseEepHeader;

#ifndef FULLDUMP
    allow2G = !!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G);
    allow5G = !!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A);
#endif

    PR_SEP();
    PR_VAL("RegDomain1", le16_to_cpu(pBase->regDmn[0]));
    PR_VAL("RegDomain2", le16_to_cpu(pBase->regDmn[1]));
    PR_BTS("TX Mask", (u8)(pBase->txrxMask >> 4));
    PR_BTS("RX Mask", (u8)(pBase->txrxMask & 0x0f));
    PR_SEPVAL();
    PR_BTS("Op Flags", pBase->opCapFlags.opFlags);
    PR_VAL("Allow 2.4GHz", !!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11G));
    PR_VAL("Allow 5GHz", !!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_11A));
    if (allow2G)
    {
        PR_VAL("Disable 2.4GHz HT20", !!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_N_2G_HT20));
        PR_VAL("Disable 2.4GHz HT40", !!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_N_2G_HT40));
    }
    if (allow5G)
    {
        PR_VAL("Disable 5Ghz HT20", !!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_N_5G_HT20));
        PR_VAL("Disable 5Ghz HT40", !!(pBase->opCapFlags.opFlags & AR5416_OPFLAGS_N_5G_HT40));
    }
    PR_SEPVAL();
    PR_BTS("EEP Misc", pBase->opCapFlags.eepMisc);
    PR_VAL("Big Endian", !!(pBase->opCapFlags.eepMisc & 0x01));
    PR_SEPVAL();
    PR_VAL("RF Silent", pBase->rfSilent);
    PR_VAL("BT Option", pBase->blueToothOptions);
    PR_VAL("Device Cap", pBase->deviceCap);
    PR_VAL("Device Type", pBase->deviceType);
    PR_VAL("Power Table Offset", pBase->pwrTableOffset);
    PR_VALS("Tuning Caps", pBase->params_for_tuning_caps, 5, 1);
    PR_SEPVAL();
    PR_BTS("Feature Enable", pBase->featureEnable);
    PR_VAL("Enable Tx Temp Comp", !!(pBase->featureEnable & BIT(0)));
    PR_VAL("Enable Tx Volt Comp", !!(pBase->featureEnable & BIT(1)));
    PR_VAL("Enable Fast Clock", !!(pBase->featureEnable & BIT(2)));
    PR_VAL("Enable Doubling", !!(pBase->featureEnable & BIT(3)));
    PR_VAL("Internal Regulator", !!(pBase->featureEnable & BIT(4)));
    PR_VAL("Enable PAPRD Scale", !!(pBase->featureEnable & BIT(5)));
    PR_VAL("Enable Tuning Caps", !!(pBase->featureEnable & BIT(6)));
    PR_VAL("XPA Timing Ctrl", !!(pBase->featureEnable & BIT(7)));
    PR_SEPVAL();
    PR_BTS("Misc Configuration", pBase->miscConfiguration);
    PR_VAL("Driver Strength", !!(pBase->miscConfiguration & BIT(0)));
    PR_VAL("Thermometer", ((pBase->miscConfiguration >> 1) & 0x3) - 1);
    PR_VAL("Chain Mask Reduce", !!(pBase->miscConfiguration & BIT(3)));
    PR_VAL("Enable Quick Drop", !!(pBase->miscConfiguration & BIT(4)));
    PR_VAL("Temp Slope Extension", !!(pBase->miscConfiguration & BIT(5)));
    PR_VAL("Enable Bias Strength", !!(pBase->miscConfiguration & BIT(6)));
    PR_SEPVAL();
    PR_VAL("Write Enable GPIO", pBase->eepromWriteEnableGpio);
    PR_VAL("WLAN Disable GPIO", pBase->wlanDisableGpio);
    PR_VAL("WLAN LED GPIO", pBase->wlanLedGpio);
    PR_VAL("Rx Band Select GPIO", pBase->rxBandSelectGpio);
    PR_VAL("Tx Gain", pBase->txrxgain >> 4);
    PR_VAL("Rx Gain", pBase->txrxgain & 0xf);
    PR_HEX("SW Reg", pBase->swreg);

    pBaseExt1 = &eep->base_ext1;

    PR_SEP();
    PR_VAL("Ant Div Control", pBaseExt1->ant_div_control);
    PR_DMP("Future", pBaseExt1->future);
    PR_SEPVAL();
    PR_BTS("Misc Enable", pBaseExt1->misc_enable);
    PR_VAL("TX Gain Cap", !!(pBaseExt1->misc_enable & BIT(0)));
    PR_VAL("Uncompress Checksum", !!(pBaseExt1->misc_enable & BIT(1)));
    if (allow2G)
        PR_VAL("MinCCApwr 2.4GHz", !!(pBaseExt1->misc_enable & BIT(2)));
    if (allow5G)
    {
        PR_VAL("MinCCApwr 5GHz", !!(pBaseExt1->misc_enable & BIT(3)));
        PR_SEPVAL();
        PR_VALS("Temp Slope Extension", pBaseExt1->tempslopextension, 4, -1);
    }

    if (allow2G)
    {
        PR_SEP();
        PR_DSC("Calibration 2.4GHz", "   Chain0 Chain1 Chain2");
        for (i = 0; i < AR9300_NUM_2G_CAL_PIERS; i++)
        {
            PR_NL();
            PR_FREQ("Frequency", eep->calFreqPier2G, i, 1);
            len = ar9300_cal_data_per_freq_op_loop_eeprom_2G(buf, len, eep->calPierData2G, i);
        }

        PR_SEP();
        PR_DSC("Target 2.4GHz 11B", " 1L-5L  5S  11L  11S");
        for (i = 0; i < AR9300_NUM_2G_CCK_TARGET_POWERS; i++)
        {
            PR_NL();
            PR_FREQ("Frequency", eep->calTarget_freqbin_Cck, i, 1);
            PR_PWRLEG("Power", eep->calTargetPowerCck, i);
        }

        PR_SEP();
        PR_DSC("Target 2.4GHz 11G", "  6-24  36   48   54");
        for (i = 0; i < AR9300_NUM_2G_20_TARGET_POWERS; i++)
        {
            PR_NL();
            PR_FREQ("Frequency", eep->calTarget_freqbin_2G, i, 1);
            PR_PWRLEG("Power", eep->calTargetPower2G, i);
        }

        PR_SEP();
        PR_DSC("Target 2.4GHz HT20", "0-16 1-19  4   5   6   7  12  13  14  15  20  21  22  23");
        for (i = 0; i < AR9300_NUM_2G_20_TARGET_POWERS; i++)
        {
            PR_NL();
            PR_FREQ("Frequency", eep->calTarget_freqbin_2GHT20, i, 1);
            PR_PWRHT("Power", eep->calTargetPower2GHT20, i);
        }

        PR_SEP();
        PR_DSC("Target 2.4GHz HT40", "0-16 1-19  4   5   6   7  12  13  14  15  20  21  22  23");
        for (i = 0; i < AR9300_NUM_2G_40_TARGET_POWERS; i++)
        {
            PR_NL();
            PR_FREQ("Frequency", eep->calTarget_freqbin_2GHT40, i, 1);
            PR_PWRHT("Power", eep->calTargetPower2GHT40, i);
        }

        PR_SEP();
        PR_CTL("Regulatory 2.4GHz", eep->ctlIndex_2G, 9);
        for (i = 0; i < AR9300_NUM_BAND_EDGES_2G; i++)
        {
            PR_NL();
            PR_FREQS("Frequency", eep->ctl_freqbin_2G, i, 1, 9);
            PR_PWRIDX("Power|Edge", eep->ctlPowerData_2G, i, "   ", "  ");
        }
    }

    if (allow5G)
    {
        PR_SEP();
        PR_DSC("Calibration 5GHz", "   Chain0 Chain1 Chain2");
        for (i = 0; i < AR9300_NUM_5G_CAL_PIERS; i ++)
        {
            PR_NL();
            PR_FREQ("Frequency", eep->calFreqPier5G, i, 0);
            len = ar9300_cal_data_per_freq_op_loop_eeprom_5G(buf, len, eep->calPierData5G, i);
        }

        PR_SEP();
        PR_DSC("Target 5GHz 11A", "  6-24  36   48   54");
        for (i = 0; i < AR9300_NUM_5G_20_TARGET_POWERS; i++)
        {
            PR_NL();
            PR_FREQ("Frequency", eep->calTarget_freqbin_5G, i, 0);
            PR_PWRLEG("Power", eep->calTargetPower5G, i);
        }

        PR_SEP();
        PR_DSC("Target 5GHz HT20", "0-16 1-19  4   5   6   7  12  13  14  15  20  21  22  23");
        for (i = 0; i < AR9300_NUM_5G_20_TARGET_POWERS; i++)
        {
            PR_NL();
            PR_FREQ("Frequency", eep->calTarget_freqbin_5GHT20, i, 0);
            PR_PWRHT("Power", eep->calTargetPower5GHT20, i);
        }

        PR_SEP();
        PR_DSC("Target 5GHz HT40", "0-16 1-19  4   5   6   7  12  13  14  15  20  21  22  23");
        for (i = 0; i < AR9300_NUM_5G_40_TARGET_POWERS; i++)
        {
            PR_NL();
            PR_FREQ("Frequency", eep->calTarget_freqbin_5GHT40, i, 0);
            PR_PWRHT("Power", eep->calTargetPower5GHT40, i);
        }

        PR_SEP();
        PR_CTL("Regulatory 5GHz", eep->ctlIndex_5G, 10);
        for (i = 0; i < AR9300_NUM_BAND_EDGES_5G; i++)
        {
            PR_NL();
            PR_FREQS("Frequency", eep->ctl_freqbin_5G, i, 0, 10);
            PR_PWRIDX("Power|Edge", eep->ctlPowerData_5G, i, "    ", "  ");
        }
    }

    if (allow2G)
        len = ar9003_dump_modal_eeprom(buf, len, &eep->modalHeader2G, pBaseExt1, &eep->base_ext2, 1);

    if (allow5G)
    {
        len = ar9003_dump_modal_eeprom(buf, len, &eep->modalHeader5G, pBaseExt1, &eep->base_ext2, 0);
        PR_SEP();
        PR_DSC("Frequency", "    Low    Mid   High");
        PR_DSC("PAPRD Scale", "  <5400 >=5400 >=5700");
        PR_DSC("Other", "   5180   5500   5785");
    }

    PR_SEP();

    return len;
}

u32 dump_fixup(struct ar9300_fixup *fixup, char *buf, u32 len)
{
    int i;
    char buf2[BUFSIZE];

    PR_SEP();
    PR_VAL("Allow 2.4GHz", fixup->allow2G);
    PR_VAL("Allow 5GHz", fixup->allow5G);
    if (fixup->allow2G)
    {
        PR_SEP();
        PR_DSC("Calibration 2.4GHz", "   Chain0 Chain1 Chain2");
        for (i = 0; i < AR9300_NUM_2G_CAL_PIERS; i++)
        {
            PR_NL();
            PR_FREQ("Frequency", fixup->calFreqPier2G, i, 1);
            len = ar9300_cal_data_per_freq_op_loop_eeprom_2G(buf, len, fixup->calPierData2G, i);
        }
    }

    if (fixup->allow5G)
    {
        PR_SEP();
        PR_DSC("Calibration 5GHz", "   Chain0 Chain1 Chain2");
        for (i = 0; i < AR9300_NUM_5G_CAL_PIERS; i ++)
        {
            PR_NL();
            PR_FREQ("Frequency", fixup->calFreqPier5G, i, 0);
            len = ar9300_cal_data_per_freq_op_loop_eeprom_5G(buf, len, fixup->calPierData5G, i);
        }
    }

    PR_SEP();

    return len;
}

