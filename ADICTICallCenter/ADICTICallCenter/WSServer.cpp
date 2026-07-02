#include "pch.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "sha1.hpp"
#include "base64.h"
#include "ADICTICallCenterDlg.h"
#include "CallStateRecordEngine.h"
#include "CDatabaseAccessURL.h"

/* Frank Debug Git */
/* Second Test Git */
static CADICTICallCenterDlg* pADICTICallCenterDlg;
static CPbxDlg* pPbxDlg;
static CCallRecorderDlg* pCallRecorderDlg;
static CVoiceRecorderDlg* pVoiceRecorderDlg;

static SOCKET CtrlSock;
static struct sockaddr_in	CtrlSa;
static fd_set	readfds;
static HANDLE Thread;

typedef struct WSCallBlock_Tag
{
	SOCKET	ThisSocket;
	BOOL InUsed;
	//PCHAR pPeerId;
	int loop;
	int ConnTimeout;
	int ConnState;
	// 舊版是 32 字元的永久 UUID 才夠用；新版 API 的登入 Token 是
	// bin2hex(random_bytes(32)) = 64 個十六進位字元，這裡要放得下
	// 才不會被截斷成無效字串（截斷後 GetOperatorInfo() 一定查不到人）。
	CHAR OperatorUUID[72];
}WSCallBlock_T, * PWSCallBlock_T;

WSCallBlock_T WSCallBlock[100];

static UINT32 GetSessionNumbers()
{
	int i;
	PWSCallBlock_T pFreeCallBlock = NULL;
	UINT32 SessionNumbers = 0;

	for (i = 0; i < 100; i++)
	{
		pFreeCallBlock = (PWSCallBlock_T)&WSCallBlock[i];
		if (pFreeCallBlock->InUsed == FALSE)
			SessionNumbers++;
	}

	return SessionNumbers;
}

static PWSCallBlock_T GetFreeCallBlok()
{
	PWSCallBlock_T pFreeCallBlock = NULL;
	BOOL Full = TRUE;

	for (int i = 0; i < 100; i++)
	{
		pFreeCallBlock = (PWSCallBlock_T)&WSCallBlock[i];
		if (pFreeCallBlock->InUsed == FALSE)
		{
			Full = FALSE;
			break;
		}
	}
	if (Full == TRUE)
		pFreeCallBlock = NULL;

	return pFreeCallBlock;
}

static PWSCallBlock_T FindWSCallBlokByMachineID(int MachineID)
{
	class CDatabaseAccessURL m_DatabaseAccessURL;
	PWSCallBlock_T pWSCallBlock_T = NULL;
	BOOL Found = FALSE;
	OperatorInfo_T OperatorInfo;
	int Ret = 0;

	for (UINT16 i = 0; i < 100; i++)
	{
		pWSCallBlock_T = (PWSCallBlock_T)&WSCallBlock[i];
		if (pWSCallBlock_T->InUsed == TRUE)
		{
			memset(&OperatorInfo, 0x00, sizeof(OperatorInfo_T));
			Ret = m_DatabaseAccessURL.GetOperatorInfo(pWSCallBlock_T->OperatorUUID, &OperatorInfo);
			if (Ret == 0)
			{
				if (OperatorInfo.MachineID == MachineID)
				{
					Found = TRUE;
					break;
				}
			}
		}
	}
	if (Found == FALSE)
		pWSCallBlock_T = NULL;

	return pWSCallBlock_T;
}

static int ReleaseCallBlock(PWSCallBlock_T pFreeCallBlock)
{
	closesocket(pFreeCallBlock->ThisSocket);
	FD_CLR(pFreeCallBlock->ThisSocket, &readfds);
	memset(pFreeCallBlock, 0, sizeof(WSCallBlock_T));
	return 0;
}

static PWSCallBlock_T GetCallBlockBySocket(SOCKET ThisSocket)
{
	PWSCallBlock_T pFreeCallBlock = NULL;
	for (int i = 0; i < 100; i++)
	{
		pFreeCallBlock = (PWSCallBlock_T)&WSCallBlock[i];
		if (pFreeCallBlock->ThisSocket == ThisSocket)
		{
			return pFreeCallBlock;
		}
	}
	return NULL;
}

typedef struct Header_Tag
{
	CHAR HeaderName[50];
	CHAR HeaderValue[100];
} Header_T, * PHeader_T;

Header_T RecvHeaderFields[] = {
	{"GET / HTTP", ""},
	{"Host: ", ""},
	{"Connection: ", ""},
	{"Upgrade: ", ""},
	{"Origin: ", ""},
	{"Sec-WebSocket-Version: ", ""},
	{"Sec-WebSocket-Key: ", ""},
	{"Sec-WebSocket-Extensions: ", ""},
};

Header_T SendHeaderFields[] = {
	{"HTTP/1.1 101 Switching Protocols", ""},
	{"Sec-WebSocket-Accept: ", ""},
};

static bool ParseHeader(PCHAR RawData)
{
	PCHAR StartPtr;
	PCHAR EndPtr;
	PCHAR pFound;
	PCHAR HeaderFieldLine = (PCHAR)malloc(2000);
	int Len;
	int HeaderLen;
	int HerderNumers = 8;
	bool IsHttp = FALSE;

	StartPtr = RawData;
	while (1)
	{
		EndPtr = strstr(StartPtr, "\r\n");
		if (EndPtr == NULL)
			break;
		Len = EndPtr - StartPtr;
		memset(HeaderFieldLine, 0, 2000);
		memcpy(HeaderFieldLine, StartPtr, Len);
		for (int i = 0; i < HerderNumers; i++)
		{
			pFound = strstr(HeaderFieldLine, RecvHeaderFields[i].HeaderName);
			if (pFound)
			{
				HeaderLen = strlen(RecvHeaderFields[i].HeaderName);
				memset(RecvHeaderFields[i].HeaderValue, 0, 100);
				strcpy_s(RecvHeaderFields[i].HeaderValue, &HeaderFieldLine[HeaderLen]);
				IsHttp = TRUE;
				break;
			}
		}
		StartPtr = EndPtr + strlen("\r\n");
	}
	free(HeaderFieldLine);

	return IsHttp;
}
static char const handshake_guid[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

static int CalAcceptKey(std::string pKey, PCHAR pAcceptKey)
{
	pKey.append(handshake_guid);

	SHA1 checksum;
	checksum.update(pKey);
	std::string Digest = checksum.final();

	int Len = Digest.length();
	PCHAR pDigest = (PCHAR)Digest.c_str();
	BYTE Di[40];
	BYTE OneByte;
	int Index = 0;
	for (int i = 0; i < Len; i += 2)
	{
		OneByte = pDigest[i];
		if (OneByte >= 0x30 && OneByte <= 0x39)
			OneByte -= 0x30;
		else if (OneByte >= 0x61 && OneByte <= 0x66)
			OneByte -= 0x57;
		else if (OneByte >= 0x41 && OneByte <= 0x46)
			OneByte -= 0x37;
		Di[Index] = OneByte << 4;
		OneByte = pDigest[i + 1];
		if (OneByte >= 0x30 && OneByte <= 0x39)
			OneByte -= 0x30;
		else if (OneByte >= 0x61 && OneByte <= 0x66)
			OneByte -= 0x57;
		else if (OneByte >= 0x41 && OneByte <= 0x46)
			OneByte -= 0x37;
		Di[Index] |= OneByte;
		Index++;
	}
	std::string key = base64_encode((PBYTE)Di, 20);

	char* c_string_copy = new char[Digest.length() + 1];
	memmove(c_string_copy, key.c_str(), key.length());
	memcpy(pAcceptKey, c_string_copy, key.length());

	return 0;
}

typedef struct WebsocketFrameSeg_Tag
{
	BYTE FrameCtrl;
	BYTE PayloadLength;
} WebsocketFrameSeg_T, * PWebsocketFrameSeg_T;

typedef struct WebsocketFramePayload_Tag
{
	UINT32 MaskingKey;
	PBYTE pMaskedPayload;
	PBYTE pPayload;
} WebsocketFramePayload_T, * PWebsocketFramePayload_T;

static int SendWebSocketPayload(SOCKET ThisSock, PCHAR pPayload, int PayloadLen)
{
	int WebsocketFrameLen = sizeof(WebsocketFrameSeg_T);
	int Ret;
	//PCHAR pSendPayload = (PCHAR)malloc(PayloadLen + WebsocketFrameLen);
	CHAR pSendPayload[1000];
	WebsocketFrameSeg_T WebsocketFrame;
	if (pSendPayload)
	{
		WebsocketFrame.FrameCtrl = 0x81;
		//WebsocketFrame.FrameCtrl = 0x82;
		WebsocketFrame.PayloadLength = PayloadLen & ~0x80;
		memcpy(pSendPayload, &WebsocketFrame, WebsocketFrameLen);
		memcpy(pSendPayload + WebsocketFrameLen, pPayload, PayloadLen);
		int SendLen = send(ThisSock, (char*)&pSendPayload, PayloadLen + WebsocketFrameLen, 0);
		if (SendLen > 0)
			Ret = 0;
		else
		{
			printf("SendLen: %d\n", SendLen);
			printf("ThisSock: %d\n", ThisSock);
			Ret = -2;
		}
	}
	else
	{
		Ret = -1;
	}

	return Ret;
}

static void ThreadWSServerHandler(LPVOID lParam)
{
	while (1)
	{
		fd_set rreadfds = readfds;
		struct timeval	timeout;
		INT32 ret = 0;
		INT RecvLen;
		CHAR SendRespData[10000];
		CHAR ReceivedData[10000];
		timeout.tv_sec = 0;
		timeout.tv_usec = 10000;
		BOOL Found = FALSE;

		UINT32	SessionNumbers;
		SessionNumbers = GetSessionNumbers();
		ret = select(SessionNumbers, &rreadfds, NULL, NULL, &timeout);
		//printf("ret: %d\n", ret);
		if (ret > 0)
		{
			for (int i = 0; i < 100; i++)
			{
				PWSCallBlock_T pCallBlock = (PWSCallBlock_T)&WSCallBlock[i];
				if (pCallBlock->InUsed == TRUE)
				{
					if (
						FD_ISSET(pCallBlock->ThisSocket, &rreadfds)
						)
					{
						memset(ReceivedData, 0, 10000);
						RecvLen = recv(pCallBlock->ThisSocket, (char*)ReceivedData, 10000, 0);
						if (RecvLen > 0)
						{
							printf("RecvLen: %d\n", RecvLen);
							if (ParseHeader(ReceivedData))
							{
								//printf("ReceivedData:\n%s\n", ReceivedData);
								CHAR AcceptKey[20];
								CalAcceptKey(RecvHeaderFields[6].HeaderValue, SendHeaderFields[1].HeaderValue);
								int Len;
								Len = sprintf_s(SendRespData, 10000, "%s\r\n", SendHeaderFields[0].HeaderName);
								Len += sprintf_s(SendRespData + Len, 10000 - Len, "%s%s\r\n", RecvHeaderFields[2].HeaderName, RecvHeaderFields[2].HeaderValue);
								Len += sprintf_s(SendRespData + Len, 10000 - Len, "%s%s\r\n", RecvHeaderFields[3].HeaderName, RecvHeaderFields[3].HeaderValue);
								Len += sprintf_s(SendRespData + Len, 10000 - Len, "%s%s\r\n", SendHeaderFields[1].HeaderName, SendHeaderFields[1].HeaderValue);
								Len += sprintf_s(SendRespData + Len, 10000 - Len, "\r\n");

								//Len += sprintf_s(SendRespData + Len, 10000 - Len,  "%s%s\r\n", SendHeaderFields[1].HeaderName, "bwsvSbxWjtmyF3q8pxNTNg==");
								//printf("SendRespData:\n%s\n", SendRespData);
								int SendLen;
								SendLen = send(pCallBlock->ThisSocket, (char*)&SendRespData, strlen(SendRespData), 0);
								if (SendLen > 0)
								{
									pCallBlock->ConnState = 2; //Connected
									printf("ConnState: %d\n", pCallBlock->ConnState);
								}
							}
							else
							{
								printf("#################ReceivedData:\n");
#if 0
								for (int i = 0; i < RecvLen; i++)
								{
									printf("%02X ", (BYTE)ReceivedData[i]);
								}
								printf("\n");
#endif
								PWebsocketFrameSeg_T pWSFrameSeg;
								pWSFrameSeg = (PWebsocketFrameSeg_T)&ReceivedData;
								//printf("FIN: %s\n", pWSFrameSeg->FrameCtrl & 0x80 ? "True" : "False");
								//printf("Reserved: %d\n", pWSFrameSeg->FrameCtrl & 0x70 >> 4);
								BYTE Opcode = pWSFrameSeg->FrameCtrl & 0x0F;
								printf("Opcode: %d\n", Opcode);
								if (Opcode == 1)
								{
									//printf("Mask: %s\n", pWSFrameSeg->PayloadLength & 0x80 ? "True" : "False");
									int Length = pWSFrameSeg->PayloadLength & 0x7F;
									//printf("Len: %d\n", Length);
									if (Length < 126)
									{
										//printf("Playload length < 126\n");
										int len = sizeof(WebsocketFrameSeg_T);
										if ((pWSFrameSeg->PayloadLength & 0x80) == 0x80)
										{
											UINT32 MaskingKey;
											MaskingKey = (UINT8)ReceivedData[len + 3];
											MaskingKey |= (UINT8)ReceivedData[len + 2] << 8;
											MaskingKey |= (UINT8)ReceivedData[len + 1] << 16;
											MaskingKey |= (UINT8)ReceivedData[len + 0] << 24;
											//printf("MaskingKey: %04X\n", MaskingKey);
											PWebsocketFramePayload_T pFramePayload = (PWebsocketFramePayload_T)&ReceivedData[len];
											UINT8 MaskingKeyByte[4];
											memcpy(MaskingKeyByte, &pFramePayload->MaskingKey, 4);
											//pCallBlock->pPeerId = (PCHAR)malloc(Length + 1);
											// Length（短格式 WS payload）上限是 125，Buffer 要跟著放大，
											// 不然比 100 長的訊息（例如新版 API 的 64 字元 Token）解出來
											// 就已經被截斷了。
											CHAR Buffer[128];
											memset(Buffer, 0x00, 128);
											PCHAR pRecvPayload = &ReceivedData[len + 4];
											UINT8 OneBytePayload;
											UINT8 MaskPayload;
											for (int i = 0; i < Length; i += 4)
											{
												for (int j = 0; j < 4; j++)
												{
													MaskPayload = pRecvPayload[i + j];
													OneBytePayload = MaskPayload ^ MaskingKeyByte[j];
													Buffer[i + j] = OneBytePayload;
												}
											}
											printf("Payload: %s\n", Buffer);
											// 原本這裡硬寫死只複製 32 bytes，是舊版永久 UUID 的長度；
											// 新版 API 的 Bearer Token 是 64 個字元，硬寫 32 會把 Token
											// 截斷成一段查不到人的亂碼，害即時看板收不到任何通話事件。
											// 改成照實際收到的長度複製，並確實補上結尾的 0。
											{
												int CopyLen = min(Length, (int)sizeof(pCallBlock->OperatorUUID) - 1);
												memcpy(pCallBlock->OperatorUUID, Buffer, CopyLen);
												pCallBlock->OperatorUUID[CopyLen] = 0;
											}
										}
										else
										{
											int CopyLen = min(Length, (int)sizeof(pCallBlock->OperatorUUID) - 1);
											memcpy(pCallBlock->OperatorUUID, &ReceivedData[len], CopyLen);
											pCallBlock->OperatorUUID[CopyLen] = 0;
										}
#if 0
										PCHAR StrPtr;
										StrPtr = strstr(pCallBlock->pPeerId, "MachineID:");
										if (StrPtr)
										{
											PCHAR EndPtr = StrPtr + strlen("MachineID:");
											pCallBlock->MachineID = atoi(EndPtr);
										}
#endif
									}
									else if (Length == 0x7E)
									{
										printf("Playload length is 16 bits\n");
										int len = sizeof(WebsocketFrameSeg_T);
										UINT16 ExtLength;
										ExtLength = (UINT8)ReceivedData[len + 1];
										ExtLength |= (UINT8)ReceivedData[len + 0] << 8;
										printf("Extened Length: %d\n", ExtLength);
										len += sizeof(UINT16);
										UINT32 MaskingKey;
										MaskingKey = (UINT8)ReceivedData[len + 3];
										MaskingKey |= (UINT8)ReceivedData[len + 2] << 8;
										MaskingKey |= (UINT8)ReceivedData[len + 1] << 16;
										MaskingKey |= (UINT8)ReceivedData[len + 0] << 24;
										printf("MaskingKey: %02X\n", MaskingKey);
										PWebsocketFramePayload_T pFramePayload = (PWebsocketFramePayload_T)&ReceivedData[len];
										UINT8 MaskingKeyByte[4];
										memcpy(MaskingKeyByte, &pFramePayload->MaskingKey, 4);
#if 0
										pCallBlock->pPeerId = (PCHAR)malloc(ExtLength + 1);
										PCHAR pRecvPayload = &ReceivedData[len + 4];
										UINT8 OneBytePayload;
										UINT8 MaskPayload;
										for (int i = 0; i < ExtLength; i += 4)
										{
											for (int j = 0; j < 4; j++)
											{
												MaskPayload = pRecvPayload[i + j];
												OneBytePayload = MaskPayload ^ MaskingKeyByte[j];
												pCallBlock->pPeerId[i + j] = OneBytePayload;
											}
										}
										pCallBlock->pPeerId[ExtLength] = 0;
#endif
									}
									else if (Length == 0x7F)
									{
										printf("Playload length is 32 bits\n");
										int len = sizeof(WebsocketFrameSeg_T);
										UINT16 ExtLength;
										ExtLength = (UINT8)ReceivedData[len + 1];
										ExtLength |= (UINT8)ReceivedData[len + 0] << 8;
										printf("Extened Length: %d\n", ExtLength);
										len += sizeof(UINT32);
										UINT32 MaskingKey;
										MaskingKey = (UINT8)ReceivedData[len + 3];
										MaskingKey |= (UINT8)ReceivedData[len + 2] << 8;
										MaskingKey |= (UINT8)ReceivedData[len + 1] << 16;
										MaskingKey |= (UINT8)ReceivedData[len + 0] << 24;
										printf("MaskingKey: %02X\n", MaskingKey);
									}
								}
								else if (Opcode == 8)
								{
									/* Close*/
									printf("#################Disconnected By WS Socket:%d!!!\n", pCallBlock->ThisSocket);
									ReleaseCallBlock(pCallBlock);
									printf("ConnState: %d\n", pCallBlock->ConnState);
								}
								else if (Opcode == 2)
								{
									sprintf_s(SendRespData, 10000, "This Server is only support TEXT mode\n");
									send(pCallBlock->ThisSocket, (char*)&SendRespData, strlen(SendRespData), 0);
								}
							}
						}
						else
						{
							printf("#################Disconnected Socket:%d!!!\n", pCallBlock->ThisSocket);
							ReleaseCallBlock(pCallBlock);
							pCallBlock->ConnState = 0; //Disconnected
							printf("ConnState: %d\n", pCallBlock->ConnState);
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
					PWSCallBlock_T pCallBlock;
					pCallBlock = GetFreeCallBlok();
					pCallBlock->ConnState = 1; //Connecting
					pCallBlock->InUsed = TRUE;
					pCallBlock->ThisSocket = NewSock;
					printf("ConnState: %d\n", pCallBlock->ConnState);
					FD_SET(NewSock, &readfds);
				}
			}
		}
		else if (ret == 0)
		{
#if 0
			int SendRet = 0;
			for (int i = 0; i < 100; i++)
			{
				PWSCallBlock_T pCallBlock = (PWSCallBlock_T)&WSCallBlock[i];
				if (pCallBlock->InUsed == TRUE)
				{
					struct tm newtime;
					__time64_t long_time;
					CHAR SendMsg[1000];
					memset(SendMsg, 0, 1000);
					_time64(&long_time);
					errno_t err;
					// Convert to local time.
					err = _localtime64_s(&newtime, &long_time);
					err = asctime_s(SendMsg, 26, &newtime);
					//int len = sprintf_s(SendMsg, 1000, "%d", pCallBlock->loop);
					//SendRet = SendWebSocketPayload(pCallBlock->ThisSocket, SendMsg, 25);
					//printf("SendRet = %d\n", SendRet);
					if (SendRet != 0)
					{
						printf("#################Garbage Socket:%d!!!\n", pCallBlock->ThisSocket);
						ReleaseCallBlock(pCallBlock);
						pCallBlock->ConnState = 0; //Disconnected
						printf("ConnState: %d\n", pCallBlock->ConnState);
					}
				}
			}
#endif
		}
		else if (ret == -1)
		{
			printf("Select Error\n");
			return;
		}
	}
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

int OpenWebSocketServer()
{
	int Ret = -1;

	//Ret = Start_Winsock();
	//if (Ret == 0)
	{
		CHAR IP[100] = "0.0.0.0";
		int ret = Create_TCP_Sock(3000, IP);
		if (ret == 0)
		{
			pADICTICallCenterDlg = (CADICTICallCenterDlg*)AfxGetMainWnd();
			pPbxDlg = (CPbxDlg*)pADICTICallCenterDlg->m_pWndModleDlg[CTRL_ID_PBX_TYPE_UI];
			pCallRecorderDlg = (CCallRecorderDlg*)pADICTICallCenterDlg->m_pWndModleDlg[CTRL_ID_CALL_RECORDER_TYPE_UI];
			pVoiceRecorderDlg = (CVoiceRecorderDlg*)pADICTICallCenterDlg->m_pWndModleDlg[CTRL_ID_VOICE_RECORDER_TYPE_UI];
			Thread = (HANDLE)CreateThread(NULL, 2000, (unsigned long(__stdcall*)(void*))ThreadWSServerHandler, NULL, 0, NULL);
		}
	}

	return Ret;
}

int WSSendOutPortStatus(PCallBlock_T pCallBlock)
{
	PWSCallBlock_T pWSCallBlock;
	CHAR SendMsg[1000];
	int SendRet = 0;
	int Len;
	class CDatabaseAccessURL m_DatabaseAccessURL;
	PWSCallBlock_T pWSCallBlock_T = NULL;
	BOOL Found = FALSE;
	OperatorInfo_T OperatorInfo;
	CustomerInfo_T CustomerInfo;
	OutVPortCallStatus_T OutVPortCallStatus;
	int Ret = 0;

	if (pCallBlock->MachineID <= 0 || pCallBlock->MachineID > 10)
	{
		return -1;
	}

	for (UINT16 i = 0; i < 100; i++)
	{
		pWSCallBlock = (PWSCallBlock_T)&WSCallBlock[i];
		if (pWSCallBlock->InUsed == TRUE)
		{
			int ExtNumber;
			int ExtVPort;
			memset(SendMsg, 0x00, 1000);
			if (pCallBlock->CallType == TYPE_CALL_OUT)
			{
				ExtNumber = pCallBlock->FromExtNo;
				ExtVPort = pCallBlock->FromExtVPort;
			}
			else
			{
				ExtNumber = pCallBlock->ToExtNo;
				ExtVPort = pCallBlock->ToExtVPort;
			}
			memset(&OperatorInfo, 0x00, sizeof(OperatorInfo_T));
			Ret = m_DatabaseAccessURL.GetOperatorInfo(pWSCallBlock->OperatorUUID, &OperatorInfo);
			if (Ret == 0)
			{
				int MachineBitmask = OperatorInfo.MachineID;
				for (int MachineIndex = 0; MachineIndex < 10; MachineIndex++)
				{
					int Bit = MachineBitmask & (1 << MachineIndex);
					if (Bit != 0 && pCallBlock->MachineID == (MachineIndex + 1))
					{
						int EditCallStatusToDBIndex;
						EditCallStatusToDBIndex = AfxGetApp()->GetProfileInt("SystemSetting", "EditCallStatusToDBIndex", 0);
						if (EditCallStatusToDBIndex == 1)
						{
							OutVPortCallStatus.Status = pCallBlock->CallStatus;
							OutVPortCallStatus.Type = pCallBlock->CallType;
							OutVPortCallStatus.PhyPortNo = pCallBlock->PhyOutPort;
							OutVPortCallStatus.pTelNo = pCallBlock->CallerId;
							OutVPortCallStatus.ExtNum = ExtNumber;
							m_DatabaseAccessURL.SetOutVPortCallStatusInfo(pCallBlock->OutVPort, &OutVPortCallStatus);
						}
						//memset(&CustomerInfo, 0x00, sizeof(CustomerInfo_T));
						//m_DatabaseAccessURL.GetCustomerInfo(pCallBlock->CallerId, &CustomerInfo);
						/*
							ExtVPort 是新加的第 8 個欄位（虛擬內線編號，對應
							dbo.extline_ports.vport），給網頁的「虛擬內線」看板
							即時更新用。內線互撥(CallType=2)時 OutVPort 一律是
							-1（沒有外線埠可言），要靠這個欄位才查得到是哪條
							虛擬內線；外線撥入/撥出時一樣會有值，可讓內線看板
							同步顯示「這支分機正在忙」。
						*/
						/* Send JSON result through WebSocket */
						Len = sprintf_s(SendMsg, 1000, "{\"data\":[\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%s\",\"%d\",\"%d\"]}",
							pCallBlock->MachineID, pCallBlock->CallType, pCallBlock->CallStatus, pCallBlock->PhyOutPort,
							pCallBlock->OutVPort, pCallBlock->CallerId, ExtNumber, ExtVPort);
						SendRet = SendWebSocketPayload(pWSCallBlock->ThisSocket, SendMsg, Len);
						break;
					}
				}
			}
		}
	}

	return 0;
}
