#ifndef AR9300_EEPROM_H
#define AR9300_EEPROM_H

#include "eeprom.h"

struct ar9300_fixup;
struct ar9300_layout;

u32 dump_eeprom(struct ar9300_eeprom *eep, char *buf, u32 len);
u32 dump_fixup(struct ar9300_fixup *fixup, char *buf, u32 len);
int read_eeproms(struct ar9300_eeprom eeproms[], const struct ar9300_layout *layout, char *filename);
int write_eeproms(struct ar9300_eeprom eeproms[], const struct ar9300_layout *layout, char *filename, char *outname);
bool default_detect(struct ar9300_eeprom eeproms[], const void *data);

#define AR9300_MAX_EEPROMS           2

#define AR9300_MAX_CHAINS            3
#define AR9300_NUM_5G_CAL_PIERS      8
#define AR9300_NUM_2G_CAL_PIERS      3

#define CTL(_tpower, _flag) ((_tpower) | ((_flag) << 6))

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

struct ar9300_fixup {
    const char *info;

    bool allow2G;
    u8 calFreqPier2G[AR9300_NUM_2G_CAL_PIERS];
    struct ar9300_cal_data_per_freq_op_loop calPierData2G[AR9300_MAX_CHAINS][AR9300_NUM_2G_CAL_PIERS];

    bool allow5G;
    u8 calFreqPier5G[AR9300_NUM_5G_CAL_PIERS];
    struct ar9300_cal_data_per_freq_op_loop calPierData5G[AR9300_MAX_CHAINS][AR9300_NUM_5G_CAL_PIERS];
} __packed;

#ifdef _MSC_VER
#pragma pack(pop)
#endif

struct ar9300_layout {
    const s32 offsets[AR9300_MAX_EEPROMS];
};

struct ar9300_revision {
    const char *name;
    const char *version;
    const char *number;
    const char *revision;
    const struct ar9300_fixup *fixup;
    bool (*detect)(struct ar9300_eeprom eeproms[], const void *data);
    const void *data;
};

struct ar9300_detect {
    const char *name;
    struct ar9300_layout layout;
    const struct ar9300_revision *revisions[];
};

struct default_detect_data {
    u8 params_for_tuning_caps[2];

    bool allow2G;
    u8 calFreqPier2G[AR9300_NUM_2G_CAL_PIERS];
    struct ar9300_cal_data_per_freq_op_loop calPierData2G[AR9300_MAX_CHAINS][AR9300_NUM_2G_CAL_PIERS];

    bool allow5G;
    u8 calFreqPier5G[AR9300_NUM_5G_CAL_PIERS];
    struct ar9300_cal_data_per_freq_op_loop calPierData5G[AR9300_MAX_CHAINS][AR9300_NUM_5G_CAL_PIERS];
};

#endif
