#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>

#include "ar9300_eeprom.h"
#include "wdr4300.h"

const static struct ar9300_detect *ar9300_detects[] = {
    &wdr4300_detect,
    NULL
};

const static struct ar9300_layout layout0 = {
    { 0x1000, 0x5000 }
};

const static struct ar9300_layout *layouts[] = {
    &layout0,
    NULL
};

static char buffer[64*1024];
static struct ar9300_eeprom eeproms[AR9300_MAX_EEPROMS];

int main(int argc, char *argv[])
{
    int i;
    int j;
    int l;
    int offset;
    int len = 0;
    int ret = -1;
    bool err = false;
    bool dump = false;
    char *option = "";
    bool found = false;
    bool update = false;
    int use_layout = -1;
    char *outname = NULL;
    char *filename = NULL;
    const struct ar9300_layout *layout;
    const struct ar9300_detect *detect;
    const struct ar9300_revision *revision;

    for (i = 1; !err && i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            option = &argv[i][1];
            if (!dump && !strcmp(option, "d"))
            {
                dump = true;
                continue;
            }
            if (!update && !strcmp(option, "u"))
            {
                update = true;
                continue;
            }
            if (use_layout < 0 && !strcmp(option, "y0"))
            {
                use_layout = 0;
                continue;
            }
            err = true;
            printf("\nInvalid option: '%s'.\n", argv[i]);
        }
        else
        {
            if (outname == NULL && !strcmp(option, "u"))
            {
                outname = argv[i];
                continue;
            }
            if (filename == NULL)
            {
                filename = argv[i];
                continue;
            }
            err = true;
            printf("\nInvalid parameter: '%s'.\n", argv[i]);
        }
    }

    if (!err && filename == NULL)
    {
        err = true;
        puts("\nBinary image file of 'art' partition not specified.");
    }
    if (!err && update && outname == NULL)
    {
        err = true;
        puts("\nOutput binary image file 'art' not specified.");
    }
    if (!err && dump && update)
    {
        err = true;
        puts("\nDump and update can not be specified together.");
    }
    if (err)
        goto print;

    l = (use_layout >= 0) ? use_layout : 0;
    while (!found && (layout = layouts[l++]) != NULL)
    {
        if ((ret = read_eeproms(eeproms, layout, filename)) < 0)
            goto print;

        if (ret < AR9300_MAX_EEPROMS)
        {
            if (use_layout < 0)
                continue;

            printf("\nSmall file: '%s'.\n", filename);
            goto print;
        }

        i = 0;
        while (!found && (detect = ar9300_detects[i++]) != NULL)
        {
            if (memcmp(&detect->layout, layout, sizeof(struct ar9300_layout)))
                continue;

            j = 0;
            while (!found && (revision = detect->revisions[j++]) != NULL)
                found = revision->detect(eeproms, revision->data);
        }

        if (use_layout >= 0)
            break;
    }

    if (!found)
    {
        if (ret < AR9300_MAX_EEPROMS)
        {
            printf("\nSmall file: '%s'.\n", filename);
            goto print;
        }

        puts("Board not detected.");

        if (use_layout < 0)
            return 1;
    }
    else
        printf("Hardware: %s        Model: %s ver. %s        Board: %s rev. %s\n",
            detect->name, revision->name, revision->version, revision->number, revision->revision);

    for (i = 0; i < AR9300_MAX_EEPROMS && (offset = layout->offsets[i]) >= 0; i++)
    {
        struct ar9300_eeprom *eeprom = &eeproms[i];
        if (dump)
        {
            len += sprintf(buffer + len, "\nEEPROM 0x%4.4X:\n", offset);
            len = dump_eeprom(eeprom, buffer, len);
        }
        if (update)
        {
            bool allow2G = !!(eeprom->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11G);
            bool allow5G = !!(eeprom->baseEepHeader.opCapFlags.opFlags & AR5416_OPFLAGS_11A);
            printf("\nUpdate EEPROM 0x%4.4X:\n", offset);
            if (found && revision->fixup != NULL)
            {
                const struct ar9300_fixup *fixup = revision->fixup;
                if (allow2G && fixup->allow2G)
                {
                    puts("\nFix up 2.4GHz.");
                    memcpy(eeprom->calFreqPier2G, fixup->calFreqPier2G, sizeof(eeprom->calFreqPier2G));
                    memcpy(eeprom->calPierData2G, fixup->calPierData2G, sizeof(eeprom->calPierData2G));
                }
                if (allow5G && fixup->allow5G)
                {
                    puts("\nFix up 5GHz.");
                    memcpy(eeprom->calFreqPier5G, fixup->calFreqPier5G, sizeof(eeprom->calFreqPier5G));
                    memcpy(eeprom->calPierData5G, fixup->calPierData5G, sizeof(eeprom->calPierData5G));
                }
            }
            if (allow2G)
            {
                int c;
                puts("\nUpdate regulatory 2.4GHz.");
                for (c = 0; c < AR9300_NUM_CTLS_2G; c++)
                {
                    int e;
                    for (e = 0; e < AR9300_NUM_BAND_EDGES_2G; e++)
                        eeprom->ctlPowerData_2G[c].ctlEdges[e] = CTL(60, CTL_EDGE_FLAGS(eeprom->ctlPowerData_2G[c].ctlEdges[e]));
                }
            }
            if (allow5G)
            {
                int c;
                puts("\nUpdate regulatory 5GHz.");
                for (c = 0; c < AR9300_NUM_CTLS_5G; c++)
                {
                    int e;
                    for (e = 0; e < AR9300_NUM_BAND_EDGES_5G; e++)
                        eeprom->ctlPowerData_5G[c].ctlEdges[e] = CTL(60, CTL_EDGE_FLAGS(eeprom->ctlPowerData_5G[c].ctlEdges[e]));
                }
            }

        }
    }

    if (found && revision->fixup != NULL)
    {
        if (dump)
        {
            len += sprintf(buffer + len, "\n%s\n", revision->fixup->info);
            len = dump_fixup((struct ar9300_fixup *)revision->fixup, buffer, len);
        }
        else
            printf("\n%s\n", revision->fixup->info);
    }

    if (dump)
        puts(buffer);

    if (update)
        write_eeproms(eeproms, layout, filename, outname);

    return 0;

print:
    puts("\nUsage: ar9300_eeprom [-d|-y0] ART_FILE [-u OUT_FILE]\n\n\t\
-d: dump eeprom.\n\t\
-u: update eeprom.\n\t\
-y0: use layout 0 - eeprom in 0x1000 and eeprom in 0x5000.\n\n\t\
ART_FILE: 64KB binary image file of 'art' partition.\n\t\
OUT_FILE: updated binary image file 'art'.\n");

    return 1;
}
