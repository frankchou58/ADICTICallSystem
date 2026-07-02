#pragma once

typedef struct PBXFormate_Tag
{
	CHAR Header[1];
	CHAR Interface[1];
	CHAR Direct[1];
	CHAR Divide[1];	/*:*/
	CHAR ChargeStartDate[8];
	CHAR CrossFlag[1];
	CHAR FirstExtNum[4];
	CHAR Reserve1[1];
	CHAR SecondExtNum[4];
	CHAR Reserve2[1];
	CHAR OutPortNo[2];
	CHAR Reserve3[1];
	CHAR CallID[26];
	CHAR Reserve4[1];
	CHAR ChargeCode[4];
	CHAR Reserve5[1];
	CHAR ChargeStartTime[8];
	CHAR Reserve6[1];
	CHAR TalkingDuration[5];
	CHAR Reserve7[1];
	CHAR ZeroBytes[5];
	CHAR Reserve8[2];
	CHAR EndChars[2];
}PBXFormate_T, *PPBXFormate_T;

BOOL PBXInterpreterOpen();
BOOL OpenUARTPort(int Port, int BaudRate);
void CloseUARTPort();
