#include "pch.h"
#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "database.h"

static int Numbers = 0;

static int CBLog(void* param, int argc, char** argv, char** azColName)
{
	PLog_T pQueryLog = (PLog_T)param;

	if (pQueryLog != NULL)
	{
		pQueryLog += Numbers;
		int i = 0;
		for (i = 0; i < argc; i++)
		{
			switch (i)
			{
			case 0: /* ID */
				pQueryLog->ID = atoi(argv[i]);
				break;
			case 1: /* timestamp */
				pQueryLog->Timestamp = atoi(argv[i]);
				break;
			case 2: /* level */
				pQueryLog->Level = atoi(argv[i]);
				break;
			case 3: /* type */
				pQueryLog->Type = atoi(argv[i]);
				break;
			case 4: /* vline */
				pQueryLog->vLineNo = atoi(argv[i]);
				break;
			case 5: /* message */
				strcpy_s(pQueryLog->Message, argv[i]);
				break;
			}
		}
	}
	Numbers++;

	return 0;
}

int CheckLogTableInDB(char* pError)
{
	sqlite3* pdb = NULL;
	int rc;
	char ErrMessage[100];
	char* zErr;
	char SqlStatement[1000] = "SELECT * FROM log;";
	char SqlCreateTable[] = "CREATE TABLE log \
		( \
			ID INTEGER PRIMARY KEY AUTOINCREMENT, \
			timestamp INT, \
			level INT, \
			type INT, \
			vline INT, \
			message	NCHAR(200) \
		)";
	char SqlCommand[1000];
	int ret = 0;

	/* Open database */
	rc = sqlite3_open(".\\log.db", &pdb);
	if (rc)
	{
		sqlite3_close(pdb);
		return -1;
	}

	sprintf_s(SqlCommand, 1000, SqlStatement);
	rc = sqlite3_exec(pdb, SqlCommand, CBLog, NULL, &zErr);
	if (rc != SQLITE_OK)
	{
		if (strncmp(zErr, "no such column: tagged", strlen("no such column: tagged")) == 0)
		{
			ret = 1;
		}
		else
		{
			sprintf_s(pError, 100, zErr);
			if (strstr(pError, "no"))
			{
				rc = sqlite3_exec(pdb, SqlCreateTable, CBLog, NULL, &zErr);
				if (rc != SQLITE_OK)
				{
					sprintf_s(pError, 100, zErr);
					ret = -2;
				}
			}
			sqlite3_free(zErr);
		}
	}

	sqlite3_close(pdb);

	return ret;
}

int InsertLogDB(int Timestame, int Level, int Type, int vLine, PCHAR pMessage)
{
	sqlite3* pdb = NULL;
	int rc;
	char ErrMessage[100];
	char* zErr;
	char SqlStatement[] = "INSERT INTO log (timestamp,level,type,message,vline) VALUES ('%d','%d','%d','%s','%d');";
	char SqlCommand[1000];
	int ret = 0;
	time_t rawtime;

	/* Open database */
	rc = sqlite3_open(".\\log.db", &pdb);
	if (rc)
	{
		sqlite3_close(pdb);
		return -1;
	}

	sprintf_s(SqlCommand, SqlStatement, Timestame, Level, Type, pMessage, vLine);
	rc = sqlite3_exec(pdb, SqlCommand, NULL, 0, &zErr);
	if (rc != SQLITE_OK)
	{
		sqlite3_free(zErr);
		ret = -2;
	}

	sqlite3_close(pdb);

	return ret;
}

int QueryLogDB(PLogParam_T pLogParam, PLog_T pLogDB)
{
	sqlite3* pdb = NULL;
	int rc;
	int ret = 0;
	char ErrMessage[100];
	char* zErr;
	char SqlStatement[1000] = "SELECT * FROM log WHERE timestamp >= %d AND timestamp <= %d";
	char SqlCommand[1000];

	/* Open database */
	rc = sqlite3_open(".\\log.db", &pdb);
	if (rc)
	{
		sqlite3_close(pdb);
		return -1;
	}
	int Len;

	Len = sprintf_s(SqlCommand, 1000, SqlStatement, pLogParam->StartTimestamp, pLogParam->EndTimestamp);
	if (pLogParam->Level != 0)
		Len += sprintf_s(SqlCommand + Len, 1000 - Len, " AND level = %d", pLogParam->Level);
	if (pLogParam->Type != 0)
		Len += sprintf_s(SqlCommand + Len, 1000 - Len, " AND type = %d", pLogParam->Type);
	if (pLogParam->VLineNo != 0)
		Len += sprintf_s(SqlCommand + Len, 1000 - Len, " AND vline = %d", pLogParam->VLineNo);

	Len += sprintf_s(SqlCommand + Len, 1000 - Len, " ORDER BY ID ASC");
	Numbers = 0;
	rc = sqlite3_exec(pdb, SqlCommand, CBLog, pLogDB, &zErr);
	if (rc != SQLITE_OK)
	{
		sqlite3_free(zErr);
		ret = -2;
	}

	sqlite3_close(pdb);
	ret = Numbers;

	return ret;
}

int GetLogNumberDB(PLogParam_T pLogParam)
{
	sqlite3* pdb = NULL;
	int rc;
	int ret = 0;
	char ErrMessage[100];
	char* zErr;
	char SqlStatement[1000] = "SELECT * FROM log WHERE timestamp >= %d AND timestamp <= %d";
	char SqlCommand[1000];

	/* Open database */
	rc = sqlite3_open(".\\log.db", &pdb);
	if (rc)
	{
		sqlite3_close(pdb);
		return -1;
	}
	int Len;

	Len = sprintf_s(SqlCommand, 1000, SqlStatement, pLogParam->StartTimestamp, pLogParam->EndTimestamp);
	if (pLogParam->Level != 0)
		Len += sprintf_s(SqlCommand + Len, 1000 - Len, " AND level = %d", pLogParam->Level);
	if (pLogParam->Type != 0)
		Len += sprintf_s(SqlCommand + Len, 1000 - Len, " AND type = %d", pLogParam->Type);
	if (pLogParam->VLineNo != 0)
		Len += sprintf_s(SqlCommand + Len, 1000 - Len, " AND vline = %d", pLogParam->VLineNo);

	Len += sprintf_s(SqlCommand + Len, 1000 - Len, " ORDER BY timestamp ASC");
	Numbers = 0;
	rc = sqlite3_exec(pdb, SqlCommand, CBLog, NULL, &zErr);
	if (rc != SQLITE_OK)
	{
		sqlite3_free(zErr);
		ret = -2;
	}

	sqlite3_close(pdb);
	ret = Numbers;

	return ret;
}

int ClearLogDB()
{
	sqlite3* pdb = NULL;
	int rc;
	char ErrMessage[100];
	char* zErr;
	char SqlStatement[] = "DELETE FROM log;";
	char SqlCommand[1000];
	int ret = 0;
	time_t rawtime;

	/* Open database */
	rc = sqlite3_open(".\\log.db", &pdb);
	if (rc)
	{
		sqlite3_close(pdb);
		return -1;
	}

	sprintf_s(SqlCommand, 1000, SqlStatement);
	rc = sqlite3_exec(pdb, SqlCommand, NULL, 0, &zErr);
	if (rc != SQLITE_OK)
	{
		sqlite3_free(zErr);
		ret = -2;
	}

	sqlite3_close(pdb);

	return ret;
}

int DeleteLogDB(int ID)
{
	sqlite3* pdb = NULL;
	int rc;
	char ErrMessage[100];
	char* zErr;
	char SqlStatement[] = "DELETE FROM log WHERE ID=%d";
	char SqlCommand[1000];
	int ret = 0;
	time_t rawtime;
	int Len;

	/* Open database */
	rc = sqlite3_open(".\\log.db", &pdb);
	if (rc)
	{
		sqlite3_close(pdb);
		return -1;
	}

	Len = sprintf_s(SqlCommand, 1000, SqlStatement, ID);
	rc = sqlite3_exec(pdb, SqlCommand, NULL, 0, &zErr);
	if (rc != SQLITE_OK)
	{
		sqlite3_free(zErr);
		ret = -2;
	}

	sqlite3_close(pdb);

	return ret;
}