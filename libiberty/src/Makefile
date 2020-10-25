
OBJECTS=getsubopt.o  strchrnul.o  strnlen.o 
ifneq "" "no"
 OBJECTS+= getopt.o getopt1.o
endif

CPPFLAGS += -I/home/fab/dvda-author/libiberty/src/include
#for config.h
CPPFLAGS += -I/home/fab/dvda-author

libiberty.a: libiberty.a($(OBJECTS))

$(OBJECTS):%.o: /home/fab/dvda-author/libiberty/src/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
