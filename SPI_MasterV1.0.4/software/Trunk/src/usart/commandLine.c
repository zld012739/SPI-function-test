/** ****************************************************************************
 * @file commandLine.c
 * @author
 * @date: 2011-02-10 11:42:26 -0800 (Thu, 10 Feb 2011)
 * @brief  Copyright (c) 2013, 2014 All Rights Reserved.
 * @brief description: Simple interactive serial
 ******************************************************************************/
 /** ***************************************************************************
 * @file   commandLine.c
 * @Author
 * @date   September, 2008
 * @brief  Copyright (c) 2013, 2014 All Rights Reserved.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * Simple interactive serial console shell - line input
 ******************************************************************************/
#include <stdint.h>
#include <stdlib.h>
//#include <string.h> // strcpy

#include "commandLine.h"
#include "debug.h"
#include "debug_usart.h"
#include "data_type.h" // for float32_t
#include "utilities.h"
#include "spi.h"
#include "timer.h"
#include "commands.h"
#include "bsp.h"

static void _ExecLine(tCommand const *cmd_table);
static void _Dispatch(char const *token, tCommand const *table);
static void _CmdHelp(tCommand const *table);

static char gCmdLine[80];
//static char oldGCmdLine[80];
static char *gCmdLineIndex;

#define PARSE_SEPARATOR ' '
/*******************************************************************************
Read from register
*******************************************************************************/
void Read_from_register(uint8_t startingAddress,
                        uint8_t endingAddress
                        )
{
	uint16_t RxBuffer[12] = {0};
	ReadFromRegisters(startingAddress, endingAddress, RxBuffer, 500, 1 );
	DEBUG_HEX( "0x", RxBuffer[1]);
}
/*******************************************************************************
write to register
*******************************************************************************/
void Write_to_register(uint8_t startingAddress,
                       uint8_t endingAddress,
                        uint8_t data
                        )
{
	uint16_t RxBuffer[12] = {0};
        WriteToRegister( startingAddress,data);
        DelayMs(10);
        ReadFromRegisters(startingAddress, endingAddress, RxBuffer, 500, 1 );
	DEBUG_HEX( "0x", RxBuffer[1]);
        
}

/** ***************************************************************************
 * @name    CmdPrintPrompt()
 * @brief send prompt characters out serial to the debug serial console
 *
 * @param [in] N/A
 * @param [out] "#> " out serial to console
 * @retval N/A
 ******************************************************************************/
static void CmdPrintPrompt()
{
	static uint16_t lineno;
    DebugPrintInt("", lineno);
    lineno++;
    DebugPrintString("> ");
}

/** ***************************************************************************
 * @name    InitCommandLine()
 * @brief at startup call send prompt characters out serial to the debug serial
 * console
 *
 * @param [in] N/A
 * @param [in] "#> " out serial to console
 * @retval N/A
 ******************************************************************************/
void InitCommandLine()
{
    CmdPrintPrompt();
}

/** ***************************************************************************
 * @name    CmdLineLookup()
 * @brief at startup call send prompt characters out serial to console
 * the debug serial
 *
 * @param [in] cmd_table - table entry to parse
 * @param [in] "#> " out serial to console
 * @retval N/A
 ******************************************************************************/
/*void CmdLineLookup(tCommand const *cmd_table)
{
    static uint32_t index = 0;
    
    if (DebugSerialReadLine((uint8_t*) gCmdLine, &index, 80)) {
        strrep(gCmdLine, '\r', 0); // replace \r with null
        if (gCmdLine[0]) {///< don't process empty string
            ///< Ignore lines that start with # character
            if (gCmdLine[0] != '#') {
                _ExecLine(cmd_table);
            }
        }
        CmdPrintPrompt();
	} ///< else wait until prompt is ready
}*/
void CmdLineLookup(tCommand const *cmd_table)
{
    static uint32_t index = 0;
    uint32_t para_num = 0;
    uint32_t i;
    if(DebugSerialReadLine((uint8_t*) gCmdLine, &index, 80)){
    
    for (i=0;i<80;i++){
      if(gCmdLine[i] == 0x0A)
        break;
      else
        para_num = para_num + 1;
    }
    if(para_num == 1){
      if(gCmdLine[0] == 0x3E){
        CmdUserSpi_BurstRead(1);
      }
      else if(gCmdLine[0] == 0x3F){
      CmdUserSpi_BurstRead_F1(0);
      }
      else if(gCmdLine[0] == 0xFF){
        PowerOff();
        DelayMs(500);
        PowerOn();
        DelayMs(500);
      }
      else{
        Read_from_register(gCmdLine[0],gCmdLine[0]);
       }
    }
    if(para_num == 2){
      if(gCmdLine[0] == 0x76){
           WriteToRegister( gCmdLine[0],gCmdLine[1]);
           DelayMs(1000);
        }
      else{
      Write_to_register(gCmdLine[0],gCmdLine[0],gCmdLine[1]);
      }
    }
    if(para_num == 3){
      if(gCmdLine[0] == 0x3E){
          CmdUserSpi_BurstRead(1000000);
      }
      else{
        WriteToRegister( gCmdLine[0],gCmdLine[1]);
      }
      }
    }
    
    /*
    uint32_t ouput = 0;
    if (DebugSerialReadLine((uint8_t*) gCmdLine, &index, 80)) {
        if (gCmdLine[0]) {///< don't process empty string
            ///< Ignore lines that start with # character
            if (gCmdLine[0] != '#') {
              while(gCmdLine[ouput] != '\r'){
                DEBUG_HEX( "0x", gCmdLine[ouput]);
                DEBUG_HEX( "0x", 0x21);
                DEBUG_ENDLINE();
                ouput++;
              }
              
            }
        }
        CmdPrintPrompt();
	} ///< else wait until prompt is ready*/
}

/** ***************************************************************************
 * @name    _ExecLine() LOCAL
 * @brief extract tokens from strings with ' ' as the delimiter
 *
 * @param [in] cmd_table - table entry to parse
 * @param [in] "#> " out serial to console
 * @retval N/A
 ******************************************************************************/
static void _ExecLine(tCommand const *cmd_table) {
	char *token = strtok_r(gCmdLine, PARSE_SEPARATOR, &gCmdLineIndex);
#if 0
    char *upArrow = "[A";
    if (!strcmpi(token, (char const*)upArrow)) // up arrow
    { // repeat last line
      strcpy (gCmdLine, oldGCmdLine);
      token = strtok_r(gCmdLine, PARSE_SEPARATOR, &gCmdLineIndex);
    } else { // only buffer the last command line
        strcpy (oldGCmdLine, gCmdLine);
    }
#endif
	_Dispatch(token, cmd_table);
}

void CmdLinexec(char const *cmdline,
                tCommand const *cmd_table) {
	if (cmdline && *cmdline) {
		char *dst = gCmdLine;
		while ((*dst++ = *cmdline++));
		_ExecLine(cmd_table);
	}
}

/** ***************************************************************************
 * @name    CmdLineGetArgString()
 * @brief get string from command line arguments
 *
 * @param [out] s - pointer to a string
 * @retval N/A
 ******************************************************************************/
int CmdLineGetArgString(uint8_t **s)
{
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

/** ***************************************************************************
 * @name    CmdLineGetArgUInt()
 * @brief get unsigned integer from command line arguments
 *
 * @param [out] t - ingteger to return
 * @retval N/A
 ******************************************************************************/
int CmdLineGetArgUInt(uint32_t *i)
{
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

/** ***************************************************************************
 * @name    _Dispatch() LOCAL
 * @brief Traverses a command table and dispatches the appropriate callback
 *
 * @param [in] token - command text from the console
 * @param [in] table - command table
 * @retval N/A
 ******************************************************************************/
static void _Dispatch(char const     *token,
                      tCommand const *table)
{
	tCommand *cmd = (tCommand*) table;
	if (0 == strcmpi(token, "help")) {
			_CmdHelp(table);
	} else {
		while (cmd->callback != 0) {
			if (0 == strcmpi(token, cmd->name)) {
				(*(cmd->callback))(cmd->callbackData);
				return; // found a command
			} else {
				cmd++;
			}
		}
		/// If we get here, the command wasn't found
		DebugPrintString("Unknown command (");
		DebugPrintString(token);
		DebugPrintString(") Type 'help' for list.\r\n");
	}
}

/** ***************************************************************************
 * @name    _CmdHelp() LOCAL
 * @brief traverses command table and prints the help strings
 *
 * @param [in] token - command text from the console
 * @param [in] table - command table
 * @retval N/A
 ******************************************************************************/
static void _CmdHelp(tCommand const *table)
{
	tCommand *cmd = (tCommand*) table;
	while (cmd->callback != 0) {
		DebugPrintString(" ");
		DebugPrintString( cmd->name);
		DebugPrintString("\t");
		DebugPrintString( cmd->help);
        DebugPrintEndline();
		cmd++;
        while (!IsDebugSerialIdle())
        {/*spin*/;}
	}
}

