#include "pch.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include "MachineClient.h"
#include "Socket.h"

SOCKET SendCtrlSock = INVALID_SOCKET;
struct sockaddr_in	CtrlSa;
bool bStartServer = FALSE;
struct sockaddr_in serverAdr;

int Start_Winsock()
{
	WSAData m_wsaData;

	if (WSAStartup(0x0101, (LPWSADATA)&m_wsaData))
	{
		printf("Error: Can't open winsock\n");
		return -1;
	}

	return 0;
}

int Stop_Winsock()
{
	WSACleanup();
	return 0;
}

int Create_TCP_Sock(unsigned int tcpport, const char* ip_address)
{
	// 保險起見：萬一呼叫端忘記先關閉舊連線，這裡先關掉，避免舊的
	// SOCKET handle 被蓋掉、變成永遠關不掉的孤兒 handle。
	Close_TCP_Sock();

	int				ip_lan, found;
	int				its_domain_name = 0;
	char			IPCharacters[] = { '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '.' };
	int j;
	struct			hostent* Server_ip;
	int				ret;

	/* check input is domain name or not */
	ip_lan = strlen(ip_address);
	if (ip_lan > 15)
	{
		its_domain_name = 1;
	}
	if (ip_lan == 0)
	{
		return -1;
	}
	else
	{
		for (int i = 0; i < ip_lan; i++)
		{
			for (j = 0; j < 15; j++)
			{
				if (ip_address[i] == IPCharacters[j])
				{
					found = 1;
					break;
				}
			}
			if (j == 15)
			{
				its_domain_name = 1;
				break;
			}
		}
	}

	int One = 1;
	/*
	* Create the control socket.
	*/
	SendCtrlSock = socket(AF_INET, SOCK_STREAM, 0);
	if (SendCtrlSock < 0)
	{
		printf("Can't create control socket\n");
		return -1;
	}

	if (setsockopt(SendCtrlSock, SOL_SOCKET, SO_KEEPALIVE, (char*)(&One), sizeof(One)) != 0)
	{
		printf("Can't set socket option, errno : %d\n", errno);
		closesocket(SendCtrlSock);
		return -1;
	}

	in_addr InAddr;
	memset(&serverAdr, 0, sizeof serverAdr);
	serverAdr.sin_family = AF_INET;
	serverAdr.sin_port = htons(tcpport);
	if (its_domain_name)
	{
		Server_ip = gethostbyname(ip_address);
		memcpy(&serverAdr.sin_addr, Server_ip->h_addr, Server_ip->h_length);
	}
	else
	{
		serverAdr.sin_addr.s_addr = inet_addr(ip_address);
	}
	memset(&(CtrlSa), 0, sizeof(CtrlSa));

	ret = connect(SendCtrlSock, (SOCKADDR*)&serverAdr, sizeof(SOCKADDR_IN));

	return ret;
}

int SendLogin(int MachineType, int MachineID, int OutPortNums, int ExtPortNums)
{
	LoginPackage_T Login;

	Login.Header.EventID = PACKAGE_EVENT_TYPE_LOGIN;
	Login.Header.MachineID = MachineID;
	Login.Header.MachineType = MachineType;
	Login.SWVer = 0x01;
	Login.OutPortNums = OutPortNums;
	Login.ExtPortNums = ExtPortNums;

	int SendLen = send(SendCtrlSock, (char*)&Login, sizeof(LoginPackage_T), 0);

	return SendLen;
}

int SendExternalCallIn(int MachineType, int MachineID, UINT32 Timestamp, int OutPortNo, PCHAR pCallerID)
{
	int Len;
	int SendLen;
	PCHAR pSendBuff;

	Len = strlen(pCallerID);
	SendLen = sizeof(ExternalCallInPackage_T) + Len;
	pSendBuff = (PCHAR)malloc(SendLen);

	ExternalCallInPackage_T ExternalCallInPackage;

	memset(&ExternalCallInPackage, 0x00, sizeof(ExternalCallInPackage_T));
	ExternalCallInPackage.Header.EventID = PACKAGE_EVENT_TYPE_EXTERNAL_CALL_IN;
	ExternalCallInPackage.Header.MachineID = MachineID;
	//ExternalCallInPackage.Header.MachineType = MachineType;
	ExternalCallInPackage.Header.MachineType = MachineType;
	ExternalCallInPackage.Timestamp = Timestamp;
	ExternalCallInPackage.OutPortNo = OutPortNo;
	ExternalCallInPackage.Length = Len;
	memset(pSendBuff, 0x00, SendLen);
	memcpy(pSendBuff, &ExternalCallInPackage, sizeof(ExternalCallInPackage_T));
	memcpy(pSendBuff + sizeof(ExternalCallInPackage_T), pCallerID, Len);

	SendLen = send(SendCtrlSock, (char*)pSendBuff, SendLen, 0);
	free(pSendBuff);

	return SendLen;
}

int SendExternalCallOut(int MachineType, int MachineID, UINT32 Timestamp, int OutPortNo, int FromExtNum, PCHAR pCalleeID)
{
	int Len = 0;
	int SendLen;
	PCHAR pSendBuff;

	if(pCalleeID)
		Len = strlen(pCalleeID);
	SendLen = sizeof(ExternalCallOutPackage_T) + Len;
	pSendBuff = (PCHAR)malloc(SendLen);

	ExternalCallOutPackage_T ExternalCallOutPackage;

	memset(&ExternalCallOutPackage, 0x00, sizeof(ExternalCallOutPackage_T));
	ExternalCallOutPackage.Header.EventID = PACKAGE_EVENT_TYPE_EXTERNAL_CALL_OUT;
	ExternalCallOutPackage.Header.MachineID = MachineID;
	ExternalCallOutPackage.Header.MachineType = MachineType;
	ExternalCallOutPackage.Timestamp = Timestamp;
	ExternalCallOutPackage.OutPortNo = OutPortNo;
	ExternalCallOutPackage.FromExtNum = FromExtNum;
	ExternalCallOutPackage.Length = Len;
	memset(pSendBuff, 0x00, SendLen);
	memcpy(pSendBuff, &ExternalCallOutPackage, sizeof(ExternalCallOutPackage_T));
	if(Len > 0)
		memcpy(pSendBuff + sizeof(ExternalCallOutPackage_T), pCalleeID, Len);

	SendLen = send(SendCtrlSock, (char*)pSendBuff, SendLen, 0);
	free(pSendBuff);

	return SendLen;
}

int SendExternalCallAnswer(int MachineType, int MachineID, UINT32 Timestamp, int OutPortNo, int FromExtNum, int RingTimes, PCHAR pCallId)
{
	int Len = 0;
	int SendLen;
	PCHAR pSendBuff;

	if(pCallId)
		Len = strlen(pCallId);
	SendLen = sizeof(ExternalCallAnswerPackage_T) + Len;
	pSendBuff = (PCHAR)malloc(SendLen);

	ExternalCallAnswerPackage_T ExternalCallAnswerPackage;

	memset(&ExternalCallAnswerPackage, 0x00, sizeof(ExternalCallAnswerPackage_T));
	ExternalCallAnswerPackage.Header.EventID = PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER;
	ExternalCallAnswerPackage.Header.MachineID = MachineID;
	ExternalCallAnswerPackage.Header.MachineType = MachineType;
	ExternalCallAnswerPackage.Timestamp = Timestamp;
	ExternalCallAnswerPackage.OutPortNo = OutPortNo;
	ExternalCallAnswerPackage.Length = Len;
	memset(pSendBuff, 0x00, SendLen);
	memcpy(pSendBuff, &ExternalCallAnswerPackage, sizeof(ExternalCallAnswerPackage_T));
	if(Len > 0)
		memcpy(pSendBuff + sizeof(ExternalCallAnswerPackage_T), pCallId, Len);

	SendLen = send(SendCtrlSock, (char*)pSendBuff, SendLen, 0);
	free(pSendBuff);

	return SendLen;
}

int SendExternalCallHangup(int MachineType, int MachineID, UINT32 Timestamp, int OutPortNo, int TalkingDuration)
{
	int Len;
	int SendLen;
	PCHAR pSendBuff;

	ExternalCallHangupPackage_T ExternalCallHangupPackage;

	SendLen = sizeof(ExternalCallHangupPackage_T);
	memset(&ExternalCallHangupPackage, 0x00, SendLen);
	ExternalCallHangupPackage.Header.EventID = PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP;
	ExternalCallHangupPackage.Header.MachineID = MachineID;
	ExternalCallHangupPackage.Header.MachineType = MachineType;
	ExternalCallHangupPackage.Timestamp = Timestamp;
	ExternalCallHangupPackage.OutPortNo = OutPortNo;
	ExternalCallHangupPackage.TalkingDuration = TalkingDuration;

	SendLen = send(SendCtrlSock, (char*)&ExternalCallHangupPackage, SendLen, 0);

	return SendLen;
}

int SendVoiceFileName(int MachineType, int MachineID, UINT32 Timestamp, int OutPortNo, int ToExtNumber, PCHAR pVoiceFileName)
{
	int Len;
	int SendLen;
	PCHAR pSendBuff;
	CallVoiceRecordPackage_T CallVoiceRecordPackage;

	Len = strlen(pVoiceFileName);
	SendLen = sizeof(CallVoiceRecordPackage_T) + Len;
	pSendBuff = (PCHAR)malloc(SendLen);


	memset(&CallVoiceRecordPackage, 0x00, sizeof(CallVoiceRecordPackage_T));
	CallVoiceRecordPackage.Header.EventID = PACKAGE_EVENT_TYPE_VOICE_REC_FILENAME;
	CallVoiceRecordPackage.Header.MachineID = MachineID;
	CallVoiceRecordPackage.Header.MachineType = MachineType;
	CallVoiceRecordPackage.Timestamp = Timestamp;
	CallVoiceRecordPackage.OutPortNo = OutPortNo;
	CallVoiceRecordPackage.ToExtNumber = ToExtNumber;
	CallVoiceRecordPackage.FromExtNumber = 0;
	CallVoiceRecordPackage.Length = Len;
	memset(pSendBuff, 0x00, SendLen);
	memcpy(pSendBuff, &CallVoiceRecordPackage, sizeof(CallVoiceRecordPackage_T));
	memcpy(pSendBuff + sizeof(CallVoiceRecordPackage_T), pVoiceFileName, Len);

	SendLen = send(SendCtrlSock, (char*)pSendBuff, SendLen, 0);
	free(pSendBuff);

	return SendLen;
}

int SendInternalCallMake(int MachineType, int MachineID, UINT32 Timestamp, int FromExtNumber, int ToExtNumber)
{
	int Len;
	int SendLen;
	PCHAR pSendBuff;

	InternalCallMakePackage_T InternalCallMakePackage;

	SendLen = sizeof(InternalCallMakePackage_T);
	memset(&InternalCallMakePackage, 0x00, SendLen);
	InternalCallMakePackage.Header.EventID = PACKAGE_EVENT_TYPE_INTERNAL_CALL_MAKE;
	InternalCallMakePackage.Header.MachineID = MachineID;
	InternalCallMakePackage.Header.MachineType = MachineType;
	InternalCallMakePackage.Timestamp = Timestamp;
	InternalCallMakePackage.ToExtNum = ToExtNumber;
	InternalCallMakePackage.FromExtNum = FromExtNumber;

	SendLen = send(SendCtrlSock, (char*)&InternalCallMakePackage, SendLen, 0);

	return SendLen;
}

int SendInternalCallAnswer(int MachineType, int MachineID, UINT32 Timestamp, int FromExtNumber, int ToExtNumber, int RingTimes)
{
	int Len;
	int SendLen;
	PCHAR pSendBuff;

	InternalCallAnswerPackage_T InternalCallAnswerPackage;

	SendLen = sizeof(InternalCallAnswerPackage_T);
	memset(&InternalCallAnswerPackage, 0x00, SendLen);
	InternalCallAnswerPackage.Header.EventID = PACKAGE_EVENT_TYPE_INTERNAL_CALL_ANSWER;
	InternalCallAnswerPackage.Header.MachineID = MachineID;
	InternalCallAnswerPackage.Header.MachineType = MachineType;
	InternalCallAnswerPackage.Timestamp = Timestamp;
	InternalCallAnswerPackage.ToExtNum = ToExtNumber;
	InternalCallAnswerPackage.FromExtNum = FromExtNumber;
	InternalCallAnswerPackage.RingTimes = RingTimes;

	SendLen = send(SendCtrlSock, (char*)&InternalCallAnswerPackage, SendLen, 0);

	return SendLen;
}

int SendInternalCallHangup(int MachineType, int MachineID, UINT32 Timestamp, int FromExtNumber, int ToExtNumber, int TalkingDuration)
{
	int Len;
	int SendLen;
	PCHAR pSendBuff;

	InternalCallHangupPackage_T InternalCallHangupPackage;

	SendLen = sizeof(InternalCallHangupPackage_T);
	memset(&InternalCallHangupPackage, 0x00, SendLen);
	InternalCallHangupPackage.Header.EventID = PACKAGE_EVENT_TYPE_INTERNAL_CALL_HANGUP;
	InternalCallHangupPackage.Header.MachineID = MachineID;
	InternalCallHangupPackage.Header.MachineType = MachineType;
	InternalCallHangupPackage.Timestamp = Timestamp;
	InternalCallHangupPackage.ToExtNum = ToExtNumber;
	InternalCallHangupPackage.FromExtNum = FromExtNumber;
	InternalCallHangupPackage.Duration = TalkingDuration;

	SendLen = send(SendCtrlSock, (char*)&InternalCallHangupPackage, SendLen, 0);

	return SendLen;
}

void Close_TCP_Sock()
{
	// 讓卡在 RecvResp() 的背景執行緒自然收到錯誤而結束迴圈，不用強制
	// TerminateThread（那個很危險，可能在拿著鎖或配置記憶體的當下砍執行緒）。
	if (SendCtrlSock != INVALID_SOCKET)
	{
		closesocket(SendCtrlSock);
		SendCtrlSock = INVALID_SOCKET;
	}
}

int RecvResp(PCHAR pMachineResponsePackage)
{
	INT32 SelectRet = 0;
	INT RecvLen;

	RecvLen = recv(SendCtrlSock, (char*)pMachineResponsePackage, sizeof(MachineResponsePackage_T), 0);

	return RecvLen;
}

