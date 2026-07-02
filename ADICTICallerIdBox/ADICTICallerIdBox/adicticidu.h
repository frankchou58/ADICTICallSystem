#pragma once
#define CARSERVERDLL_API __declspec(dllimport)
//FCallback = procedure(LineID, ItemID: Integer; Data: PAnsiChar); stdcall;

//CARSERVERDLL_API void CIDU_SetCallback(ACallback: FCallback); stdcall;
CARSERVERDLL_API bool CIDU_StartDevice();
CARSERVERDLL_API bool  CIDU_StopDevice();
CARSERVERDLL_API int CIDU_GetDeviceCount();
CARSERVERDLL_API int CIDU_GetLineCount();
CARSERVERDLL_API char* CIDU_GetDeviceSerialNo(int DeviceID);
CARSERVERDLL_API int CIDU_GetLineOfDevice(int LineID);
CARSERVERDLL_API char* CIDU_GetCallerID(int LineID);
CARSERVERDLL_API char* CIDU_GetLastCallerID(int LineID);
CARSERVERDLL_API char* CIDU_GetDialDigit(int LineID);
CARSERVERDLL_API char* CIDU_GetLastDialDigit(int LineID);
CARSERVERDLL_API char* CIDU_GetDTMFs(int LineID);
CARSERVERDLL_API char* CIDU_GetLastDTMFs(int LineID);
CARSERVERDLL_API bool CIDU_ClearDTMFs(int LineID);
CARSERVERDLL_API int CIDU_GetRingCount(int LineID);
CARSERVERDLL_API bool CIDU_IsRing(int LineID);
CARSERVERDLL_API bool CIDU_IsPickup(int LineID);
CARSERVERDLL_API int CIDU_GetTalkTime(int LineID);
CARSERVERDLL_API int CIDU_GetLastTalkTime(int LineID);
CARSERVERDLL_API int CIDU_GetDirection(int LineID);
CARSERVERDLL_API int CIDU_GetLastDirection(int LineID);
CARSERVERDLL_API bool CIDU_SwapDevice(int DeviceID1, int DeviceID2);
CARSERVERDLL_API bool CIDU_WritePrivateCode(int DeviceID, char* Code);
CARSERVERDLL_API char* CIDU_ReadPrivateCode(int DeviceID);
CARSERVERDLL_API char* CIDU_GetFirmwareVersion(int DeviceID);

