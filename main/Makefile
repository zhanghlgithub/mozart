DIR_CUR := $(shell pwd)

# $1: string
# $2: start pos
# $3: length
define substring
$(shell echo `expr substr $(1) $(2) $(3)`)
endef

BT ?= 0
VR ?= 0
TTS ?= 0
LAPSULE ?= 0
INGENICPLAYER ?= 0
DMR ?= 0
WEBRTC ?= 0
DMS ?= 0
AIRPLAY ?= 0
LOCALPLAYER ?= 0
ALARM ?= 0
MULROOM ?= 0
CAMERA ?= 0

CC = mipsel-linux-gcc
CXX = mipsel-linux-g++
STRIP = mipsel-linux-strip
INSTALL = install
DESTDIR =
PREFIX = /usr
INITDIR = /etc/init.d/
SBINDIR = $(PREFIX)/sbin

SRCS = mozart.c mozart_app.c mozart_key_function.c mozart_tts.c mozart_musicplayer.c egoserver.c lifting_appliance.c mozart_iic_touch.c
SRCS += misc/battery_capacity.c
SRCS += modules/mozart_module_linein.c

# common opts.
CFLAGS = -fPIC -Wall -Werror -g -O2 -I. -I./include/ -I./modules/ -I$(MOLIB_DIR)/usr/include -I$(MOLIB_UPDATER_DIR)/usr/include -I$(MOZART_DIR)/usr/include -I$(MOZART_UPDATER_DIR)/usr/include
CFLAGS += -rdynamic -fasynchronous-unwind-tables

LDFLAGS = -g -lpthread -lstdc++ -L$(MOLIB_DIR)/usr/lib -L$(MOLIB_UPDATER_DIR)/usr/lib -L$(MOZART_DIR)/usr/lib -L$(MOZART_UPDATER_DIR)/usr/lib
LDFLAGS += -Wl,-rpath-link,$(MOLIB_DIR)/usr/lib:$(MOLIB_UPDATER_DIR)/usr/lib:$(MOZART_DIR)/usr/lib

# basic depends.
LDFLAGS += -levent -lpower -lwifi -lvolume -ltips -lsharememory -lini -lutils -lpthread -lplayer -ljson-c -lm -lresample -lnvrw -lupdater -lmusiclist -llinklist -lasound -llocalstream -lcurl

SRCS += modules/mozart_module_cloud_music.c

ifeq ("$(SONG_SUPPLYER)","aispeech")
SRCS += modules/song_supplyer/speech.c
endif

ifeq ("$(SONG_SUPPLYER)","ximalaya")
SRCS += modules/song_supplyer/ximalaya.c
LDFLAGS += -lximalaya
endif

ifeq ("$(SONG_SUPPLYER)","jushang")
SRCS += modules/song_supplyer/jushang.c
LDFLAGS += -lvr_jushang
endif

ifeq ($(LOCALPLAYER),1)
SRCS += modules/mozart_module_local_music.c
LDFLAGS += -llocalplayer
else
$(warning localplayer func CLOSED!!!!)
endif

ifeq ($(DMR),1)
CFLAGS += -I$(MOLIB_DIR)/usr/include/upnp
LDFLAGS += -lrender
SRCS += modules/mozart_module_dmr.c
else
$(warning dmr func CLOSED!!!!)
endif

ifeq ($(MULROOM),1)
SRCS += modules/mozart_module_mulroom.c
LDFLAGS += -lmulroom
else
$(warning multiroom func CLOSED!!!!)
endif

ifeq ($(CAMERA),1)
SRCS += modules/camera/camera.c \
	modules/camera/lcd.c\
	modules/camera/video.c\
	modules/camera/savebmp.c\
	modules/camera/convert.c\
	modules/mozart_module_camera.c
else
$(warning camera func CLOSED!!!!)
endif

ifeq ("$(TTS)","baidu")
SRCS += misc/tts_baidu.c
else
ifeq ("$(TTS)","jietong")
LDFLAGS +=  -lhci_sys -lhci_curl -lhci_tts -ltts_jietong
else
ifeq ("$(TTS)","aispeech")
LDFLAGS +=  -lvr
endif
endif
endif

ifeq ($(AIRPLAY),1)
SRCS += modules/mozart_module_airplay.c
else
$(warning airplay func CLOSED!!!!)
endif

ifeq ($(BT),0)
$(warning bt func are CLOSED!!!!)
else
SRCS += modules/mozart_module_bt.c
endif
ifeq ("$(call substring,$(BT),1,3)", "rtk") # realtek's bt module.
endif
ifeq ("$(call substring,$(BT),1,3)", "bcm") # broadcom's bt module.
LDFLAGS += -lbsa -lbt -lappcommon -lchannels
endif

ifeq ($(INGENICPLAYER),1)
SRCS +=  ingenicplayer/appserver.c ingenicplayer/ingenicplayer.c 
LDFLAGS += -lappserver
else
$(warning ingenicplayer func CLOSED!!!!)
endif

ifeq ("$(VR)","0")
$(warning vr func are CLOSED!!!!)
else
SRCS += modules/mozart_module_vr.c
endif
ifeq ("$(VR)","aispeech")
LDFLAGS +=  -lvr
endif
ifeq ("$(VR)","iflytek")
LDFLAGS += -lvr_iflytek
endif
ifeq ("$(VR)","jietong")
LDFLAGS += -lvr_jietong -lrecord -lwakeup_jietong
endif

ifeq ("$(UI)","0")
$(warning ui func are CLOSED!!!!)
else
SRCS += mozart_ui.c
endif
ifeq ("$(UI)","lcd")
LDFLAGS += -llcd -llcdshow -lpng -lz
endif
ifeq ("$(UI)","smartui")
LDFLAGS += -lsmartui
endif

ifeq ($(ALARM),1)
SRCS += modules/mozart_module_alarm.c
LDFLAGS += -lalarm
else
$(warning alarm func CLOSED!!!!)
endif

ifeq ($(WEBRTC),1)
LDFLAGS += -lwebrtc_audio_processing -lwebrtc
else
$(warning webrtc func CLOSED!!!!)
endif

ifeq ($(DMS),1)
SRCS += misc/dms.c
else
$(warning dms func CLOSED!!!!)
endif

ifeq ($(LAPSULE),1)
SRCS += misc/lapsule_control.c
else
$(warning lapsule func CLOSED!!!!)
endif

OBJS = $(patsubst %.c, %.c.o, $(SRCS))
TARGET = mozart

all:$(TARGET)

$(TARGET):$(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.c.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

install:
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -d $(DESTDIR)$(INITDIR)
	if [ -e $(TARGET) ]; then \
		$(INSTALL) -m 755 -s --strip-program=$(STRIP) $(TARGET) $(DESTDIR)$(SBINDIR)/; \
		$(INSTALL) -m 755 S10mozart.sh $(DESTDIR)$(INITDIR)/; \
	fi

clean:
	-rm -rf $(OBJS) $(TARGET)

uninstall:
	if [ -e $(DESTDIR)$(SBINDIR)/$(TARGET)]; then \
		rm -rf $(DESTDIR)$(SBINDIR)/$(TARGET); \
		rm -rf $(DESTDIR)$(INITDIR)/S10mozart.sh; \
	fi

.PHONY:all clean install uninstall $(TARGET)
