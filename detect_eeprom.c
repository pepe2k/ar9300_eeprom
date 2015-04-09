#include <string.h>

#include "ar9300_eeprom.h"

bool default_detect(struct ar9300_eeprom eeproms[], const void *data)
{
    int i;

    for (i = 0; i < AR9300_MAX_EEPROMS; i++)
    {
        const struct ar9300_eeprom *eeprom = &eeproms[i];
        const struct default_detect_data *ddd = &((struct default_detect_data *)data)[i];
        if (!ddd->allow2G && !ddd->allow5G)
            return true;

        if (!(memcmp(ddd->params_for_tuning_caps, eeprom->baseEepHeader.params_for_tuning_caps, sizeof(ddd->params_for_tuning_caps)) == 0 &&
            ddd->allow2G == !!(eeprom->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11G) &&
            (!ddd->allow2G || memcmp(ddd->calFreqPier2G, eeprom->calFreqPier2G, sizeof(ddd->calFreqPier2G)) == 0) &&
            (!ddd->allow2G || memcmp(ddd->calPierData2G, eeprom->calPierData2G, sizeof(ddd->calPierData2G)) == 0) &&
            ddd->allow5G == !!(eeprom->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11A) &&
            (!ddd->allow5G || memcmp(ddd->calFreqPier5G, eeprom->calFreqPier5G, sizeof(ddd->calFreqPier5G)) == 0) &&
            (!ddd->allow5G || memcmp(ddd->calPierData5G, eeprom->calPierData5G, sizeof(ddd->calPierData5G)) == 0)))
            return false;
    }

    return true;
}
