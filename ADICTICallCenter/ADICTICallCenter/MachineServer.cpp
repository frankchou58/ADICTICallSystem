#include "pch.h"
//#include <winsock.h>
#include "MachineServer.h"
//#include <iostream.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ADICTICallCenterDlg.h"
#include "CallStateRecordEngine.h"
#include "CDatabaseAccessURL.h"
#include <mstcpip.h>

#define PACKAGE_CALL_BLOCK_NUMS	40

static CADICTICallCenterDlg* pADICTICallCenterDlg;
static CPbxDlg* pPbxDlg;
static CCallRecorderDlg* pCallRecorderDlg;
static CVoiceRecorderDlg* pVoiceRecorderDlg;
static SOCKET CtrlSock;
static struct sockaddr_in	CtrlSa;
static fd_set	readfds;
static HANDLE Thread;

typedef struct MachineCallBlock_Tag
{
	SOCKET	ThisSocket;
	BOOL InUsed;
	PCHAR pPeerId;
	int loop;
	int ConnTimeout;
	int ConnState;
	int MachineID;
	int MachineType;
	CHAR MachineIPAddr[16];
}MachineCallBlock_T, * PMachineCallBlock_T;

static MachineCallBlock_T MachineCallBlock[PACKAGE_CALL_BLOCK_NUMS];

static bool IsMachineLogin(int MachineType, int MachineID)
{
	bool LoginAlready = FALSE;

	for (int i = 0; i < PACKAGE_CALL_BLOCK_NUMS; i++)
	{
		PMachineCallBlock_T pMachineCallBlock = (PMachineCallBlock_T)&MachineCallBlock[i];
		if (pMachineCallBlock->InUsed)
		{
			if (pMachineCallBlock->MachineID == MachineID && pMachineCallBlock->MachineType == MachineType)
			{
				LoginAlready = TRUE;
				break;
			}
		}
	}
	return LoginAlready;
}

static int CheckPortNums(PMachineCallBlock_T pMachineCallBlock, int OutPortNums, int ExtPortNums, int* DBOutPortNums, int* DBXExtPortNums)
{
	int MachineType = pMachineCallBlock->MachineType;
	int MachineID = pMachineCallBlock->MachineID;
	int Ret;
	int CheckResult = PACKAGE_RESPONSE_LOGIN_OK;
	class CDatabaseAccessURL m_DatabaseAccessURL;

	Ret = m_DatabaseAccessURL.GetMachineInfo(MachineType, MachineID, NULL, DBOutPortNums, DBXExtPortNums, NULL);
	if (Ret == 0)
	{
		if (MachineType == MACHINE_TYPE_PBX)
		{
			if (*DBOutPortNums != OutPortNums)
				CheckResult = PACKAGE_RESPONSE_MACHINE_OUTPORTS_NOTMATCH;
			else if(*DBXExtPortNums != ExtPortNums)
				CheckResult = PACKAGE_RESPONSE_MACHINE_EXTPORTS_NOTMATCH;
			else if (*DBOutPortNums == 0 || DBXExtPortNums == 0)
				CheckResult = PACKAGE_RESPONSE_MACHINE_NOT_SETTING;
		}
		else if (MachineType == MACHINE_TYPE_CALLER_ID_BOX || MachineType == MACHINE_TYPE_VOICE_CARD)
		{
			if (*DBOutPortNums != OutPortNums)
				CheckResult = PACKAGE_RESPONSE_MACHINE_OUTPORTS_NOTMATCH;
			else if (*DBOutPortNums == 0)
				CheckResult = PACKAGE_RESPONSE_MACHINE_NOT_SETTING;
		}
	}

	return CheckResult;
}

static PMachineCallBlock_T GetFreeCallBlok()
{
	PMachineCallBlock_T pFreeCallBlock = NULL;
	BOOL Full = TRUE;

	for (int i = 0; i < PACKAGE_CALL_BLOCK_NUMS; i++)
	{
		pFreeCallBlock = (PMachineCallBlock_T)&MachineCallBlock[i];
		if (pFreeCallBlock->InUsed == FALSE)
		{
			Full = FALSE;
			break;
		}
	}
	if(Full == TRUE)
		pFreeCallBlock = NULL;

	return pFreeCallBlock;
}

static int ReleaseCallBlock(PMachineCallBlock_T pFreeMachineCallBlock)
{
	//closesocket(pFreeMachineCallBlock->ThisSocket);
	pFreeMachineCallBlock->ConnState = 0; //Disconnected
	FD_CLR(pFreeMachineCallBlock->ThisSocket, &readfds);
	if (pFreeMachineCallBlock->pPeerId)
		free(pFreeMachineCallBlock->pPeerId);
	memset(pFreeMachineCallBlock, 0, sizeof(MachineCallBlock_T));
	return 0;
}

static UINT32 GetSessionNumbers()
{
	int i;
	PMachineCallBlock_T pFreeCallBlock = NULL;
	UINT32 SessionNumbers = 0;

	for (i = 0; i < PACKAGE_CALL_BLOCK_NUMS; i++)
	{
		pFreeCallBlock = (PMachineCallBlock_T)&MachineCallBlock[i];
		if (pFreeCallBlock->InUsed == FALSE)
			SessionNumbers++;
	}

	return SessionNumbers;
}


static void ThreadCallCenterServerHandler(LPVOID lParam)
{
	int EventID;
	int MachineType;
	int MachineID;
	int OutPortNums;
	int ExtPortNums;
	EventParamsT EventParams;
	int SWVer;
	int PackageHeaderSize;
	int DataLen;
	bool LoginAlready;
	MachineResponsePackage_T MachineResponsePackage;
	int SendLen;
	int Checked;
	static int Loop = 0;
	int Ret;

	PLoginPackage_T pLoginPackage;
	PExternalCallInPackage_T pExternalCallInPackage;
	PExternalCallOutPackage_T pExternalCallOutPackage;
	PExternalCallAnswerPackage_T pExternalCallAnswerPackage;
	PExternalCallHangupPackage_T pExternalCallHangupPackage;
	PCallVoiceRecordPackage_T pCallVoiceRecordPackage;
	PInternalCallMakePackage_T pInternalCallMakePackage;
	PInternalCallAnswerPackage_T pInternalCallAnswerPackage;
	PInternalCallHangupPackage_T pInternalCallHangupPackage;

	while (1)
	{
		fd_set rreadfds = readfds;
		struct timeval	timeout;
		INT32 ret = 0;
		INT RecvLen;
		CHAR ReceivedData[200];
		timeout.tv_sec = 0;
		timeout.tv_usec = 10;
		BOOL Found = FALSE;
		int			SaLen;
		UINT8		in_addr[4];
		SaLen = sizeof CtrlSa;
		SOCKET NewSock;
		UINT32	SessionNumbers;
		SessionNumbers = GetSessionNumbers();
		ret = select(SessionNumbers, &rreadfds, NULL, NULL, &timeout);
		if (ret > 0)
		{
			for (int i = 0; i < PACKAGE_CALL_BLOCK_NUMS; i++)
			{
				PMachineCallBlock_T pMachineCallBlock = (PMachineCallBlock_T)&MachineCallBlock[i];
				if (pMachineCallBlock->InUsed == TRUE)
				{
					if (
						FD_ISSET(pMachineCallBlock->ThisSocket, &rreadfds)
						)
					{
						memset(ReceivedData, 0, 200);
						RecvLen = recv(pMachineCallBlock->ThisSocket, (char*)ReceivedData, 200, 0);
						if (RecvLen > 0)
						{
							printf("RecvLen: %d\n", RecvLen);
							PLoginPackage_T pLoginPackage = (PLoginPackage_T)ReceivedData;
							PMachineHeaderPackage_T pMachineHeader = (PMachineHeaderPackage_T)&pLoginPackage->Header;
							EventID = pMachineHeader->EventID;
							MachineID = pMachineHeader->MachineID;
							MachineType = pMachineHeader->MachineType;
							OutPortNums = pLoginPackage->OutPortNums;
							ExtPortNums = pLoginPackage->ExtPortNums;
							LoginAlready = FALSE;
							if (EventID == PACKAGE_EVENT_TYPE_LOGIN)
							{
								LoginAlready = IsMachineLogin(MachineType, MachineID);
							}
							if (LoginAlready)
							{
								MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_LOGIN_ALREADY;
								SendLen = send(pMachineCallBlock->ThisSocket, (PCHAR)&MachineResponsePackage, sizeof(MachineResponsePackage_T), 0);
								ReleaseCallBlock(pMachineCallBlock);
							}
							else
							{
								switch (EventID)
								{
								case PACKAGE_EVENT_TYPE_LOGIN:
									pMachineCallBlock->MachineID = MachineID;
									pMachineCallBlock->MachineType = MachineType;
									INT DBOutPortNums;
									INT DBXExtPortNums;
									Checked = CheckPortNums(pMachineCallBlock, OutPortNums, ExtPortNums, &DBOutPortNums, &DBXExtPortNums);
									if (Checked != PACKAGE_RESPONSE_LOGIN_OK)
									{
										MachineResponsePackage.ResponseID = Checked;
										MachineResponsePackage.ExtPortNum = DBXExtPortNums;
										MachineResponsePackage.OutPortNum = DBOutPortNums;
									}
									else
									{
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_LOGIN_OK;
										pLoginPackage = (PLoginPackage_T)ReceivedData;
										switch (MachineType)
										{
										case PACKAGE_MACHINE_TYPE_PBX:
											pPbxDlg->MachineLogin(MachineID, pMachineCallBlock->MachineIPAddr, pLoginPackage->SWVer);
											break;
										case PACKAGE_MACHINE_TYPE_CALLERID_BOX:
											pCallRecorderDlg->MachineLogin(MachineID, pMachineCallBlock->MachineIPAddr, pLoginPackage->SWVer);
											break;
										case PACKAGE_MACHINE_TYPE_VOICE_CARD:
											pVoiceRecorderDlg->MachineLogin(MachineID, pMachineCallBlock->MachineIPAddr, pLoginPackage->SWVer);
											break;
										}
									}
									break;
								case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_IN:
									pExternalCallInPackage = (PExternalCallInPackage_T)ReceivedData;
									PackageHeaderSize = sizeof(ExternalCallInPackage_T);
									DataLen = pExternalCallInPackage->Length;
									EventParams.MachineType = MachineType;
									EventParams.MachineID = MachineID;
									EventParams.PhyOutPort = pExternalCallInPackage->OutPortNo;
									EventParams.FromExtNum = 0;
									EventParams.ToExtNum = 0;
									EventParams.Timestamp = pExternalCallInPackage->Timestamp;
									memset(EventParams.TelNo, 0x00, 50);
									memcpy(EventParams.TelNo, &ReceivedData[PackageHeaderSize], DataLen);
									Ret = SendEvent(&EventParams, EVENT_ID_OUTLINE_IN);
									if (Ret >= 0)
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EVENT_OK;
									else
									{
										if (Ret == -1)
											MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EXT_NUMER_NOTEXIST;
										else if (Ret == -2)
											MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_OUTPORT_OVER;
									}
									break;
								case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_OUT:
									pExternalCallOutPackage = (PExternalCallOutPackage_T)ReceivedData;
									PackageHeaderSize = sizeof(ExternalCallOutPackage_T);
									DataLen = pExternalCallOutPackage->Length;
									EventParams.MachineType = MachineType;
									EventParams.MachineID = MachineID;
									EventParams.PhyOutPort = pExternalCallOutPackage->OutPortNo;
									EventParams.FromExtNum = pExternalCallOutPackage->FromExtNum;
									EventParams.ToExtNum = 0;
									EventParams.Timestamp = pExternalCallOutPackage->Timestamp;
									memset(EventParams.TelNo, 0x00, 50);
									memcpy(EventParams.TelNo, &ReceivedData[PackageHeaderSize], DataLen);
									Ret = SendEvent(&EventParams, EVENT_ID_OUTLINE_OUT);
									if (Ret >= 0)
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EVENT_OK;
									else
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EXT_NUMER_NOTEXIST;
									break;
								case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_ANSWER:
									pExternalCallAnswerPackage = (PExternalCallAnswerPackage_T)ReceivedData;
									PackageHeaderSize = sizeof(ExternalCallAnswerPackage_T);
									DataLen = pExternalCallAnswerPackage->Length;
									EventParams.MachineType = MachineType;
									EventParams.MachineID = MachineID;
									EventParams.PhyOutPort = pExternalCallAnswerPackage->OutPortNo;
									EventParams.FromExtNum = pExternalCallAnswerPackage->ExtNum;
									EventParams.ToExtNum = 0;
									EventParams.RingTimes = pExternalCallAnswerPackage->RingTimes;
									EventParams.Timestamp = pExternalCallAnswerPackage->Timestamp;
									memset(EventParams.TelNo, 0x00, 50);
									memcpy(EventParams.TelNo, &ReceivedData[PackageHeaderSize], DataLen);
									Ret = SendEvent(&EventParams, EVENT_ID_OUTLINE_TALKING);
									if (Ret >= 0)
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EVENT_OK;
									else
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EXT_NUMER_NOTEXIST;
									break;
								case PACKAGE_EVENT_TYPE_EXTERNAL_SECOND_DIAL:
									break;
								case PACKAGE_EVENT_TYPE_EXTERNAL_CALL_HANGUP:
									pExternalCallHangupPackage = (PExternalCallHangupPackage_T)ReceivedData;
									PackageHeaderSize = sizeof(PExternalCallHangupPackage_T);
									EventParams.MachineType = MachineType;
									EventParams.MachineID = MachineID;
									EventParams.PhyOutPort = pExternalCallHangupPackage->OutPortNo;
									EventParams.FromExtNum = 0;
									EventParams.ToExtNum = 0;
									EventParams.TalkingDuration = pExternalCallHangupPackage->TalkingDuration;
									EventParams.Timestamp = pExternalCallHangupPackage->Timestamp;
									Ret = SendEvent(&EventParams, EVENT_ID_OUTLINE_DISCONNECT);
									if (Ret >= 0)
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EVENT_OK;
									else
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EXT_NUMER_NOTEXIST;
									break;
								case PACKAGE_EVENT_TYPE_INTERNAL_CALL_MAKE:
									pInternalCallMakePackage = (PInternalCallMakePackage_T)ReceivedData;
									PackageHeaderSize = sizeof(InternalCallMakePackage_T);
									EventParams.MachineType = MachineType;
									EventParams.MachineID = MachineID;
									EventParams.PhyOutPort = 0;
									EventParams.FromExtNum = pInternalCallMakePackage->FromExtNum;
									EventParams.ToExtNum = pInternalCallMakePackage->ToExtNum;
									EventParams.Timestamp = pInternalCallMakePackage->Timestamp;
									Ret = SendEvent(&EventParams, EVENT_ID_EXTLINE_MAKE);
									if (Ret >= 0)
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EVENT_OK;
									else
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EXT_NUMER_NOTEXIST;
									break;
								case PACKAGE_EVENT_TYPE_INTERNAL_CALL_ANSWER:
									pInternalCallAnswerPackage = (PInternalCallAnswerPackage_T)ReceivedData;
									PackageHeaderSize = sizeof(InternalCallAnswerPackage_T);
									EventParams.MachineType = MachineType;
									EventParams.MachineID = MachineID;
									EventParams.PhyOutPort = 0;
									EventParams.FromExtNum = pInternalCallAnswerPackage->FromExtNum;
									EventParams.ToExtNum = pInternalCallAnswerPackage->ToExtNum;
									EventParams.Timestamp = pInternalCallAnswerPackage->Timestamp;
									EventParams.RingTimes = pInternalCallAnswerPackage->RingTimes;
									Ret = SendEvent(&EventParams, EVENT_ID_EXTLINE_TALKING);
									if (Ret >= 0)
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EVENT_OK;
									else
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EXT_NUMER_NOTEXIST;
									break;
								case PACKAGE_EVENT_TYPE_INTERNAL_SECOND_DIAL:
									break;
								case PACKAGE_EVENT_TYPE_INTERNAL_CALL_HANGUP:
									pInternalCallHangupPackage = (PInternalCallHangupPackage_T)ReceivedData;
									PackageHeaderSize = sizeof(InternalCallHangupPackage_T);
									EventParams.MachineType = MachineType;
									EventParams.MachineID = MachineID;
									EventParams.PhyOutPort = 0;
									EventParams.FromExtNum = pInternalCallHangupPackage->FromExtNum;
									EventParams.ToExtNum = pInternalCallHangupPackage->ToExtNum;
									EventParams.Timestamp = pInternalCallHangupPackage->Timestamp;
									EventParams.TalkingDuration = pInternalCallHangupPackage->Duration;
									Ret = SendEvent(&EventParams, EVENT_ID_EXTLINE_DISCONNECT);
									if (Ret >= 0)
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EVENT_OK;
									else
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EXT_NUMER_NOTEXIST;
									break;
								case PACKAGE_EVENT_TYPE_VOICE_REC_FILENAME:
									pCallVoiceRecordPackage = (PCallVoiceRecordPackage_T)ReceivedData;
									DataLen = pCallVoiceRecordPackage->Length;
									PackageHeaderSize = sizeof(CallVoiceRecordPackage_T);
									EventParams.MachineType = MachineType;
									EventParams.MachineID = MachineID;
									EventParams.PhyOutPort = pCallVoiceRecordPackage->OutPortNo;
									EventParams.FromExtNum = pCallVoiceRecordPackage->FromExtNumber;
									EventParams.ToExtNum = pCallVoiceRecordPackage->ToExtNumber;
									EventParams.Timestamp = pCallVoiceRecordPackage->Timestamp;
									memset(EventParams.VoiceRecFileName, 0x00, 33);
									memcpy(EventParams.VoiceRecFileName, &ReceivedData[PackageHeaderSize], DataLen);
									Ret = SendEvent(&EventParams, EVENT_ID_VOICE_REC_FILE_UUID);
									if (Ret >= 0)
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EVENT_OK;
									else
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_EXT_NUMER_NOTEXIST;
									break;
								}
								if (EventID != PACKAGE_EVENT_TYPE_LOGIN)
								{
									MachineResponsePackage.EventID = EventID;
									if (pMachineCallBlock->MachineType != MachineType)
										MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_MACHINE_TYPE_NOTMATCH;
									SendLen = send(pMachineCallBlock->ThisSocket, (PCHAR)&MachineResponsePackage, sizeof(MachineResponsePackage_T), 0);
									if (pMachineCallBlock->MachineType != MachineType)
									{
										ReleaseCallStateBlock(MachineType, MachineID);
										ReleaseCallBlock(pMachineCallBlock);
									}
								}
								else
								{
									MachineResponsePackage.EventID = EventID;
									SendLen = send(pMachineCallBlock->ThisSocket, (PCHAR)&MachineResponsePackage, sizeof(MachineResponsePackage_T), 0);
									if (MachineResponsePackage.ResponseID != PACKAGE_RESPONSE_LOGIN_OK)
										ReleaseCallBlock(pMachineCallBlock);
								}
							}
						}
						else
						{
							printf("#################Disconnected Socket:%d!!!\n", pMachineCallBlock->ThisSocket);
							printf("ConnState: %d\n", pMachineCallBlock->ConnState);
							MachineID = pMachineCallBlock->MachineID;
							MachineType = pMachineCallBlock->MachineType;
							switch (MachineType)
							{
							case PACKAGE_MACHINE_TYPE_PBX:
								pPbxDlg->MachineLogout(MachineID);
								break;
							case PACKAGE_MACHINE_TYPE_CALLERID_BOX:
								pCallRecorderDlg->MachineLogout(MachineID);
								break;
							case PACKAGE_MACHINE_TYPE_VOICE_CARD:
								pVoiceRecorderDlg->MachineLogout(MachineID);
								break;
							}
							ReleaseCallStateBlock(MachineType, MachineID);
							ReleaseCallBlock(pMachineCallBlock);
						}
					}
				}
			}
			if (
				FD_ISSET(CtrlSock, &rreadfds)
				)
			{
				int			SaLen;
				UINT8		in_addr[4];
				SaLen = sizeof CtrlSa;
				SOCKET NewSock;
				NewSock = accept(CtrlSock, (struct sockaddr*)(&(CtrlSa)), &SaLen);
				if (NewSock)
				{
					printf("#################New Connection: %d\n", NewSock);
					PMachineCallBlock_T pMachineCallBlock;
					pMachineCallBlock = GetFreeCallBlok();
					if (pMachineCallBlock)
					{
						pMachineCallBlock->ConnState = 1; //Connecting
						pMachineCallBlock->InUsed = TRUE;
						pMachineCallBlock->ThisSocket = NewSock;
						printf("ConnState: %d\n", pMachineCallBlock->ConnState);
						sprintf_s(pMachineCallBlock->MachineIPAddr, "%03d.%03d.%03d.%03d",
							CtrlSa.sin_addr.S_un.S_un_b.s_b1, CtrlSa.sin_addr.S_un.S_un_b.s_b2,
							CtrlSa.sin_addr.S_un.S_un_b.s_b3, CtrlSa.sin_addr.S_un.S_un_b.s_b4);
						FD_SET(NewSock, &readfds);
					}
					else
					{
						MachineResponsePackage.ResponseID = PACKAGE_RESPONSE_OVER_MACHINE_CALLBLOCK_NUMS;
						SendLen = send(NewSock, (PCHAR)&MachineResponsePackage, sizeof(MachineResponsePackage_T), 0);
						closesocket(NewSock);
					}
				}
			}
		}
	}
}

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

int GetLocalNetworkDevice(PCHAR pMessage)
{
	SOCKET sd = WSASocket(AF_INET, SOCK_DGRAM, 0, 0, 0, 0);
	if (sd == SOCKET_ERROR) {
		printf("Failed to get a socket. Error %d\n", WSAGetLastError());
		return -1;
	}

	INTERFACE_INFO InterfaceList[20];
	unsigned long nBytesReturned;
	if (WSAIoctl(sd, SIO_GET_INTERFACE_LIST, 0, 0, &InterfaceList,
		sizeof(InterfaceList), &nBytesReturned, 0, 0) == SOCKET_ERROR) {
		printf("Failed calling WSAIoctl: error %d\n", WSAGetLastError());
		return -2;
	}

	int nNumInterfaces = nBytesReturned / sizeof(INTERFACE_INFO);
	int Len = 0;
	Len += sprintf_s(pMessage + Len, 1000 - Len, "There are %d interfaces:\n", nNumInterfaces);
	for (int i = 0; i < nNumInterfaces; ++i) {
		sockaddr_in* pAddress;
		pAddress = (sockaddr_in*)&(InterfaceList[i].iiAddress);
		Len += sprintf_s(pMessage + Len, 1000 - Len, " %s", inet_ntoa(pAddress->sin_addr));
		pAddress = (sockaddr_in*)&(InterfaceList[i].iiBroadcastAddress);
		Len += sprintf_s(pMessage + Len, 1000 - Len, " has bcast ", inet_ntoa(pAddress->sin_addr));
		pAddress = (sockaddr_in*)&(InterfaceList[i].iiNetmask);
		Len += sprintf_s(pMessage + Len, 1000 - Len, " and netmask ", inet_ntoa(pAddress->sin_addr));
#if 0
		cout << " Iface is ";
		u_long nFlags = InterfaceList[i].iiFlags;
		if (nFlags & IFF_UP) cout << "up";
		else                 cout << "down";
		if (nFlags & IFF_POINTTOPOINT) cout << ", is point-to-point";
		if (nFlags & IFF_LOOPBACK)     cout << ", is a loopback iface";
		cout << ", and can do: ";
		if (nFlags & IFF_BROADCAST) cout << "bcast ";
		if (nFlags & IFF_MULTICAST) cout << "multicast ";
		cout << endl;
#endif
		Len += sprintf_s(pMessage + Len, 1000 - Len, "\n");
	}

	return 0;
}

static int Create_TCP_Sock(unsigned int tcpport, char* ip_address)
{
	int One = 1;

	/*
	* Create the control socket.
	*/
	CtrlSock = socket(AF_INET, SOCK_STREAM, 0);
	if (CtrlSock < 0)
	{
		printf("Can't create control socket\n");
		return -1;
	}

	if (setsockopt(CtrlSock, SOL_SOCKET, SO_KEEPALIVE, (char*)(&One), sizeof(One)) != 0)
	{
		printf("Can't set socket option, errno : %d\n", errno);
		closesocket(CtrlSock);
		return -1;
	}

	// set KeepAlive parameter 
	tcp_keepalive alive_in;
	tcp_keepalive alive_out;
	alive_in.keepalivetime = 500; // 0.5s 
	alive_in.keepaliveinterval = 1000; // 1s 
	alive_in.onoff = TRUE;
	unsigned long ulBytesReturn = 0;
	int nRet = WSAIoctl(CtrlSock, SIO_KEEPALIVE_VALS, &alive_in, sizeof(alive_in),
		&alive_out, sizeof(alive_out), &ulBytesReturn, NULL, NULL);
	if (nRet == SOCKET_ERROR)
	{
		printf(" WSAIoctl failed: %d/n ", WSAGetLastError());
		return FALSE;
	}
	memset(&(CtrlSa), 0, sizeof(CtrlSa));
	//	CtrlSa.sin_len = sizeof( struct sockaddr_in );
	CtrlSa.sin_family = AF_INET;
	CtrlSa.sin_addr.s_addr = INADDR_ANY;
	CtrlSa.sin_port = htons(tcpport);
	if (bind(CtrlSock, (struct sockaddr*)(&CtrlSa), sizeof(CtrlSa)) < 0)
	{
		printf("Can't bind control socket, errno : %d\n", errno);
		closesocket(CtrlSock);
		return -1;
	}
	listen(CtrlSock, 4);

	FD_ZERO(&readfds);
	FD_SET(CtrlSock, &readfds);

	return 0;
}


int OpenMachineServer()
{
	int Ret = -1;

	//Ret = Start_Winsock();
	//if (Ret == 0)
	{
		CHAR IP[100] = "0.0.0.0";
		int ret = Create_TCP_Sock(3015, IP);
		if (ret == 0)
		{
			pADICTICallCenterDlg = (CADICTICallCenterDlg*)AfxGetMainWnd();
			pPbxDlg = (CPbxDlg*)pADICTICallCenterDlg->m_pWndModleDlg[CTRL_ID_PBX_TYPE_UI];
			//pCallRecorderDlg = (CCallRecorderDlg*)pADICTICallCenterDlg->m_pWndModleDlg[CTRL_ID_CALL_RECORDER_TYPE_UI];
			//pVoiceRecorderDlg = (CVoiceRecorderDlg*)pADICTICallCenterDlg->m_pWndModleDlg[CTRL_ID_VOICE_RECORDER_TYPE_UI];
			Thread = (HANDLE)CreateThread(NULL, 2000, (unsigned long(__stdcall*)(void*))ThreadCallCenterServerHandler, NULL, 0, NULL);
		}
	}
	return Ret;
}

