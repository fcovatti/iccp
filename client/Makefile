LIBIEC_HOME=../../libiec_iccp_mod

PROJECT_BINARY_NAME = iccp_client
PROJECT_SOURCES = client.c
PROJECT_SOURCES += util.c
PROJECT_SOURCES += comm.c
#PROJECT_SOURCES += control.c

include $(LIBIEC_HOME)/make/target_system.mk
include $(LIBIEC_HOME)/make/stack_includes.mk

all:	$(PROJECT_BINARY_NAME)

include $(LIBIEC_HOME)/make/common_targets.mk

$(PROJECT_BINARY_NAME):	$(PROJECT_SOURCES) $(LIB_NAME)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(PROJECT_BINARY_NAME) -I../include $(PROJECT_SOURCES) $(INCLUDES) $(LIB_NAME) $(LDLIBS)

clean:
	rm -f $(PROJECT_BINARY_NAME)


