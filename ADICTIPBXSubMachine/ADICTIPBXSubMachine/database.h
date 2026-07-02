#pragma once

typedef struct LogParam_Tag
{
	int StartTimestamp;
	int EndTimestamp;
	int Level;
	int Type;
	CHAR Message[200];
}LogParam_T, * PLogParam_T;

typedef struct Log_Tag
{
	int ID;
	int Timestamp;
	int Level;
	int Type;
	CHAR Message[200];
}Log_T, * PLog_T;

enum
{
	LOG_TYPE_SUB_MACHINE = 1,
	LOG_TYPE_PBX_UART,
};

enum
{
	LOG_LEVEL_MESSAGE = 1,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_FATAL_ERROR,
};

enum
{
	ID_LIST_ID = 0,
	ID_LIST_TYPE,
	ID_LIST_LEVEL,
	ID_LIST_DATE,
	ID_LIST_MESSAGE,
};


int CheckLogTableInDB(char* Error);
int InsertLogDB(int Timestame, int Level, int Type, PCHAR pMessage);
int GetLogNumberDB(PLogParam_T pLogParam);
int QueryLogDB(PLogParam_T pLogParam, PLog_T pLogDB);
int ClearLogDB();
int DeleteLogDB(int ID);
