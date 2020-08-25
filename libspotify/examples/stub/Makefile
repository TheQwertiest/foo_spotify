TARGET=posix_stu
CFLAGs += -Werror

include ../common.mk

$(TARGET): main.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(LDLIBS) $^ -o $@

ifdef DEBUG
ifeq ($(shell uname),Darwin)
	install_name_tool -change @loader_path/../Frameworks/libspotify.framework/libspotify @rpath/libspotify.so $@
endif
endif
