CC = gcc
CFLAGS += -g -O0 -Wall
CFLAGS += -I./include

all : ar9300_eeprom

DEF_INC = include/ar9300_eeprom.h include/eeprom.h include/types.h include/ar9003_eeprom.h

#ar9300_eeprom : CFLAGS = -Wall -O2 -I./include
#ar9300_eeprom : LDLIBS =
ar9300_eeprom : ar9300_eeprom.o detect_eeprom.o dump_eeprom.o io_eeproms.o
ar9300_eeprom.o : ar9300_eeprom.c $(DEF_INC) include/wdr4300.h
detect_eeprom.o : detect_eeprom.c $(DEF_INC)
dump_eeprom.o : dump_eeprom.c $(DEF_INC)
io_eeproms.o : io_eeproms.c $(DEF_INC)

clean :
	rm -rf *.o ar9300_eeprom
