
OBJECTS=getsubopt.o  strchrnul.o  strnlen.o 
ifneq "" "no"
 OBJECTS+= getopt.o getopt1.o
endif

CPPFLAGS += -I/cygdrive/c/Users/fabrn/dvda-author/libiberty/src/include
#for config.h
CPPFLAGS += -I/cygdrive/c/Users/fabrn/dvda-author

libiberty.a: libiberty.a($(OBJECTS))

$(OBJECTS):%.o: /cygdrive/c/Users/fabrn/dvda-author/libiberty/src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
