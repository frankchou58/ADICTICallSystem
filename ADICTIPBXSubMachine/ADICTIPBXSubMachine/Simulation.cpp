#include "pch.h"
#include <iostream>
#include <fstream>
#include <time.h>
#include "Simulation.h"
#include "MachineClient.h"

using namespace std;
const int MAX_CHARS_PER_LINE = 1024;
int m_MachineID = 2;

static HANDLE m_hThreadSimulation;

static UINT32 ParseTime(PCHAR pBuff, PCHAR pDate)
{
    UINT32 Timestamp = 0;
    PCHAR PtrEnd;
    PCHAR Ptr;
    int Len;
    CHAR Data[100];
    int HeaderLen = strlen("DateTime=");

    Ptr = strstr(pBuff, "DateTime=");
    if (Ptr)
    {
        Ptr += HeaderLen;
        PtrEnd = strstr(Ptr, "^");
        if (PtrEnd)
        {
            Len = PtrEnd - Ptr;
            strncpy_s(Data, Ptr, Len);
            memcpy(pDate, Data, 19);
        }
        else
        {
            strcpy_s(Data, Ptr);
            memcpy(pDate, Data, 19);
        }
    }
    /*YYYY/MM/DD HH:MM:SS*/
    Data[4] = Data[7] = Data[10] = Data[13] = Data[16] = Data[19] = 0;

    struct tm time;
    time.tm_year = atoi(&Data[0]) - 1900;
    time.tm_mon = atoi(&Data[5]) - 1;
    time.tm_mday = atoi(&Data[8]);
    time.tm_hour = atoi(&Data[11]);
    time.tm_min = atoi(&Data[14]);
    time.tm_sec = atoi(&Data[17]);
    time_t loctime = mktime(&time);  // timestamp in current timezone

    return Timestamp = loctime;
}

static int ParseLineNo(PCHAR pBuff)
{
    int LineNo;
    PCHAR PtrEnd;
    PCHAR Ptr;
    int Len;
    CHAR Data[100];
    int HeaderLen = strlen("LineNo=");

    Ptr = strstr(pBuff, "LineNo=");
    if (Ptr)
    {
        Ptr += HeaderLen;
        PtrEnd = strstr(Ptr, "^");
        if (PtrEnd)
        {
            Len = PtrEnd - Ptr;
            strncpy_s(Data, Ptr, Len);
        }
        else
        {
            strcpy_s(Data, Ptr);
        }
    }
    LineNo = atoi(Data);

    return LineNo;
}

static int ParseExtNo(PCHAR pBuff)
{
    int ExtNo;
    PCHAR PtrEnd;
    PCHAR Ptr;
    int Len;
    CHAR Data[100];
    int HeaderLen = strlen("ExtNo=");

    Ptr = strstr(pBuff, "ExtNo=");
    if (Ptr)
    {
        Ptr += HeaderLen;
        PtrEnd = strstr(Ptr, "^");
        if (PtrEnd)
        {
            Len = PtrEnd - Ptr;
            strncpy_s(Data, Ptr, Len);
        }
        else
        {
            strcpy_s(Data, Ptr);
        }
    }
    ExtNo = atoi(Data);

    return ExtNo;
}

static int ParseFromExtNo(PCHAR pBuff)
{
    int ExtNo;
    PCHAR PtrEnd;
    PCHAR Ptr;
    int Len;
    CHAR Data[100];
    int HeaderLen = strlen("FromExtNo=");

    Ptr = strstr(pBuff, "FromExtNo=");
    if (Ptr)
    {
        Ptr += HeaderLen;
        PtrEnd = strstr(Ptr, "^");
        if (PtrEnd)
        {
            Len = PtrEnd - Ptr;
            strncpy_s(Data, Ptr, Len);
        }
        else
        {
            strcpy_s(Data, Ptr);
        }
    }
    ExtNo = atoi(Data);

    return ExtNo;
}

static int ParseToExtNo(PCHAR pBuff)
{
    int ExtNo;
    PCHAR PtrEnd;
    PCHAR Ptr;
    int Len;
    CHAR Data[100];
    int HeaderLen = strlen("ToExtNo=");

    Ptr = strstr(pBuff, "ToExtNo=");
    if (Ptr)
    {
        Ptr += HeaderLen;
        PtrEnd = strstr(Ptr, "^");
        if (PtrEnd)
        {
            Len = PtrEnd - Ptr;
            strncpy_s(Data, Ptr, Len);
        }
        else
        {
            strcpy_s(Data, Ptr);
        }
    }
    ExtNo = atoi(Data);

    return ExtNo;
}

static int ParseCallerID(PCHAR pBuff, PCHAR pCallerID)
{
    int Ret = -1;
    PCHAR PtrEnd;
    PCHAR Ptr;
    int Len;
    CHAR Data[100];
    int HeaderLen = strlen("Tel=");

    Ptr = strstr(pBuff, "Tel=");
    if (Ptr)
    {
        Ptr += HeaderLen;
        PtrEnd = strstr(Ptr, "^");
        if (PtrEnd)
        {
            Len = PtrEnd - Ptr;
            strncpy_s(pCallerID, 50, Ptr, Len);
        }
        else
        {
            strcpy_s(pCallerID, 50, Ptr);
        }
        Ret = 0;
    }

    return Ret;
}

static int ParseRingNums(PCHAR pBuff)
{
    int RingNums;
    PCHAR PtrEnd;
    PCHAR Ptr;
    int Len;
    CHAR Data[100];
    int HeaderLen = strlen("RingDuration=");

    Ptr = strstr(pBuff, "RingDuration=");
    if (Ptr)
    {
        Ptr += HeaderLen;
        PtrEnd = strstr(Ptr, "^");
        if (PtrEnd)
        {
            Len = PtrEnd - Ptr;
            strncpy_s(Data, Ptr, Len);
        }
        else
        {
            strcpy_s(Data, Ptr);
        }
    }
    RingNums = atoi(Data);

    return RingNums;
}

static int ParseTalkDuration(PCHAR pBuff)
{
    int TalkDuration;
    PCHAR PtrEnd;
    PCHAR Ptr;
    int Len;
    CHAR Data[100];
    int HeaderLen = strlen("Duration=");

    Ptr = strstr(pBuff, "Duration=");
    if (Ptr)
    {
        Ptr += HeaderLen;
        PtrEnd = strstr(Ptr, "^");
        if (PtrEnd)
        {
            Len = PtrEnd - Ptr;
            strncpy_s(Data, Ptr, Len);
        }
        else
        {
            strcpy_s(Data, Ptr);
        }
    }
    TalkDuration = atoi(Data);

    return TalkDuration;
}

static int ParseVoiceFileName(PCHAR pBuff, PCHAR pVoiceFileName)
{
    PCHAR PtrEnd;
    PCHAR Ptr;
    int Len;
    int HeaderLen = strlen("VoiceFile=");
    int Ret = -1;

    Ptr = strstr(pBuff, "VoiceFile=");
    if (Ptr)
    {
        Ptr += HeaderLen;
        PtrEnd = strstr(Ptr, "^");
        if (PtrEnd)
        {
            Len = PtrEnd - Ptr;
            strncpy_s(pVoiceFileName, 50, Ptr, Len);
        }
        else
        {
            strcpy_s(pVoiceFileName, 50, Ptr);
        }
        Ret = 0;
    }

    return Ret;
}


static int DoExternalCallOut(PCHAR PtrOC)
{
    int LineNo;
    int ExtNo;
    int FromExtNo;
    int ToExtNo;
    int RingNums;
    int Duration;
    int Ret;

    LineNo = ParseLineNo(PtrOC);
    CHAR CalleeID[50];
    ParseCallerID(PtrOC, CalleeID);
    ExtNo = ParseExtNo(PtrOC);
    //printf("外線撥出: 時間=%s 外線號碼=%d 去電碼=%s 分機號碼=%d\n", Date, LineNo, CalleeID, ExtNo);
    CallParams_T CallParams;
    memset(&CallParams, 0x00, sizeof(CallParams_T));
    CallParams.MachineType = PACKAGE_MACHINE_TYPE_PBX;
    CallParams.MachineID = m_MachineID;
    CallParams.OutPortNo = LineNo;
    CallParams.FromExtNo = ExtNo;
    CallParams.pCallID = CalleeID;
    Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_OUT, &CallParams);

    return Ret;
}

static int DoExternalCallIn(PCHAR PtrRC)
{
    int LineNo;
    int ExtNo;
    int FromExtNo;
    int ToExtNo;
    int RingNums;
    int Duration;
    int Ret;

    LineNo = ParseLineNo(PtrRC);
    CHAR CallerID[50];
    ParseCallerID(PtrRC, CallerID);
    ExtNo = ParseExtNo(PtrRC);
    //printf("外線撥入: 時間=%s 外線號碼=%d 來電碼=%s 分機號碼=%d\n", Date, LineNo, CallerID, ExtNo);
    CallParams_T CallParams;
    memset(&CallParams, 0x00, sizeof(CallParams_T));
    CallParams.MachineType = PACKAGE_MACHINE_TYPE_PBX;
    CallParams.MachineID = m_MachineID;
    CallParams.OutPortNo = LineNo;
    CallParams.FromExtNo = ExtNo;
    CallParams.pCallID = CallerID;
    Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_IN, &CallParams);

    return Ret;
}

static int DoExternalAnswer(PCHAR PtrAN)
{
    int LineNo;
    int ExtNo;
    int FromExtNo;
    int ToExtNo;
    int RingNums;
    int Duration;
    int Ret;

    LineNo = ParseLineNo(PtrAN);
    ExtNo = ParseExtNo(PtrAN);
    RingNums = ParseRingNums(PtrAN);
    //printf("通話    : 時間=%s 外線號碼=%d與分機號碼=%d,振鈴次數=%d次[通話中] TimeStamp=%d\n", Date, LineNo, ExtNo, RingNums, TimeStamp);
    CallParams_T CallParams;
    memset(&CallParams, 0x00, sizeof(CallParams_T));
    CallParams.MachineType = PACKAGE_MACHINE_TYPE_PBX;
    CallParams.MachineID = m_MachineID;
    CallParams.OutPortNo = LineNo;
    CallParams.FromExtNo = ExtNo;
    CallParams.RingNums = RingNums;
    Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER, &CallParams);

    return Ret;
}

static int DoExternalHangup(PCHAR PtrHU)
{
    int LineNo;
    int ExtNo;
    int FromExtNo;
    int ToExtNo;
    int RingNums;
    int Duration;
    int Ret;

    LineNo = ParseLineNo(PtrHU);
    ExtNo = ParseExtNo(PtrHU);
    Duration = ParseTalkDuration(PtrHU);
    //printf("掛斷    : 時間=%s 外線號碼=%d與分機號碼=%d,通話時間=%d秒[通話結束],\n", Date, LineNo, ExtNo, Duration);
    CallParams_T CallParams;
    memset(&CallParams, 0x00, sizeof(CallParams_T));
    CallParams.MachineType = PACKAGE_MACHINE_TYPE_PBX;
    CallParams.MachineID = m_MachineID;
    CallParams.OutPortNo = LineNo;
    CallParams.FromExtNo = ExtNo;
    CallParams.Duration = Duration;
    Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP, &CallParams);

    return Ret;
}

static int DoVoiceRecFileName(PCHAR PtrVF)
{
    int LineNo;
    int ExtNo;
    int FromExtNo;
    int ToExtNo;
    int RingNums;
    int Duration;
    int Ret;

    CHAR FileName[50];
    LineNo = ParseLineNo(PtrVF);
    ExtNo = ParseExtNo(PtrVF);
    ParseVoiceFileName(PtrVF, FileName);
    //printf("錄音檔   : 時間=%s 外線號碼=%d與分機號碼=%d,錄音檔=%s,\n", Date, LineNo, ExtNo, FileName);
    CallParams_T CallParams;
    memset(&CallParams, 0x00, sizeof(CallParams_T));
    CallParams.MachineType = PACKAGE_MACHINE_TYPE_PBX;
    CallParams.MachineID = m_MachineID;
    CallParams.OutPortNo = LineNo;
    CallParams.FromExtNo = ExtNo;
    CallParams.pVoiceRecFileName = FileName;
    Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_VOICE_REC_FILENAME, &CallParams);

    return Ret;
}

static int DoInternalCallMakeup(PCHAR PtrERC)
{
    int LineNo;
    int ExtNo;
    int FromExtNo;
    int ToExtNo;
    int RingNums;
    int Duration;
    int Ret;

    FromExtNo = ParseFromExtNo(PtrERC);
    ToExtNo = ParseToExtNo(PtrERC);
    //printf("分機互打  : 時間=%s 撥出分機號碼=%d與撥入分機號碼=%d\n", Date, FromExtNo, ToExtNo);
    CallParams_T CallParams;
    memset(&CallParams, 0x00, sizeof(CallParams_T));
    CallParams.MachineType = PACKAGE_MACHINE_TYPE_PBX;
    CallParams.MachineID = m_MachineID;
    CallParams.ToExtNo = ToExtNo;
    CallParams.FromExtNo = FromExtNo;
    Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_INTERNAL_CALL_MAKE, &CallParams);

    return Ret;
}

static int DoInternalCallAnswer(PCHAR PtrEAN)
{
    int LineNo;
    int ExtNo;
    int FromExtNo;
    int ToExtNo;
    int RingNums;
    int Duration;
    int Ret;

    FromExtNo = ParseFromExtNo(PtrEAN);
    ToExtNo = ParseToExtNo(PtrEAN);
    RingNums = ParseRingNums(PtrEAN);
    //printf("分機通話  : 時間=%s 撥出分機號碼=%d與撥入分機號碼=%d,振鈴次數=%d次[通話中]\n", Date, FromExtNo, ToExtNo, RingNums);
    CallParams_T CallParams;
    memset(&CallParams, 0x00, sizeof(CallParams_T));
    CallParams.MachineType = PACKAGE_MACHINE_TYPE_PBX;
    CallParams.MachineID = m_MachineID;
    CallParams.ToExtNo = ToExtNo;
    CallParams.FromExtNo = FromExtNo;
    CallParams.RingNums = RingNums;
    Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_INTERNAL_CALL_ANSWER, &CallParams);

    return Ret;
}

static int DoInternalCallHangup(PCHAR PtrEHU)
{
    int LineNo;
    int ExtNo;
    int FromExtNo;
    int ToExtNo;
    int RingNums;
    int Duration;
    int Ret;

    FromExtNo = ParseFromExtNo(PtrEHU);
    ToExtNo = ParseToExtNo(PtrEHU);
    Duration = ParseTalkDuration(PtrEHU);
    //printf("分機掛機  : 時間=%s 撥出分機號碼=%d與撥入分機號碼=%d,通話時間=%d秒[通話結束],\n", Date, FromExtNo, ToExtNo, Duration);
    CallParams_T CallParams;
    memset(&CallParams, 0x00, sizeof(CallParams_T));
    CallParams.MachineType = PACKAGE_MACHINE_TYPE_PBX;
    CallParams.MachineID = m_MachineID;
    CallParams.ToExtNo = ToExtNo;
    CallParams.FromExtNo = FromExtNo;
    CallParams.Duration = Duration;
    Ret = ClientSendCallStatus(PACKAGE_EVENT_TYPE_INTERNAL_CALL_HANGUP, &CallParams);

    return Ret;
}

static void ThreadSimulation(LPVOID lParam)
{
    CHAR buff[MAX_CHARS_PER_LINE];
    fstream  PPGLogFile;
    PCHAR PtrRC;
    PCHAR PtrOC;
    PCHAR PtrAN;
    PCHAR PtrHU;
    PCHAR PtrLineNo;
    PCHAR PtrEnd;
    PCHAR PtrVF;
    PCHAR PtrERC;
    PCHAR PtrEAN;
    PCHAR PtrEHU;
    int Len;
    CHAR FileName[8][100] =
	{
		".\\TCP20220210.log",
		".\\TCP20220211.log",
		".\\TCP20220212.log",
		".\\TCP20220213.log",
		".\\TCP20220214.log",
		".\\TCP20220215.log",
		".\\TCP20220216.log",
		".\\TCP20220217.log"
	};
	int Index = 0;
	PPGLogFile.open(FileName[Index], ios::in);
	BOOL bFileOpen = PPGLogFile.is_open();
    CHAR Date[20];
    UINT32 TimeStamp = 0;
    UINT32 FirstTimestamp = 0;
    UINT32 Delay;

    while (1)
	{
        if (bFileOpen == TRUE)
        {
            PPGLogFile.getline(buff, MAX_CHARS_PER_LINE);
            PCHAR Ptr = strstr(buff, "Send:");
            if(FirstTimestamp == 0)
                FirstTimestamp = ParseTime(buff, Date);
            if (Ptr)
            {
                PtrOC = strstr(buff, "Act=OC");
                PtrRC = strstr(buff, "Act=RC");
                PtrAN = strstr(buff, "Act=AN");
                PtrHU = strstr(buff, "Act=HU");
                PtrVF = strstr(buff, "Act=VF");
                PtrERC = strstr(buff, "Act=ERC");
                PtrEAN = strstr(buff, "Act=EAN");
                PtrEHU = strstr(buff, "Act=EHU");
                memset(Date, 0x00, 20);
                if (PtrOC)
                {
                    /* 撥出 */
                    DoExternalCallOut(PtrOC);
                    TimeStamp = ParseTime(PtrOC, Date);
                }
                else if (PtrRC)
                {
                    /* 撥入 */
                    DoExternalCallIn(PtrRC);
                    TimeStamp = ParseTime(PtrRC, Date);
                }
                else if (PtrAN)
                {
                    /* 應答 */
                    DoExternalAnswer(PtrAN);
                    TimeStamp = ParseTime(PtrAN, Date);
                }
                else if (PtrHU)
                {
                    /* 掛機 */
                    DoExternalHangup(PtrHU);
                    TimeStamp = ParseTime(PtrHU, Date);
                }
                else if (PtrVF)
                {
                    /* 錄音檔 */
                    DoVoiceRecFileName(PtrVF);
                    TimeStamp = ParseTime(PtrVF, Date);
                }
                else if (PtrERC)
                {
                    /* 分機互打 */
                    DoInternalCallMakeup(PtrERC);
                    TimeStamp = ParseTime(PtrERC, Date);
                }
                else if (PtrEAN)
                {
                    /* 分機通話 */
                    DoInternalCallAnswer(PtrEAN);
                    TimeStamp = ParseTime(PtrEAN, Date);
                }
                else if (PtrEHU)
                {
                    /* 分機掛機 */
                    DoInternalCallHangup(PtrEHU);
                    TimeStamp = ParseTime(PtrEHU, Date);
                }
                Delay = TimeStamp - FirstTimestamp;
            }
            Sleep(Delay * 1);
        }
	}
}

int StartPBXMachineSimulation()
{
    m_MachineID = AfxGetApp()->GetProfileInt("SystemSetting", "MachineID", 1);
	m_hThreadSimulation = (HANDLE)CreateThread(NULL, 5000, (unsigned long(__stdcall*)(void*))ThreadSimulation, NULL, 0, NULL);

	return 0;
}
