#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/input.h>
#include <fcntl.h>

#include "ini_interface.h"
#include "wifi_interface.h"
#include "volume_interface.h"
#include "tips_interface.h"
#include "utils_interface.h"
#include "sharememory_interface.h"
#include "localplayer_interface.h"

#include "mozart_config.h"
#include "lapsule_control.h"

#include "mozart_app.h"
#include "mozart_ui.h"
#include "mozart_key_function.h"
#include "mozart_musicplayer.h"

#ifdef SUPPORT_VR
#include "modules/mozart_module_vr.h"
#endif

#if (SUPPORT_DMR == 1)
#include "modules/mozart_module_dmr.h"
#endif

#ifdef SUPPORT_BT
#include "modules/mozart_module_bt.h"
#endif
#include "modules/mozart_module_linein.h"
#include "modules/mozart_module_airplay.h"
#include "modules/mozart_module_cloud_music.h"
#include "modules/mozart_module_local_music.h"

// keyvalue2str for EV_KEY, ONLY for debug.
char *keyvalue_str[] = {
	[0] = "Released",
	[1] = "Pressed",
};

// keycode2str for EV_KEY, ONLY for debug.
char *keycode_str[] = {
	[KEY_RESERVED] = "KEY_RESERVED",
	[KEY_ESC] = "KEY_ESC",
	[KEY_1] = "KEY_1",
	[KEY_2] = "KEY_2",
	[KEY_3] = "KEY_3",
	[KEY_4] = "KEY_4",
	[KEY_5] = "KEY_5",
	[KEY_6] = "KEY_6",
	[KEY_7] = "KEY_7",
	[KEY_8] = "KEY_8",
	[KEY_9] = "KEY_9",
	[KEY_0] = "KEY_0",
	[KEY_MINUS] = "KEY_MINUS",
	[KEY_EQUAL] = "KEY_EQUAL",
	[KEY_BACKSPACE] = "KEY_BACKSPACE",
	[KEY_TAB] = "KEY_TAB",
	[KEY_Q] = "KEY_Q",
	[KEY_W] = "KEY_W",
	[KEY_E] = "KEY_E",
	[KEY_R] = "KEY_R",
	[KEY_T] = "KEY_T",
	[KEY_Y] = "KEY_Y",
	[KEY_U] = "KEY_U",
	[KEY_I] = "KEY_I",
	[KEY_O] = "KEY_O",
	[KEY_P] = "KEY_P",
	[KEY_LEFTBRACE] = "KEY_LEFTBRACE",
	[KEY_RIGHTBRACE] = "KEY_RIGHTBRACE",
	[KEY_ENTER] = "KEY_ENTER",
	[KEY_LEFTCTRL] = "KEY_LEFTCTRL",
	[KEY_A] = "KEY_A",
	[KEY_S] = "KEY_S",
	[KEY_D] = "KEY_D",
	[KEY_F] = "KEY_F",
	[KEY_G] = "KEY_G",
	[KEY_H] = "KEY_H",
	[KEY_J] = "KEY_J",
	[KEY_K] = "KEY_K",
	[KEY_L] = "KEY_L",
	[KEY_SEMICOLON] = "KEY_SEMICOLON",
	[KEY_APOSTROPHE] = "KEY_APOSTROPHE",
	[KEY_GRAVE] = "KEY_GRAVE",
	[KEY_LEFTSHIFT] = "KEY_LEFTSHIFT",
	[KEY_BACKSLASH] = "KEY_BACKSLASH",
	[KEY_Z] = "KEY_Z",
	[KEY_X] = "KEY_X",
	[KEY_C] = "KEY_C",
	[KEY_V] = "KEY_V",
	[KEY_B] = "KEY_B",
	[KEY_N] = "KEY_N",
	[KEY_M] = "KEY_M",
	[KEY_COMMA] = "KEY_COMMA",
	[KEY_DOT] = "KEY_DOT",
	[KEY_SLASH] = "KEY_SLASH",
	[KEY_RIGHTSHIFT] = "KEY_RIGHTSHIFT",
	[KEY_KPASTERISK] = "KEY_KPASTERISK",
	[KEY_LEFTALT] = "KEY_LEFTALT",
	[KEY_SPACE] = "KEY_SPACE",
	[KEY_CAPSLOCK] = "KEY_CAPSLOCK",
	[KEY_F1] = "KEY_F1",
	[KEY_F2] = "KEY_F2",
	[KEY_F3] = "KEY_F3",
	[KEY_F4] = "KEY_F4",
	[KEY_F5] = "KEY_F5",
	[KEY_F6] = "KEY_F6",
	[KEY_F7] = "KEY_F7",
	[KEY_F8] = "KEY_F8",
	[KEY_F9] = "KEY_F9",
	[KEY_F10] = "KEY_F10",
	[KEY_NUMLOCK] = "KEY_NUMLOCK",
	[KEY_SCROLLLOCK] = "KEY_SCROLLLOCK",
	[KEY_KP7] = "KEY_KP7",
	[KEY_KP8] = "KEY_KP8",
	[KEY_KP9] = "KEY_KP9",
	[KEY_KPMINUS] = "KEY_KPMINUS",
	[KEY_KP4] = "KEY_KP4",
	[KEY_KP5] = "KEY_KP5",
	[KEY_KP6] = "KEY_KP6",
	[KEY_KPPLUS] = "KEY_KPPLUS",
	[KEY_KP1] = "KEY_KP1",
	[KEY_KP2] = "KEY_KP2",
	[KEY_KP3] = "KEY_KP3",
	[KEY_KP0] = "KEY_KP0",
	[KEY_KPDOT] = "KEY_KPDOT",
	[KEY_ZENKAKUHANKAKU] = "KEY_ZENKAKUHANKAKU",
	[KEY_102ND] = "KEY_102ND",
	[KEY_F11] = "KEY_F11",
	[KEY_F12] = "KEY_F12",
	[KEY_RO] = "KEY_RO",
	[KEY_KATAKANA] = "KEY_KATAKANA",
	[KEY_HIRAGANA] = "KEY_HIRAGANA",
	[KEY_HENKAN] = "KEY_HENKAN",
	[KEY_KATAKANAHIRAGANA] = "KEY_KATAKANAHIRAGANA",
	[KEY_MUHENKAN] = "KEY_MUHENKAN",
	[KEY_KPJPCOMMA] = "KEY_KPJPCOMMA",
	[KEY_KPENTER] = "KEY_KPENTER",
	[KEY_RIGHTCTRL] = "KEY_RIGHTCTRL",
	[KEY_KPSLASH] = "KEY_KPSLASH",
	[KEY_SYSRQ] = "KEY_SYSRQ",
	[KEY_RIGHTALT] = "KEY_RIGHTALT",
	[KEY_LINEFEED] = "KEY_LINEFEED",
	[KEY_HOME] = "KEY_HOME",
	[KEY_UP] = "KEY_UP",
	[KEY_PAGEUP] = "KEY_PAGEUP",
	[KEY_LEFT] = "KEY_LEFT",
	[KEY_RIGHT] = "KEY_RIGHT",
	[KEY_END] = "KEY_END",
	[KEY_DOWN] = "KEY_DOWN",
	[KEY_PAGEDOWN] = "KEY_PAGEDOWN",
	[KEY_INSERT] = "KEY_INSERT",
	[KEY_DELETE] = "KEY_DELETE",
	[KEY_MACRO] = "KEY_MACRO",
	[KEY_MUTE] = "KEY_MUTE",
	[KEY_VOLUMEDOWN] = "KEY_VOLUMEDOWN",
	[KEY_VOLUMEUP] = "KEY_VOLUMEUP",
	[KEY_POWER] = "KEY_POWER",
	[KEY_KPEQUAL] = "KEY_KPEQUAL",
	[KEY_KPPLUSMINUS] = "KEY_KPPLUSMINUS",
	[KEY_PAUSE] = "KEY_PAUSE",
	[KEY_SCALE] = "KEY_SCALE",
	[KEY_KPCOMMA] = "KEY_KPCOMMA",
	[KEY_HANGEUL] = "KEY_HANGEUL",
	[KEY_HANGUEL] = "KEY_HANGUEL",
	[KEY_HANJA] = "KEY_HANJA",
	[KEY_YEN] = "KEY_YEN",
	[KEY_LEFTMETA] = "KEY_LEFTMETA",
	[KEY_RIGHTMETA] = "KEY_RIGHTMETA",
	[KEY_COMPOSE] = "KEY_COMPOSE",
	[KEY_STOP] = "KEY_STOP",
	[KEY_AGAIN] = "KEY_AGAIN",
	[KEY_PROPS] = "KEY_PROPS",
	[KEY_UNDO] = "KEY_UNDO",
	[KEY_FRONT] = "KEY_FRONT",
	[KEY_COPY] = "KEY_COPY",
	[KEY_OPEN] = "KEY_OPEN",
	[KEY_PASTE] = "KEY_PASTE",
	[KEY_FIND] = "KEY_FIND",
	[KEY_CUT] = "KEY_CUT",
	[KEY_HELP] = "KEY_HELP",
	[KEY_MENU] = "KEY_MENU",
	[KEY_CALC] = "KEY_CALC",
	[KEY_SETUP] = "KEY_SETUP",
	[KEY_SLEEP] = "KEY_SLEEP",
	[KEY_WAKEUP] = "KEY_WAKEUP",
	[KEY_FILE] = "KEY_FILE",
	[KEY_SENDFILE] = "KEY_SENDFILE",
	[KEY_DELETEFILE] = "KEY_DELETEFILE",
	[KEY_XFER] = "KEY_XFER",
	[KEY_PROG1] = "KEY_PROG1",
	[KEY_PROG2] = "KEY_PROG2",
	[KEY_WWW] = "KEY_WWW",
	[KEY_MSDOS] = "KEY_MSDOS",
	[KEY_COFFEE] = "KEY_COFFEE",
	[KEY_SCREENLOCK] = "KEY_SCREENLOCK",
	[KEY_DIRECTION] = "KEY_DIRECTION",
	[KEY_CYCLEWINDOWS] = "KEY_CYCLEWINDOWS",
	[KEY_MAIL] = "KEY_MAIL",
	[KEY_BOOKMARKS] = "KEY_BOOKMARKS",
	[KEY_COMPUTER] = "KEY_COMPUTER",
	[KEY_BACK] = "KEY_BACK",
	[KEY_FORWARD] = "KEY_FORWARD",
	[KEY_CLOSECD] = "KEY_CLOSECD",
	[KEY_EJECTCD] = "KEY_EJECTCD",
	[KEY_EJECTCLOSECD] = "KEY_EJECTCLOSECD",
	[KEY_NEXTSONG] = "KEY_NEXTSONG",
	[KEY_PLAYPAUSE] = "KEY_PLAYPAUSE",
	[KEY_PREVIOUSSONG] = "KEY_PREVIOUSSONG",
	[KEY_STOPCD] = "KEY_STOPCD",
	[KEY_RECORD] = "KEY_RECORD",
	[KEY_REWIND] = "KEY_REWIND",
	[KEY_PHONE] = "KEY_PHONE",
	[KEY_ISO] = "KEY_ISO",
	[KEY_CONFIG] = "KEY_CONFIG",
	[KEY_HOMEPAGE] = "KEY_HOMEPAGE",
	[KEY_REFRESH] = "KEY_REFRESH",
	[KEY_EXIT] = "KEY_EXIT",
	[KEY_MOVE] = "KEY_MOVE",
	[KEY_EDIT] = "KEY_EDIT",
	[KEY_SCROLLUP] = "KEY_SCROLLUP",
	[KEY_SCROLLDOWN] = "KEY_SCROLLDOWN",
	[KEY_KPLEFTPAREN] = "KEY_KPLEFTPAREN",
	[KEY_KPRIGHTPAREN] = "KEY_KPRIGHTPAREN",
	[KEY_NEW] = "KEY_NEW",
	[KEY_REDO] = "KEY_REDO",
	[KEY_F13] = "KEY_F13",
	[KEY_F14] = "KEY_F14",
	[KEY_F15] = "KEY_F15",
	[KEY_F16] = "KEY_F16",
	[KEY_F17] = "KEY_F17",
	[KEY_F18] = "KEY_F18",
	[KEY_F19] = "KEY_F19",
	[KEY_F20] = "KEY_F20",
	[KEY_F21] = "KEY_F21",
	[KEY_F22] = "KEY_F22",
	[KEY_F23] = "KEY_F23",
	[KEY_F24] = "KEY_F24",
	[KEY_PLAYCD] = "KEY_PLAYCD",
	[KEY_PAUSECD] = "KEY_PAUSECD",
	[KEY_PROG3] = "KEY_PROG3",
	[KEY_PROG4] = "KEY_PROG4",
	[KEY_DASHBOARD] = "KEY_DASHBOARD",
	[KEY_SUSPEND] = "KEY_SUSPEND",
	[KEY_CLOSE] = "KEY_CLOSE",
	[KEY_PLAY] = "KEY_PLAY",
	[KEY_FASTFORWARD] = "KEY_FASTFORWARD",
	[KEY_BASSBOOST] = "KEY_BASSBOOST",
	[KEY_PRINT] = "KEY_PRINT",
	[KEY_HP] = "KEY_HP",
	[KEY_CAMERA] = "KEY_CAMERA",
	[KEY_SOUND] = "KEY_SOUND",
	[KEY_QUESTION] = "KEY_QUESTION",
	[KEY_EMAIL] = "KEY_EMAIL",
	[KEY_CHAT] = "KEY_CHAT",
	[KEY_SEARCH] = "KEY_SEARCH",
	[KEY_CONNECT] = "KEY_CONNECT",
	[KEY_FINANCE] = "KEY_FINANCE",
	[KEY_SPORT] = "KEY_SPORT",
	[KEY_SHOP] = "KEY_SHOP",
	[KEY_ALTERASE] = "KEY_ALTERASE",
	[KEY_CANCEL] = "KEY_CANCEL",
	[KEY_BRIGHTNESSDOWN] = "KEY_BRIGHTNESSDOWN",
	[KEY_BRIGHTNESSUP] = "KEY_BRIGHTNESSUP",
	[KEY_MEDIA] = "KEY_MEDIA",
	[KEY_SWITCHVIDEOMODE] = "KEY_SWITCHVIDEOMODE",
	[KEY_KBDILLUMTOGGLE] = "KEY_KBDILLUMTOGGLE",
	[KEY_KBDILLUMDOWN] = "KEY_KBDILLUMDOWN",
	[KEY_KBDILLUMUP] = "KEY_KBDILLUMUP",
	[KEY_SEND] = "KEY_SEND",
	[KEY_REPLY] = "KEY_REPLY",
	[KEY_FORWARDMAIL] = "KEY_FORWARDMAIL",
	[KEY_SAVE] = "KEY_SAVE",
	[KEY_DOCUMENTS] = "KEY_DOCUMENTS",
	[KEY_BATTERY] = "KEY_BATTERY",
	[KEY_BLUETOOTH] = "KEY_BLUETOOTH",
	[KEY_WLAN] = "KEY_WLAN",
	[KEY_UWB] = "KEY_UWB",
	[KEY_UNKNOWN] = "KEY_UNKNOWN",
	[KEY_VIDEO_NEXT] = "KEY_VIDEO_NEXT",
	[KEY_VIDEO_PREV] = "KEY_VIDEO_PREV",
	[KEY_BRIGHTNESS_CYCLE] = "KEY_BRIGHTNESS_CYCLE",
	[KEY_BRIGHTNESS_ZERO] = "KEY_BRIGHTNESS_ZERO",
	[KEY_DISPLAY_OFF] = "KEY_DISPLAY_OFF",
	[KEY_WIMAX] = "KEY_WIMAX",
	[KEY_RFKILL] = "KEY_RFKILL",
	[BTN_MISC] = "BTN_MISC",
	[BTN_0] = "BTN_0",
	[BTN_1] = "BTN_1",
	[BTN_2] = "BTN_2",
	[BTN_3] = "BTN_3",
	[BTN_4] = "BTN_4",
	[BTN_5] = "BTN_5",
	[BTN_6] = "BTN_6",
	[BTN_7] = "BTN_7",
	[BTN_8] = "BTN_8",
	[BTN_9] = "BTN_9",
	[BTN_MOUSE] = "BTN_MOUSE",
	[BTN_LEFT] = "BTN_LEFT",
	[BTN_RIGHT] = "BTN_RIGHT",
	[BTN_MIDDLE] = "BTN_MIDDLE",
	[BTN_SIDE] = "BTN_SIDE",
	[BTN_EXTRA] = "BTN_EXTRA",
	[BTN_FORWARD] = "BTN_FORWARD",
	[BTN_BACK] = "BTN_BACK",
	[BTN_TASK] = "BTN_TASK",
	[BTN_JOYSTICK] = "BTN_JOYSTICK",
	[BTN_TRIGGER] = "BTN_TRIGGER",
	[BTN_THUMB] = "BTN_THUMB",
	[BTN_THUMB2] = "BTN_THUMB2",
	[BTN_TOP] = "BTN_TOP",
	[BTN_TOP2] = "BTN_TOP2",
	[BTN_PINKIE] = "BTN_PINKIE",
	[BTN_BASE] = "BTN_BASE",
	[BTN_BASE2] = "BTN_BASE2",
	[BTN_BASE3] = "BTN_BASE3",
	[BTN_BASE4] = "BTN_BASE4",
	[BTN_BASE5] = "BTN_BASE5",
	[BTN_BASE6] = "BTN_BASE6",
	[BTN_DEAD] = "BTN_DEAD",
	[BTN_GAMEPAD] = "BTN_GAMEPAD",
	[BTN_A] = "BTN_A",
	[BTN_B] = "BTN_B",
	[BTN_C] = "BTN_C",
	[BTN_X] = "BTN_X",
	[BTN_Y] = "BTN_Y",
	[BTN_Z] = "BTN_Z",
	[BTN_TL] = "BTN_TL",
	[BTN_TR] = "BTN_TR",
	[BTN_TL2] = "BTN_TL2",
	[BTN_TR2] = "BTN_TR2",
	[BTN_SELECT] = "BTN_SELECT",
	[BTN_START] = "BTN_START",
	[BTN_MODE] = "BTN_MODE",
	[BTN_THUMBL] = "BTN_THUMBL",
	[BTN_THUMBR] = "BTN_THUMBR",
	[BTN_DIGI] = "BTN_DIGI",
	[BTN_TOOL_PEN] = "BTN_TOOL_PEN",
	[BTN_TOOL_RUBBER] = "BTN_TOOL_RUBBER",
	[BTN_TOOL_BRUSH] = "BTN_TOOL_BRUSH",
	[BTN_TOOL_PENCIL] = "BTN_TOOL_PENCIL",
	[BTN_TOOL_AIRBRUSH] = "BTN_TOOL_AIRBRUSH",
	[BTN_TOOL_FINGER] = "BTN_TOOL_FINGER",
	[BTN_TOOL_MOUSE] = "BTN_TOOL_MOUSE",
	[BTN_TOOL_LENS] = "BTN_TOOL_LENS",
	[BTN_TOUCH] = "BTN_TOUCH",
	[BTN_STYLUS] = "BTN_STYLUS",
	[BTN_STYLUS2] = "BTN_STYLUS2",
	[BTN_TOOL_DOUBLETAP] = "BTN_TOOL_DOUBLETAP",
	[BTN_TOOL_TRIPLETAP] = "BTN_TOOL_TRIPLETAP",
	[BTN_TOOL_QUADTAP] = "BTN_TOOL_QUADTAP",
	[BTN_WHEEL] = "BTN_WHEEL",
	[BTN_GEAR_DOWN] = "BTN_GEAR_DOWN",
	[BTN_GEAR_UP] = "BTN_GEAR_UP",
	[KEY_OK] = "KEY_OK",
	[KEY_SELECT] = "KEY_SELECT",
	[KEY_GOTO] = "KEY_GOTO",
	[KEY_CLEAR] = "KEY_CLEAR",
	[KEY_POWER2] = "KEY_POWER2",
	[KEY_OPTION] = "KEY_OPTION",
	[KEY_INFO] = "KEY_INFO",
	[KEY_TIME] = "KEY_TIME",
	[KEY_VENDOR] = "KEY_VENDOR",
	[KEY_ARCHIVE] = "KEY_ARCHIVE",
	[KEY_PROGRAM] = "KEY_PROGRAM",
	[KEY_CHANNEL] = "KEY_CHANNEL",
	[KEY_FAVORITES] = "KEY_FAVORITES",
	[KEY_EPG] = "KEY_EPG",
	[KEY_PVR] = "KEY_PVR",
	[KEY_MHP] = "KEY_MHP",
	[KEY_LANGUAGE] = "KEY_LANGUAGE",
	[KEY_TITLE] = "KEY_TITLE",
	[KEY_SUBTITLE] = "KEY_SUBTITLE",
	[KEY_ANGLE] = "KEY_ANGLE",
	[KEY_ZOOM] = "KEY_ZOOM",
	[KEY_MODE] = "KEY_MODE",
	[KEY_KEYBOARD] = "KEY_KEYBOARD",
	[KEY_SCREEN] = "KEY_SCREEN",
	[KEY_PC] = "KEY_PC",
	[KEY_TV] = "KEY_TV",
	[KEY_TV2] = "KEY_TV2",
	[KEY_VCR] = "KEY_VCR",
	[KEY_VCR2] = "KEY_VCR2",
	[KEY_SAT] = "KEY_SAT",
	[KEY_SAT2] = "KEY_SAT2",
	[KEY_CD] = "KEY_CD",
	[KEY_TAPE] = "KEY_TAPE",
	[KEY_RADIO] = "KEY_RADIO",
	[KEY_TUNER] = "KEY_TUNER",
	[KEY_PLAYER] = "KEY_PLAYER",
	[KEY_TEXT] = "KEY_TEXT",
	[KEY_DVD] = "KEY_DVD",
	[KEY_AUX] = "KEY_AUX",
	[KEY_MP3] = "KEY_MP3",
	[KEY_AUDIO] = "KEY_AUDIO",
	[KEY_VIDEO] = "KEY_VIDEO",
	[KEY_DIRECTORY] = "KEY_DIRECTORY",
	[KEY_LIST] = "KEY_LIST",
	[KEY_MEMO] = "KEY_MEMO",
	[KEY_CALENDAR] = "KEY_CALENDAR",
	[KEY_RED] = "KEY_RED",
	[KEY_GREEN] = "KEY_GREEN",
	[KEY_YELLOW] = "KEY_YELLOW",
	[KEY_BLUE] = "KEY_BLUE",
	[KEY_CHANNELUP] = "KEY_CHANNELUP",
	[KEY_CHANNELDOWN] = "KEY_CHANNELDOWN",
	[KEY_FIRST] = "KEY_FIRST",
	[KEY_LAST] = "KEY_LAST",
	[KEY_AB] = "KEY_AB",
	[KEY_NEXT] = "KEY_NEXT",
	[KEY_RESTART] = "KEY_RESTART",
	[KEY_SLOW] = "KEY_SLOW",
	[KEY_SHUFFLE] = "KEY_SHUFFLE",
	[KEY_BREAK] = "KEY_BREAK",
	[KEY_PREVIOUS] = "KEY_PREVIOUS",
	[KEY_DIGITS] = "KEY_DIGITS",
	[KEY_TEEN] = "KEY_TEEN",
	[KEY_TWEN] = "KEY_TWEN",
	[KEY_VIDEOPHONE] = "KEY_VIDEOPHONE",
	[KEY_GAMES] = "KEY_GAMES",
	[KEY_ZOOMIN] = "KEY_ZOOMIN",
	[KEY_ZOOMOUT] = "KEY_ZOOMOUT",
	[KEY_ZOOMRESET] = "KEY_ZOOMRESET",
	[KEY_WORDPROCESSOR] = "KEY_WORDPROCESSOR",
	[KEY_EDITOR] = "KEY_EDITOR",
	[KEY_SPREADSHEET] = "KEY_SPREADSHEET",
	[KEY_GRAPHICSEDITOR] = "KEY_GRAPHICSEDITOR",
	[KEY_PRESENTATION] = "KEY_PRESENTATION",
	[KEY_DATABASE] = "KEY_DATABASE",
	[KEY_NEWS] = "KEY_NEWS",
	[KEY_VOICEMAIL] = "KEY_VOICEMAIL",
	[KEY_ADDRESSBOOK] = "KEY_ADDRESSBOOK",
	[KEY_MESSENGER] = "KEY_MESSENGER",
	[KEY_DISPLAYTOGGLE] = "KEY_DISPLAYTOGGLE",
	[KEY_SPELLCHECK] = "KEY_SPELLCHECK",
	[KEY_LOGOFF] = "KEY_LOGOFF",
	[KEY_DOLLAR] = "KEY_DOLLAR",
	[KEY_EURO] = "KEY_EURO",
	[KEY_FRAMEBACK] = "KEY_FRAMEBACK",
	[KEY_FRAMEFORWARD] = "KEY_FRAMEFORWARD",
	[KEY_CONTEXT_MENU] = "KEY_CONTEXT_MENU",
	[KEY_MEDIA_REPEAT] = "KEY_MEDIA_REPEAT",
	[KEY_10CHANNELSUP] = "KEY_10CHANNELSUP",
	[KEY_10CHANNELSDOWN] = "KEY_10CHANNELSDOWN",
	[KEY_IMAGES] = "KEY_IMAGES",
	[KEY_DEL_EOL] = "KEY_DEL_EOL",
	[KEY_DEL_EOS] = "KEY_DEL_EOS",
	[KEY_INS_LINE] = "KEY_INS_LINE",
	[KEY_DEL_LINE] = "KEY_DEL_LINE",
	[KEY_FN] = "KEY_FN",
	[KEY_FN_ESC] = "KEY_FN_ESC",
	[KEY_FN_F1] = "KEY_FN_F1",
	[KEY_FN_F2] = "KEY_FN_F2",
	[KEY_FN_F3] = "KEY_FN_F3",
	[KEY_FN_F4] = "KEY_FN_F4",
	[KEY_FN_F5] = "KEY_FN_F5",
	[KEY_FN_F6] = "KEY_FN_F6",
	[KEY_FN_F7] = "KEY_FN_F7",
	[KEY_FN_F8] = "KEY_FN_F8",
	[KEY_FN_F9] = "KEY_FN_F9",
	[KEY_FN_F10] = "KEY_FN_F10",
	[KEY_FN_F11] = "KEY_FN_F11",
	[KEY_FN_F12] = "KEY_FN_F12",
	[KEY_FN_1] = "KEY_FN_1",
	[KEY_FN_2] = "KEY_FN_2",
	[KEY_FN_D] = "KEY_FN_D",
	[KEY_FN_E] = "KEY_FN_E",
	[KEY_FN_F] = "KEY_FN_F",
	[KEY_FN_S] = "KEY_FN_S",
	[KEY_FN_B] = "KEY_FN_B",
	[KEY_BRL_DOT1] = "KEY_BRL_DOT1",
	[KEY_BRL_DOT2] = "KEY_BRL_DOT2",
	[KEY_BRL_DOT3] = "KEY_BRL_DOT3",
	[KEY_BRL_DOT4] = "KEY_BRL_DOT4",
	[KEY_BRL_DOT5] = "KEY_BRL_DOT5",
	[KEY_BRL_DOT6] = "KEY_BRL_DOT6",
	[KEY_BRL_DOT7] = "KEY_BRL_DOT7",
	[KEY_BRL_DOT8] = "KEY_BRL_DOT8",
	[KEY_BRL_DOT9] = "KEY_BRL_DOT9",
	[KEY_BRL_DOT10] = "KEY_BRL_DOT10",
	[KEY_NUMERIC_0] = "KEY_NUMERIC_0",
	[KEY_NUMERIC_1] = "KEY_NUMERIC_1",
	[KEY_NUMERIC_2] = "KEY_NUMERIC_2",
	[KEY_NUMERIC_3] = "KEY_NUMERIC_3",
	[KEY_NUMERIC_4] = "KEY_NUMERIC_4",
	[KEY_NUMERIC_5] = "KEY_NUMERIC_5",
	[KEY_NUMERIC_6] = "KEY_NUMERIC_6",
	[KEY_NUMERIC_7] = "KEY_NUMERIC_7",
	[KEY_NUMERIC_8] = "KEY_NUMERIC_8",
	[KEY_NUMERIC_9] = "KEY_NUMERIC_9",
	[KEY_NUMERIC_STAR] = "KEY_NUMERIC_STAR",
	[KEY_NUMERIC_POUND] = "KEY_NUMERIC_POUND",
	[KEY_CAMERA_FOCUS] = "KEY_CAMERA_FOCUS",
	[KEY_WPS_BUTTON] = "KEY_WPS_BUTTON",
	[KEY_TOUCHPAD_TOGGLE] = "KEY_TOUCHPAD_TOGGLE",
	[KEY_TOUCHPAD_ON] = "KEY_TOUCHPAD_ON",
	[KEY_TOUCHPAD_OFF] = "KEY_TOUCHPAD_OFF",
	[KEY_CAMERA_ZOOMIN] = "KEY_CAMERA_ZOOMIN",
	[KEY_CAMERA_ZOOMOUT] = "KEY_CAMERA_ZOOMOUT",
	[KEY_CAMERA_UP] = "KEY_CAMERA_UP",
	[KEY_CAMERA_DOWN] = "KEY_CAMERA_DOWN",
	[KEY_CAMERA_LEFT] = "KEY_CAMERA_LEFT",
	[KEY_CAMERA_RIGHT] = "KEY_CAMERA_RIGHT",
	[BTN_TRIGGER_HAPPY] = "BTN_TRIGGER_HAPPY",
	[BTN_TRIGGER_HAPPY1] = "BTN_TRIGGER_HAPPY1",
	[BTN_TRIGGER_HAPPY2] = "BTN_TRIGGER_HAPPY2",
	[BTN_TRIGGER_HAPPY3] = "BTN_TRIGGER_HAPPY3",
	[BTN_TRIGGER_HAPPY4] = "BTN_TRIGGER_HAPPY4",
	[BTN_TRIGGER_HAPPY5] = "BTN_TRIGGER_HAPPY5",
	[BTN_TRIGGER_HAPPY6] = "BTN_TRIGGER_HAPPY6",
	[BTN_TRIGGER_HAPPY7] = "BTN_TRIGGER_HAPPY7",
	[BTN_TRIGGER_HAPPY8] = "BTN_TRIGGER_HAPPY8",
	[BTN_TRIGGER_HAPPY9] = "BTN_TRIGGER_HAPPY9",
	[BTN_TRIGGER_HAPPY10] = "BTN_TRIGGER_HAPPY10",
	[BTN_TRIGGER_HAPPY11] = "BTN_TRIGGER_HAPPY11",
	[BTN_TRIGGER_HAPPY12] = "BTN_TRIGGER_HAPPY12",
	[BTN_TRIGGER_HAPPY13] = "BTN_TRIGGER_HAPPY13",
	[BTN_TRIGGER_HAPPY14] = "BTN_TRIGGER_HAPPY14",
	[BTN_TRIGGER_HAPPY15] = "BTN_TRIGGER_HAPPY15",
	[BTN_TRIGGER_HAPPY16] = "BTN_TRIGGER_HAPPY16",
	[BTN_TRIGGER_HAPPY17] = "BTN_TRIGGER_HAPPY17",
	[BTN_TRIGGER_HAPPY18] = "BTN_TRIGGER_HAPPY18",
	[BTN_TRIGGER_HAPPY19] = "BTN_TRIGGER_HAPPY19",
	[BTN_TRIGGER_HAPPY20] = "BTN_TRIGGER_HAPPY20",
	[BTN_TRIGGER_HAPPY21] = "BTN_TRIGGER_HAPPY21",
	[BTN_TRIGGER_HAPPY22] = "BTN_TRIGGER_HAPPY22",
	[BTN_TRIGGER_HAPPY23] = "BTN_TRIGGER_HAPPY23",
	[BTN_TRIGGER_HAPPY24] = "BTN_TRIGGER_HAPPY24",
	[BTN_TRIGGER_HAPPY25] = "BTN_TRIGGER_HAPPY25",
	[BTN_TRIGGER_HAPPY26] = "BTN_TRIGGER_HAPPY26",
	[BTN_TRIGGER_HAPPY27] = "BTN_TRIGGER_HAPPY27",
	[BTN_TRIGGER_HAPPY28] = "BTN_TRIGGER_HAPPY28",
	[BTN_TRIGGER_HAPPY29] = "BTN_TRIGGER_HAPPY29",
	[BTN_TRIGGER_HAPPY30] = "BTN_TRIGGER_HAPPY30",
	[BTN_TRIGGER_HAPPY31] = "BTN_TRIGGER_HAPPY31",
	[BTN_TRIGGER_HAPPY32] = "BTN_TRIGGER_HAPPY32",
	[BTN_TRIGGER_HAPPY33] = "BTN_TRIGGER_HAPPY33",
	[BTN_TRIGGER_HAPPY34] = "BTN_TRIGGER_HAPPY34",
	[BTN_TRIGGER_HAPPY35] = "BTN_TRIGGER_HAPPY35",
	[BTN_TRIGGER_HAPPY36] = "BTN_TRIGGER_HAPPY36",
	[BTN_TRIGGER_HAPPY37] = "BTN_TRIGGER_HAPPY37",
	[BTN_TRIGGER_HAPPY38] = "BTN_TRIGGER_HAPPY38",
	[BTN_TRIGGER_HAPPY39] = "BTN_TRIGGER_HAPPY39",
	[BTN_TRIGGER_HAPPY40] = "BTN_TRIGGER_HAPPY40",
	[KEY_MAX] = "ERROR_KEY",
};




const char *snd_source_str[] = {
#ifdef SUPPORT_SONG_SUPPLYER
	[SND_SRC_CLOUD] = "cloud_music",
#endif
#if (SUPPORT_BT == BT_BCM)
	[SND_SRC_BT_AVK] = "bluetooth",
#endif
#if (SUPPORT_LOCALPLAYER == 1)
	[SND_SRC_LOCALPLAY] = "localplay",
#endif
#if (SUPPORT_INGENICPLAYER == 1)
	[SND_SRC_INGENICPLAYER] = "ingenicplayer",
#endif
	[SND_SRC_LINEIN] = "linein",
};

snd_source_t snd_source = SND_SRC_NONE;

#if (SUPPORT_BT == BT_BCM)
pthread_t bt_mode_pthread;
pthread_mutex_t bt_mode_lock;
extern int bluetooth_create_auto_reconnect_pthread();
extern int bluetooth_cancel_auto_reconnect_pthread();
#endif

void mozart_wifi_mode(void)
{
	wifi_ctl_msg_t new_mode;
	wifi_info_t infor = get_wifi_mode();

	memset(&new_mode, 0, sizeof(wifi_ctl_msg_t));
	new_mode.force = true;

	if (infor.wifi_mode == STA || infor.wifi_mode == STANET || infor.wifi_mode == STA_WAIT){
		new_mode.cmd = SW_AP;
	}
	else if (infor.wifi_mode == AP || infor.wifi_mode == AP_WAIT){
		new_mode.cmd = SW_STA;
		new_mode.param.switch_sta.sta_timeout = -1;
	}
	else if (infor.wifi_mode == AIRKISS || infor.wifi_mode == WIFI_NULL){
		new_mode.cmd = SW_AP;
	}
	else {
		new_mode.cmd = SW_AP;
	}

	strcpy(new_mode.name, app_name);
	if(request_wifi_mode(new_mode) != true)
		printf("ERROR: [%s] Request Network Failed, Please Register First!!!!\n", app_name);
}

void mozart_config_wifi(void)
{
	wifi_ctl_msg_t new_mode;
	new_mode.force = true;

	memset(&new_mode, 0, sizeof(wifi_ctl_msg_t));
	new_mode.cmd = SW_NETCFG;
	strcpy(new_mode.name, app_name);
	new_mode.param.network_config.timeout = -1;
	strcpy(new_mode.param.network_config.product_model, "ALINKTEST_LIVING_LIGHT_SMARTLED");
	strcpy(new_mode.param.network_config.wl_module, wifi_module_str[BROADCOM]);
	new_mode.param.network_config.method |= COOEE;
	if(request_wifi_mode(new_mode) != true)
		printf("ERROR: [%s] Request Network Failed, Please Register First!!!!\n", app_name);
}

#if (SUPPORT_BT == BT_BCM)
static void *bluetooth_mode_pthread(void *args)
{
	int num = 600;

	pthread_mutex_lock(&bt_mode_lock);
	if (BT_LINK_CONNECTED != mozart_bluetooth_get_link_status()) {
		bluetooth_create_auto_reconnect_pthread();
		while ((BT_LINK_CONNECTED != mozart_bluetooth_get_link_status()) && (num > 0)) {
			num--;
			sleep(1);
		}
	}
	if ((BT_LINK_CONNECTED != mozart_bluetooth_get_link_status()) || (num <= 0)) {
		printf("bluetooth reconnect failed !\n");
		pthread_mutex_unlock(&bt_mode_lock);
		return NULL; // make compile happy.
	}
	printf("mozart_bluetooth_avk_start_play !\n");
	mozart_bluetooth_avk_start_play();
	pthread_mutex_unlock(&bt_mode_lock);

	return NULL; // make compile happy.
}
#endif /* SUPPORT_BT == BT_BCM */

static int mozart_check_dsp(void)
{
#ifdef SUPPORT_AUDIO_OSS
	int cnt = 500;

	while (cnt--) {
		if (check_dsp_opt(O_WRONLY))
			break;
		usleep(10 * 1000);
	}
	if (cnt < 0) {
		printf("[%s, %d]: /dev/dsp is busy\n", __func__, __LINE__);
		return -1;
	}
#endif
	return 0;
}

void mozart_previous_song(void)
{
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_prev_music(mozart_musicplayer_handler);
		break;
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
		if (lapsule_is_controler()) {
			if (lapsule_do_prev_song())
				printf("lapsule_do_prev_song failure.\n");
		} else if (qplay_is_controler()) {
			mozart_render_prev_song();
		} else {
			printf("Do not support Pre song on normal DLNA mode in %s:%s:%d.\n",
			       __FILE__, __func__, __LINE__);
		}
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		mozart_bluetooth_avk_prev_music();
		break;
#endif
	case UNKNOWN_DOMAIN:
		printf("[%s, %d]: idle mode\n", __func__, __LINE__);
		break;
	default:
		if (domain > 0 && domain < MAX_DOMAIN)
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
		else
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
		break;
	}
}

void mozart_next_song(void)
{
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_next_music(mozart_musicplayer_handler);
		break;
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
		if (lapsule_is_controler()) {
			if (lapsule_do_next_song())
				printf("lapsule_do_next_song failure.\n");
		} else if (qplay_is_controler()) {
			mozart_render_next_song();
		} else {
			printf("Do not support Next song on normal DLNA mode in %s:%s:%d.\n",
			       __FILE__, __func__, __LINE__);
		}
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		mozart_bluetooth_avk_next_music();
		break;
#endif
	case UNKNOWN_DOMAIN:
		printf("[%s, %d]: idle mode\n", __func__, __LINE__);
		break;
	default:
		if (domain > 0 && domain < MAX_DOMAIN)
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
		else
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
		break;
	}
}

enum mozart_module_play_status mozart_module_get_play_status(void)
{
	int ret = -1;
	memory_domain domain;
	module_status status;
	enum mozart_module_play_status play_status = mozart_module_status_stop;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return 0;
	}

	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		if (mozart_musicplayer_get_status(mozart_musicplayer_handler) == PLAYER_PAUSED)
			play_status = mozart_module_status_pause;
		else
			play_status = mozart_module_status_play;
		break;
#if (SUPPORT_AIRPLAY == 1)
	case AIRPLAY_DOMAIN:
		if (share_mem_get(AIRPLAY_DOMAIN, &status)) {
			printf("[%s, %d]share_mem_get failed.", __func__, __LINE__);
			return -1;
		}
		if (STATUS_PLAYING == status)
			play_status = mozart_module_status_play;
		else if (STATUS_PAUSE == status)
			play_status = mozart_module_status_pause;
		break;
#endif
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
		if (share_mem_get(RENDER_DOMAIN, &status)) {
			printf("[%s, %d]share_mem_get failed.", __func__, __LINE__);
			return -1;
		}
		if (STATUS_PLAYING == status)
			play_status = mozart_module_status_play;
		else if (STATUS_PAUSE == status)
			play_status = mozart_module_status_pause;
                break;
#endif
#ifdef SUPPORT_BT
	case BT_HS_DOMAIN:
		printf("bt priority is highest, WILL NOT play_status in %s:%d.\n", __func__, __LINE__);
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		if (share_mem_get(BT_AVK_DOMAIN, &status)) {
			printf("[%s, %d]share_mem_get failed.", __func__, __LINE__);
			return -1;
		}
		if (STATUS_PLAYING == status)
			play_status = mozart_module_status_play;
		else if (STATUS_PAUSE == status)
			play_status = mozart_module_status_pause;
		break;
#endif
	case UNKNOWN_DOMAIN:
		printf("[%s, %d]: idle mode\n", __func__, __LINE__);
		break;
	default:
		if (domain > 0 && domain < MAX_DOMAIN)
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
		else
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
		play_status = mozart_module_status_stop;
		break;
	}

	return play_status;
}

void mozart_play_pause(void)
{
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}
printf("mozart_play_pause:source :%d domain:%d",snd_source,domain);
	switch (domain) {
	case UNKNOWN_DOMAIN:
		printf("system is in idle mode in %s:%s:%d.\n",
		       __FILE__, __func__, __LINE__);
#if (SUPPORT_LOCALPLAYER == 1)
		if (snd_source != SND_SRC_LINEIN) {
			printf("start localplayer playback...\n");
			mozart_module_local_music_startup();
		}
#endif
		break;
	case MUSICPLAYER_DOMAIN:
#ifdef SUPPORT_VR
		if (mozart_vr_get_status() == VR_ASR) {
			printf("ASR mode, interrupt it.\n");
			mozart_vr_asr_break();
		}
#endif
		mozart_musicplayer_play_pause(mozart_musicplayer_handler);
		break;
#if (SUPPORT_AIRPLAY == 1)
	case AIRPLAY_DOMAIN:
		mozart_airplay_play_pause();
		return;
#endif
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
		if (lapsule_is_controler()) {
			if (lapsule_do_toggle())
				printf("lapsule_do_toggle failure.\n");
		} else {
			if (mozart_render_play_pause())
				printf("play/pause render playback error in %s:%s:%d\n",
				       __FILE__, __func__, __LINE__);
		}
		break;
#endif
#ifdef SUPPORT_BT
	case BT_HS_DOMAIN:
		printf("bt priority is highest, WILL NOT play/pause in %s:%d.\n", __func__, __LINE__);
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		mozart_bluetooth_avk_play_pause();
		break;
#endif
	default:
		if (domain > 0 && domain < MAX_DOMAIN)
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
		else
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
		break;
	}
}

int mozart_module_pause(void)
{
	int ret = -1;
	int i = 100;
	memory_domain domain;
	module_status status;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return 0;
	}
printf("mozart_module_pause:source :%d domain:%d",snd_source,domain);
	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		if (mozart_musicplayer_get_status(mozart_musicplayer_handler) == PLAYER_PLAYING)
			mozart_musicplayer_play_pause(mozart_musicplayer_handler);
		while (i--) {
			if (PLAYER_PAUSED == mozart_musicplayer_get_status(mozart_musicplayer_handler))
				break;
			usleep(10);
		}
		if (i == 0)
			printf("pause musicplayer timeout(>1s).\n");
		break;
#if (SUPPORT_AIRPLAY == 1)
	case AIRPLAY_DOMAIN:
		if (share_mem_get(AIRPLAY_DOMAIN, &status)) {
			printf("[%s, %d]share_mem_get failed.", __func__, __LINE__);
			return -1;
		}
		if (STATUS_PLAYING == status)
			mozart_airplay_play_pause();
		break;
#endif
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
		if (0 != share_mem_get(RENDER_DOMAIN, &status)) {
			printf("[%s, %d]share_mem_get failed.", __func__, __LINE__);
			return -1;
		}
		if (STATUS_PLAYING == status) {
			if (lapsule_is_controler()) {
				if (lapsule_do_toggle())
					printf("lapsule_do_toggle failure.\n");
			} else {
				if (mozart_render_play_pause())
					printf("play/pause render playback error in %s:%s:%d\n",
					       __FILE__, __func__, __LINE__);
			}
		}
                break;
#endif
#ifdef SUPPORT_BT
	case BT_HS_DOMAIN:
		printf("bt priority is highest, WILL NOT play/pause in %s:%d.\n", __func__, __LINE__);
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		if (share_mem_get(BT_AVK_DOMAIN, &status)) {
			printf("[%s, %d]share_mem_get failed.", __func__, __LINE__);
			return -1;
		}
		if (STATUS_PLAYING == status)
			mozart_bluetooth_avk_play_pause();
		break;
#endif
	case UNKNOWN_DOMAIN:
		printf("[%s, %d]: idle mode\n", __func__, __LINE__);
		break;
	default:
		if (domain > 0 && domain < MAX_DOMAIN)
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
		else
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
		break;
	}
	return 0;
}

int mozart_module_stop(void)
{
	printf("************this is success enter mozart_module_stop*************\n");
	int ret = 0;
	memory_domain domain;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return -1;
	}
	printf("mozart_module_stop:source :%d domain:%d\n",snd_source,domain);
#if 1
#if (SUPPORT_BT == BT_BCM)
	if (snd_source == SND_SRC_BT_AVK)
		{
		bluetooth_cancel_auto_reconnect_pthread();
		}
#endif
#endif
	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_stop(mozart_musicplayer_handler);
		ret = 0;
		break;
#if (SUPPORT_AIRPLAY == 1)
	case AIRPLAY_DOMAIN:
		ret = mozart_airplay_stop_playback();
		break;
#endif
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
		if (lapsule_is_controler()) {
			if (lapsule_do_stop_play())
				printf("lapsule_do_toggle failure.\n");
		}
		ret = mozart_render_stop_playback();
		break;
#endif
#if 1
#ifdef SUPPORT_BT
	case BT_HS_DOMAIN:
		printf("bt priority is highest, do nothing in %s:%d.\n", __func__, __LINE__);
		ret = -1;
		break;
#endif

#if (SUPPORT_BT == BT_BCM)
	case BT_AVK_DOMAIN:
		// FIXME: avk_stop_play() no return value.
		mozart_bluetooth_avk_stop_play();
		printf("[%s, %d]: BT_AVK_STOP\n", __func__, __LINE__);
		ret = 0;
		break;
#endif
#endif
	case UNKNOWN_DOMAIN:
		if (snd_source == SND_SRC_LINEIN && mozart_linein_is_in())
			mozart_linein_off();
		else
			printf("[%s, %d]: idle mode\n", __func__, __LINE__);
		return 0;
	default:
		if (domain > 0 && domain < MAX_DOMAIN) {
			printf("[%s, %d]: %s domain is active\n", __func__, __LINE__,
			       memory_domain_str[domain]);
			ret = 0;
		} else {
			printf("[%s, %d]: Not support domain is active\n", __func__, __LINE__);
			ret = -1;
		}
		break;
	}

#if 1
#if (SUPPORT_BT == BT_BCM)
	if (snd_source == SND_SRC_BT_AVK)
		{
		printf("------------------------------------------------------------------");
		mozart_bluetooth_avk_stop_service();
               mozart_bluetooth_set_visibility(false, false);
		}
#endif
#endif

	if (ret == 0)
		ret = mozart_check_dsp();

	return ret;
}

void mozart_snd_source_switch(void)
{
	snd_source_t source = snd_source;
      printf("\n\n%d\n\n",snd_source);
	mozart_module_stop();

	// find the next snd source.
	while (1) {
		source = (source + 1) % 2;//SND_SRC_MAX;
#ifdef SUPPORT_SONG_SUPPLYER
		if (source == SND_SRC_CLOUD && get_wifi_mode().wifi_mode != STA)
			continue;
#endif

#if (SUPPORT_INGENICPLAYER == 1)
		if (source == SND_SRC_INGENICPLAYER)
			continue;
#endif
		if (source == SND_SRC_LINEIN && !mozart_linein_is_in())
			continue;
		break;
	}
      printf("\n\n%d\n\n",snd_source);
	if (mozart_check_dsp() < 0)
		return ;

	printf("switching to %s mode.\n\n", snd_source_str[source]);
	switch (source) {
#ifdef SUPPORT_SONG_SUPPLYER
	case SND_SRC_CLOUD:
		mozart_module_cloud_music_startup();
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case SND_SRC_BT_AVK:
		avk_start();
		mozart_ui_bt_avk_play();
		mozart_play_key_sync("bluetooth_mode");
		if (pthread_create(&bt_mode_pthread, NULL, bluetooth_mode_pthread, NULL) != 0) {
			printf("create bluetooth_mode_pthread failed !\n");
			break;
		}

		pthread_detach(bt_mode_pthread);
		snd_source = source;
		break;
#endif

#if (SUPPORT_LOCALPLAYER == 1)
	case SND_SRC_LOCALPLAY:
		
               
		mozart_play_key("native_mode") ;      //进入本地播放模式
		mozart_module_local_music_startup();
		mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);
		mozart_start_mode(FREE);
		break;
#endif
	case SND_SRC_LINEIN:
		mozart_linein_on();
		snd_source = source;
		break;
	default:
		printf("Unknow snd source(id: %d).\n", snd_source);
		printf("force to LOCALPLAYE.\n");
		
              mozart_play_key("native_mode");	//进入本地播放模式
		mozart_module_local_music_startup();	
		mozart_musicplayer_musiclist_clean(mozart_musicplayer_handler);
		mozart_start_mode(FREE);
		break;
	}
	return;
}

//增大音量
void mozart_volume_up(int i)
{
	int vol = 0;
	char vol_buf[8] = {};
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	if (mozart_ini_getkey("/usr/data/system.ini", "volume", "music", vol_buf)) {
		printf("failed to parse /usr/data/system.ini, set music volume to 20.\n");
		vol = 20;
	} else {
		vol = atoi(vol_buf);
	}

	if (vol == 100) {
		printf("max volume already, do nothing.\n");
		return;
	}

      
	vol += i;
	if (vol > 100)
		vol = 100;
	else if(vol < 0)
		vol = 0;

	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_set_volume(mozart_musicplayer_handler, vol);
		break;
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
		if (lapsule_is_controler()) {
			if (lapsule_do_set_vol(vol))
				printf("lapsule_do_set_vol failure.\n");
		}
		mozart_render_set_volume(vol);
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_HS_DOMAIN:
		{
			int i = 0;
			char vol_buf[8] = {};
			int hs_volume_set[16] = {0, 6, 13, 20, 26, 33, 40, 46, 53, 60, 66, 73, 80, 86, 93, 100};

			if (mozart_ini_getkey("/usr/data/system.ini", "volume", "bt_call", vol_buf)) {
				printf("failed to parse /usr/data/system.ini, set BT volume to 66.\n");
				vol = 66;
			} else {
				vol = atoi(vol_buf);
			}

			if(vol >= 100) {
				printf("bt volume has maximum!\n");
				break;
			}
			for(i = 0; i < 16; i++) {
				if(hs_volume_set[i] == vol)
					break;
			}
			if(i > 15) {
				printf("failed to get bt volume %d from hs_volume_set\n", vol);
				break;
			}
			mozart_volume_set(hs_volume_set[i + 1], BT_CALL_VOLUME);
			mozart_blutooth_hs_set_volume(BTHF_VOLUME_TYPE_SPK, i + 1);
			mozart_blutooth_hs_set_volume(BTHF_VOLUME_TYPE_MIC, i + 1);
		}
		break;
	case BT_AVK_DOMAIN:
		{
			int i = 0;
			int avk_volume_max = 17;
			UINT8 avk_volume_set_dsp[17] = {0, 6, 12, 18, 25, 31, 37, 43, 50, 56, 62, 68, 75, 81, 87, 93, 100};
			UINT8 avk_volume_set_phone[17] = {0, 15, 23, 31, 39, 47, 55, 63, 71, 79, 87, 95, 103, 111, 113, 125, 127};

			if (mozart_ini_getkey("/usr/data/system.ini", "volume", "bt_music", vol_buf)) {
				printf("failed to parse /usr/data/system.ini, set BT volume to 66.\n");
				vol = 63;
			} else {
				vol = atoi(vol_buf);
			}

			if(vol >= 100) {
				printf("bt volume has maximum!\n");
				break;
			}
			for(i = 0; i < avk_volume_max; i++) {
				if(avk_volume_set_dsp[i] > vol)
					break;
			}
			if(i >= avk_volume_max) {
				printf("failed to get music volume %d from avk_volume_set_dsp\n", vol);
				break;
			}
			printf("set avk dsp %d, set phone %d\n", avk_volume_set_dsp[i], avk_volume_set_phone[i]);
			mozart_volume_set(avk_volume_set_dsp[i], BT_MUSIC_VOLUME);
			mozart_bluetooth_avk_set_volume(avk_volume_set_phone[i]);
		}
		break;
#endif
	default:
		mozart_volume_set(vol, MUSIC_VOLUME);
		break;
	}
}

//减小音量
void mozart_volume_down(int i)
{
	int vol = 0;
	char vol_buf[8] = {};
	memory_domain domain;
	int ret = -1;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	if (mozart_ini_getkey("/usr/data/system.ini", "volume", "music", vol_buf)) {
		printf("failed to parse /usr/data/system.ini, set music volume to 20.\n");
		vol = 20;
	} else {
		vol = atoi(vol_buf);
	}

	if (vol == 0) {
		printf("min volume already, do nothing.\n");
		return;
	}

	vol -= i;
	if(vol > 100)
		vol = 100;
	else if(vol < 0)
		vol = 0;

	switch (domain) {
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_set_volume(mozart_musicplayer_handler, vol);
		break;
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
		if (lapsule_is_controler()) {
			if (lapsule_do_set_vol(vol))
				printf("lapsule_do_set_vol failure.\n");
		}
		mozart_render_set_volume(vol);
		break;
#endif
#if (SUPPORT_BT == BT_BCM)
	case BT_HS_DOMAIN:
		{
			int i = 0;
			char vol_buf[8] = {};
			int hs_volume_set[16] = {0, 6, 13, 20, 26, 33, 40, 46, 53, 60, 66, 73, 80, 86, 93, 100};

			if (mozart_ini_getkey("/usr/data/system.ini", "volume", "bt_call", vol_buf)) {
				printf("failed to parse /usr/data/system.ini, set BT volume to 66.\n");
				vol = 66;
			} else {
				vol = atoi(vol_buf);
			}

			if(vol <= 0) {
				printf("bt volume has maximum!\n");
				break;
			}
			for(i = 0; i < 16; i++) {
				if(hs_volume_set[i] == vol)
					break;
			}
			if(i > 15) {
				printf("failed to get bt volume %d from hs_volume_set\n", vol);
				break;
			}
			mozart_volume_set(hs_volume_set[i - 1], BT_CALL_VOLUME);
			mozart_blutooth_hs_set_volume(BTHF_VOLUME_TYPE_SPK, i - 1);
			mozart_blutooth_hs_set_volume(BTHF_VOLUME_TYPE_MIC, i - 1);
		}
		break;
	case BT_AVK_DOMAIN:
		{
			int i = 0;
			int avk_volume_max = 17;
			UINT8 avk_volume_set_dsp[17] = {0, 6, 12, 18, 25, 31, 37, 43, 50, 56, 62, 68, 75, 81, 87, 93, 100};
			UINT8 avk_volume_set_phone[17] = {0, 15, 23, 31, 39, 47, 55, 63, 71, 79, 87, 95, 103, 111, 113, 125, 127};

			if (mozart_ini_getkey("/usr/data/system.ini", "volume", "bt_music", vol_buf)) {
				printf("failed to parse /usr/data/system.ini, set BT volume to 63.\n");
				vol = 63;
			} else {
				vol = atoi(vol_buf);
			}

			if(vol <= 0) {
				printf("bt volume has minimum!\n");
				break;
			}
			for(i = (avk_volume_max - 1); i >= 0; i--) {
				if(avk_volume_set_dsp[i] < vol)
					break;
			}
			if(i < 0) {
				printf("failed to get music volume %d from avk_volume_set_dsp\n", vol);
				break;
			}
			printf("set avk dsp %d, set phone %d\n", avk_volume_set_dsp[i], avk_volume_set_phone[i]);

			mozart_volume_set(avk_volume_set_dsp[i], BT_MUSIC_VOLUME);
			mozart_bluetooth_avk_set_volume(avk_volume_set_phone[i]);
		}
		break;
#endif
	default:
		mozart_volume_set(vol, MUSIC_VOLUME);
		break;
	}
}
#if (SUPPORT_USB_AUDIO == 1)
void mozart_usb_audio_plug_in(void)          //没使用
{
	mozart_module_stop();
	stopall(1);
	mozart_ini_setkey("/usr/data/system.ini","usb_audio","use_usb_audio","0");
	mozart_play_key_sync("linein_mode");
	mozart_ini_setkey("/usr/data/system.ini","usb_audio","use_usb_audio","1");
	stopall(0);
	system("insmod /usr/fs/modules/g_audio.ko");
}

void mozart_usb_audio_plug_out(void)
{
	system("rmmod g_audio");
	mozart_play_key("linein_off");  //退出外接播放模式
	startall(0);
	startall(1);
}

#endif
static bool linein_is_active;

void mozart_linein_on(void)
{
	if (linein_is_active)
		return ;

	mozart_ui_linein_plugin();
	mozart_play_key_sync("linein_mode");
	mozart_linein_open();

	snd_source = SND_SRC_LINEIN;
	linein_is_active = true;
}

void mozart_linein_off(void)
{
	if (!linein_is_active)
		return ;

	mozart_linein_close();

	//TODO; tone contents may need change here.
	mozart_play_key("linein_off");	//退出外接播放模式
	linein_is_active = false;
}

void mozart_music_list(int keyIndex)
{
	int ret = -1;
	memory_domain domain;

	ret = share_mem_get_active_domain(&domain);
	if (ret) {
		printf("get active domain error in %s:%s:%d, do nothing.\n",
		       __FILE__, __func__, __LINE__);
		return;
	}

	switch (domain) {
	case UNKNOWN_DOMAIN:
		printf("system is in idle mode in %s:%s:%d.\n",
		       __FILE__, __func__, __LINE__);
		break;
#if (SUPPORT_DMR == 1)
	case RENDER_DOMAIN:
		if (lapsule_is_controler())
			lapsule_do_music_list(keyIndex);
		break;
#endif
	case MUSICPLAYER_DOMAIN:
		mozart_musicplayer_play_shortcut(mozart_musicplayer_handler, keyIndex);
		break;
	default:
		printf("Not support module(domain id:%d) in %s:%d.\n", domain, __func__, __LINE__);
		break;
	}
}
