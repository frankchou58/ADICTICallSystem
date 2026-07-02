#pragma once


int Start_Winsock();
int Stop_Winsock();
int Create_TCP_Sock(unsigned int tcpport, const char* ip_address);
void Close_TCP_Sock();
int RecvResp(PCHAR pMachineResponsePackage);
int SendLogin(int MachineType, int MachineID, int OutPortNums, int ExtPortNums);
int SendExternalCallIn(int MachineType, int MachineID, UINT32 Timestamp, int OutPortNo, PCHAR pCallerID);
int SendExternalCallOut(int MachineType, int MachineID, UINT32 Timestamp, int OutPortNo, int FromExtNum, PCHAR pCallerID);
int SendExternalCallAnswer(int MachineType, int MachineID, UINT32 Timestamp, int OutPortNo, int FromExtNum, int RingTimes);
int SendExternalCallHangup(int MachineType, int MachineID, UINT32 Timestamp, int OutPortNo, int TalkingDuration);
int SendInternalCallMake(int MachineType, int MachineID, UINT32 Timestamp, int FromExtNumber, int ToExtNumber);
int SendInternalCallAnswer(int MachineType, int MachineID, UINT32 Timestamp, int FromExtNumber, int ToExtNumber, int RingTimes);
int SendInternalCallHangup(int MachineType, int MachineID, UINT32 Timestamp, int FromExtNumber, int ToExtNumber, int TalkingDuration);
int SendVoiceFileName(int MachineType, int MachineID, UINT32 Timestamp, int OutPortNo, int ToExtNumber, PCHAR pVoiceFileName);


