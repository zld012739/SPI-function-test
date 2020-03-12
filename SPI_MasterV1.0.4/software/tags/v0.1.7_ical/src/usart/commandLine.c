/***************************************************************************
** commandLine.c
** Description:
** Simple interactive shell
**
**************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include "commandLine.h"
#include "debug.h"
#include "debug_usart.h"
#include "data_type.h" // for float32_t
#include "utilities.h"

static void _ExecLine(tCommand const *cmd_table);
static void _Dispatch(char const *token, tCommand const *table);
static void _CmdHelp(tCommand const *table);

static char gCmdLine[80];
static char *gCmdLineIndex;
#define PARSE_SEPARATOR ' '

static void CmdPrintPrompt()
{
	static uint16_t lineno;
    DebugPrintInt("", lineno);
    lineno++;
    DebugPrintString("> ");
}

void InitCommandLine() 
{
    CmdPrintPrompt();
}

void CmdLineLookup(tCommand const *cmd_table) 
{
    static uint32_t index = 0;
    if (DebugSerialReadLine((uint8_t*) gCmdLine, &index, 80)) {
        strrep(gCmdLine, '\r', 0);
        if (gCmdLine[0]) {//don't process empty string
            //Ignore lines that start with # character
            if (gCmdLine[0] != '#') {
                _ExecLine(cmd_table);
            }
        }
        CmdPrintPrompt();
	} // else wait until prompt is ready
}

static void _ExecLine(tCommand const *cmd_table) {
	char *token = strtok_r(gCmdLine, PARSE_SEPARATOR, &gCmdLineIndex);
	_Dispatch(token, cmd_table);
}

void CmdLinexec(char const *cmdline, tCommand const *cmd_table) {
	if (cmdline && *cmdline) {
		char *dst = gCmdLine;
		while ((*dst++ = *cmdline++));
		_ExecLine(cmd_table);
	}
}


int CmdLineGetArgString(uint8_t **s) {
	char *token = strtok_r(0, PARSE_SEPARATOR, &gCmdLineIndex);
	if (token) {
		if (s) {
			*s = (uint8_t*)token;
		}
		return 1;
	} else {
		return 0;
	}
}

int CmdLineGetArgInt(int32_t *i) {
	char *token = strtok_r(0, PARSE_SEPARATOR, &gCmdLineIndex);
	if (token) {
		if (i) {
			*i = atoi(token);
		}
		return 1;
	} else {
		return 0;
	}
}

int CmdLineGetArgUInt(uint32_t *i) {
  char *token = strtok_r(0, PARSE_SEPARATOR, &gCmdLineIndex);
  if (token) {
    if (i) {
      *i = atoi(token);
    }
    return 1;
  } else {
    return 0;
  }
}

int CmdLineGetArgFloat(float32_t *f) {
  char *token = strtok_r(0, PARSE_SEPARATOR, &gCmdLineIndex);
  if (token) {
    if (f) {
      *f = atof(token);
    }
    return 1;
  } else {
    return 0;
  }
}


//Traverses a command table and dispatches the appropriate callback
static void _Dispatch(char const *token, tCommand const *table) {
	tCommand *cmd = (tCommand*) table;
	if (0 == strcmpi(token, "help")) {
			_CmdHelp(table);
	} else {
		while (cmd->callback != 0) {
			if (0 == strcmpi(token, cmd->name)) {
				(*(cmd->callback))(cmd->callbackData);
				return;
			} else {
				cmd++;
			}
		}
		//If we get here, the command wasn't found
		DebugPrintString("Unknown command (");
		DebugPrintString(token);
		DebugPrintString(") Type 'help' for list.\r\n");
	}
}

static void _CmdHelp(tCommand const *table) {
	tCommand *cmd = (tCommand*) table;
	while (cmd->callback != 0) {
		DebugPrintString(" ");
		DebugPrintString( cmd->name);
		DebugPrintString("\t");
		DebugPrintString( cmd->help);
        DebugPrintEndline();
		cmd++;
        while (!IsDebugSerialIdle()) {/*spin*/;}
	}
}

