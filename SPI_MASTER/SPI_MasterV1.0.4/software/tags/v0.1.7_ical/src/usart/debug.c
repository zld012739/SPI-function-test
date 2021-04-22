//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// FILE:  debug.c
//
// Copyright 2013 by MEMSIC, Inc., all rights reserved.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "debug.h"
#include "debug_usart.h"

// helper functions
static void itoa(int value, char s[], int base);

void DebugPrintString(const char * str)
{
    while (str && *str) {
        if (*str < 0x7E) { // valid printable chars only
            DebugSerialPutChar(*str);
        }
        str++;
    }
}
void DebugPrintInt(const char *s, int i)
{
    /* 32 bit int: -2147483646        */
    /* digits      01234567890 + NULL */
    char numberString[11]; 
    DebugPrintString(s);
    itoa(i, numberString, 10); // base 10
    DebugPrintString(numberString);
}
void DebugPrintHex(const char *s, int i)
{
    /* 32 bit int: FFFFFFFF        */
    /* digits      01234567 + NULL */
    char numberString[9]; 
    DebugPrintString(s);
    itoa(i, numberString, 16); /* base 16 */
    DebugPrintString(numberString);

}
void DebugPrintFloat(const char *s, float f, int sigDigits)
{
    char numberString[11]; 
    int i;
    DebugPrintString(s);
    i = (int) f; // just get the number to the left of the decimal
    if (i == 0 && f < 0) {
        DebugPrintString("-");
    }
    DebugPrintInt("", i);
    
    // now get the number of significant digits to the right
    f = f - i;
    if (f < 0) f = -f;
    if (sigDigits > (sizeof(numberString) -1) ) {
        sigDigits = sizeof(numberString) -1;
    }
    DebugPrintString(".");
    while (sigDigits) {    
        f *= 10;
        sigDigits--;
        i = (int) f;
        DebugPrintInt("", i);
        f = f - i;
    }
}

void DebugPrintEndline()
{
    DebugPrintString("\r\n");
}

#define MAX_INT32_STRING 11 // 10 digits plus a NULL
#define RADIX_DECIMAL    10

void itoa(int value, char *sp, int radix)
{
    char tmp[MAX_INT32_STRING];
    char *tp = tmp;
    int i;
    unsigned v;
    int sign;
    
    sign = (radix == RADIX_DECIMAL && value < 0);
    if (sign)  { v = -value; }
    else       { v = (unsigned)value; }
    
    while (v || tp == tmp)
    {
        i = v % radix;
        v /= radix; 
        if (i < RADIX_DECIMAL) {
          *tp++ = i+'0';
        } else {
          *tp++ = i + 'A' - RADIX_DECIMAL;
        }
    }
    
    if (radix != RADIX_DECIMAL) { // zero fill non decimal values
        while (tp < &(tmp[4])) {
          *tp++ = '0';
        }
    }
    
    if (sign) { *sp++ = '-'; }
    // reverse and put in output buffer
    while (tp > tmp) {
        tp--;
        *sp = *tp;
        sp++; 
    }
    *sp = '\0';
    
}