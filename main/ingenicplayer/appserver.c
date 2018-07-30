#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>

#include "json-c/json.h"

#include "mozart_config.h"
#include "appserver.h"
#include "ingenicplayer.h"
#if (SUPPORT_ALARM == 1)
#include "modules/mozart_module_alarm.h"
#include "alarm_interface.h"
#endif

/*接受app发来的指令*/
int appserver_cmd_callback(char *scope, char *command, char *data, struct appserver_cli_info *owner)
{
	int ret = 0;

	  /* printf("%s: %s, %s %s\n", __func__, scope, command, data); */
	       printf("%s（app发送来的命令）: %s, %s %s\n", __func__, scope, command, data);

		ret = mozart_ingenicplayer_response_cmd(scope, data, owner);
		return ret;

}
