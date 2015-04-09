#ifdef _MSC_VER

#define _CRT_SECURE_NO_WARNINGS

#include <io.h>

#define open _open
#define read _read
#define write _write
#define lseek _lseek
#define close _close

#endif

#ifdef __GNUC__

#include <unistd.h>

#define O_BINARY 0

#endif

#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>

#include "ar9300_eeprom.h"

int read_eeproms(struct ar9300_eeprom eeproms[], const struct ar9300_layout *layout, char *filename)
{
    int file;
    int i = 0;
    int offset;

    if ((file = open(filename, O_BINARY | O_RDONLY)) == -1)
    {
        printf("\nFile not found: '%s'.\n", filename);
        return -1;
    }

    for (i = 0; i < AR9300_MAX_EEPROMS && (offset = layout->offsets[i]) >= 0; i++)
    {
        struct ar9300_eeprom *eeprom = &eeproms[i];
        if ((lseek(file, offset, SEEK_SET) != offset || read(file, eeprom, sizeof(struct ar9300_eeprom)) != sizeof(struct ar9300_eeprom)))
        {
            close(file);
            return i;
        }
    }

    close(file);
    return AR9300_MAX_EEPROMS;
}

static char buffer[64*1024];
int write_eeproms(struct ar9300_eeprom eeproms[], const struct ar9300_layout *layout, char *filename, char *outname)
{
    int i;
    int out;
    int file;
    int bytes;
    int offset;

    if ((out = open(outname, O_BINARY | O_RDWR | O_CREAT | O_TRUNC, S_IREAD | S_IWRITE)) == -1)
    {
        printf("\nFile not open for output: '%s'.\n", outname);
        return -1;
    }

    if ((file = open(filename, O_BINARY | O_RDONLY)) == -1)
    {
        printf("\nFile not found: '%s'.\n", filename);
        close(out);
        return -1;
    }

    while ((bytes = read(file, buffer, sizeof(buffer))) > 0)
        if (write(out, buffer, bytes) != bytes)
        {
            printf("\nFile not write to output: '%s'.\n", outname);
            close(file);
            close(out);
            return -1;
        }

    close(file);
    for (i = 0; i < AR9300_MAX_EEPROMS && (offset = layout->offsets[i]) >= 0; i++)
    {
        struct ar9300_eeprom *eeprom = &eeproms[i];
        if ((lseek(out, offset, SEEK_SET) != offset || write(out, eeprom, sizeof(struct ar9300_eeprom)) != sizeof(struct ar9300_eeprom)))
        {
            close(out);
            return i;
        }
    }

    close(out);
    return AR9300_MAX_EEPROMS;
}
