TARGET=spshell
LDLIBS += -lreadline


ifdef SP_LIBSPOTIFY_WITH_SCROBBLING
CFLAGS += -DSP_LIBSPOTIFY_WITH_SCROBBLING=1
OBJS += scrobbling.o
endif

include ../common.mk

OBJS += spshell.o spshell_posix.o appkey.o cmd.o browse.o search.o toplist.o inbox.o star.o playlist.o test.o



$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@
ifdef DEBUG
ifeq ($(shell uname),Darwin)
	install_name_tool -change @loader_path/../Frameworks/libspotify.framework/libspotify @rpath/libspotify.so $@
endif
endif
