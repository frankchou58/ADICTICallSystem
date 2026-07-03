#include "pch.h"
#include "VirtualOutPortDBAccess.h"

static int PBXOutPortNum[SUBPROGRAM_NUMBERS];
static int PBXExtPortNum[SUBPROGRAM_NUMBERS];
static int CallerIDBoxOutPortNum[SUBPROGRAM_NUMBERS];
static int CallerIDBoxExtPortNum[SUBPROGRAM_NUMBERS];
static int VoiceCardOutPortNum[SUBPROGRAM_NUMBERS];
static int VoiceCardExtPortNum[SUBPROGRAM_NUMBERS];

static int LoadMachinePorts()
{
	// 原本對 30 個機碼（3 種類型 x 10 個機碼）逐一呼叫 GetMachineInfo()，
	// 等於每次都要跑 30 次同步 HTTP 往返；改成一次 GET /machines 拿全部
	// 列回來自己分類，降成 1 次 HTTP 往返。
	CDatabaseAccessURL m_DatabaseAccessURL;
	int Ret = m_DatabaseAccessURL.GetAllMachinesPortCounts(
		PBXOutPortNum, PBXExtPortNum,
		CallerIDBoxOutPortNum, CallerIDBoxExtPortNum,
		VoiceCardOutPortNum, VoiceCardExtPortNum);
	if (Ret != ERROR_CODE_SUCCESS)
		return -1;

	return 0;
}

// 傳回值：0 = 全部指派成功；>0 = 有幾個實體埠指派失敗（該埠在資料庫裡
// 沒有對應的既有佈線資料，遷移後的資料庫只能搬動已存在的埠，沒辦法
// 憑空生出新的實體埠對應——常見於外線/內線數量設定超過客戶原本佈線
// 涵蓋的實體埠數）；-1 = 連機碼資料都讀不到，整個流程沒執行。
int SetDBOutVPortSetting(int MachineID)
{
	CDatabaseAccessURL m_DatabaseAccessURL;

	if (LoadMachinePorts() < 0)
	{
		return -1;
	}
	/*根據Machine的資料重新設定外線資料庫內容*/
	int PortNums = GetDBOutVPortNums();
	int PortIndex = 1;
	int Ret;
	int FailedCount = 0;
	for (int MachineIDIndex = 0; MachineIDIndex < SUBPROGRAM_NUMBERS; MachineIDIndex++)
	{
		if (PBXOutPortNum[MachineIDIndex] > 0)
		{
			for (int PhyPort = 1; PhyPort <= PBXOutPortNum[MachineIDIndex]; PhyPort++)
			{
				Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_PBX, MachineIDIndex + 1, PhyPort, NULL);
				if (Ret != ERROR_CODE_SUCCESS)
					FailedCount++;
				if (CallerIDBoxOutPortNum[MachineIDIndex] >= 0 && MachineIDIndex == MachineID - 1)
				{
					if (PhyPort <= CallerIDBoxOutPortNum[MachineIDIndex])
						Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_CALLER_ID_BOX, MachineIDIndex + 1, PhyPort, NULL);
					else
						Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_CALLER_ID_BOX, MachineIDIndex + 1, 0, NULL);
					if (Ret != ERROR_CODE_SUCCESS)
						FailedCount++;
				}
				if (VoiceCardOutPortNum[MachineIDIndex] >= 0 && MachineIDIndex == MachineID - 1)
				{
					if (PhyPort <= VoiceCardOutPortNum[MachineIDIndex])
						Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_VOICE_CARD, MachineIDIndex + 1, PhyPort, NULL);
					else
						Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_VOICE_CARD, MachineIDIndex + 1, 0, NULL);
					if (Ret != ERROR_CODE_SUCCESS)
						FailedCount++;
				}
				PortIndex++;
			}
		}
		else if (PBXOutPortNum[MachineIDIndex] == 0)
		{
			if (CallerIDBoxOutPortNum[MachineIDIndex] >= 0)
			{
				for (int PhyPort = 1; PhyPort <= CallerIDBoxOutPortNum[MachineIDIndex]; PhyPort++)
				{
					Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_CALLER_ID_BOX, MachineIDIndex + 1, PhyPort, NULL);
					if (Ret != ERROR_CODE_SUCCESS)
						FailedCount++;
					Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_PBX, MachineIDIndex + 1, 0, NULL);
					if (Ret != ERROR_CODE_SUCCESS)
						FailedCount++;
					PortIndex++;
				}
			}
			if (VoiceCardOutPortNum[MachineIDIndex] >= 0)
			{
				for (int PhyPort = 1; PhyPort <= VoiceCardOutPortNum[MachineIDIndex]; PhyPort++)
				{
					Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_VOICE_CARD, MachineIDIndex + 1, PhyPort, NULL);
					if (Ret != ERROR_CODE_SUCCESS)
						FailedCount++;
					Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_PBX, MachineIDIndex + 1, 0, NULL);
					if (Ret != ERROR_CODE_SUCCESS)
						FailedCount++;
					PortIndex++;
				}
			}
		}
	}

	for (int VPort = 1; VPort <= VIRTUAL_PORT_NUMS; VPort++)
	{
		if (PortNums >= VPort)
		{
			Ret = m_DatabaseAccessURL.SetOutPortInUsed(VPort, TRUE, NULL);
			if (Ret != 0)
				printf("dewe");
		}
		else
		{
			Ret = m_DatabaseAccessURL.SetOutPortInUsed(VPort, FALSE, NULL);
			if (Ret != 0)
				printf("dewe");
		}
	}

	// 清除超出新數量的舊指派：上面的迴圈只會「指派/覆蓋」到新數量涵蓋
	// 的實體埠，縮減數量時（例如從 8 改成 0）超出新數量的那些舊指派
	// 完全不會被碰到——資料庫是固定 240 筆的池子，沒有列可以刪，只能
	// 明確查一次目前實際佈線內容，把超出新數量的實體埠取消指派，畫面
	// 上的埠號/虛擬編號才會真的消失，不會一直顯示已經不該存在的舊資料。
	for (int MachineIDIndex = 0; MachineIDIndex < SUBPROGRAM_NUMBERS; MachineIDIndex++)
	{
		int WiredPorts[VIRTUAL_PORT_NUMS];
		int WiredCount = 0;

		if (m_DatabaseAccessURL.GetWiredOutPhyPorts(MACHINE_TYPE_PBX, MachineIDIndex + 1, WiredPorts, VIRTUAL_PORT_NUMS, &WiredCount) == ERROR_CODE_SUCCESS)
		{
			for (int w = 0; w < WiredCount; w++)
			{
				if (WiredPorts[w] > PBXOutPortNum[MachineIDIndex])
				{
					Ret = m_DatabaseAccessURL.UnassignPhyOutPortInfo(MACHINE_TYPE_PBX, MachineIDIndex + 1, WiredPorts[w], NULL);
					if (Ret != ERROR_CODE_SUCCESS)
						FailedCount++;
				}
			}
		}

		if (m_DatabaseAccessURL.GetWiredOutPhyPorts(MACHINE_TYPE_CALLER_ID_BOX, MachineIDIndex + 1, WiredPorts, VIRTUAL_PORT_NUMS, &WiredCount) == ERROR_CODE_SUCCESS)
		{
			for (int w = 0; w < WiredCount; w++)
			{
				if (WiredPorts[w] > CallerIDBoxOutPortNum[MachineIDIndex])
				{
					Ret = m_DatabaseAccessURL.UnassignPhyOutPortInfo(MACHINE_TYPE_CALLER_ID_BOX, MachineIDIndex + 1, WiredPorts[w], NULL);
					if (Ret != ERROR_CODE_SUCCESS)
						FailedCount++;
				}
			}
		}

		if (m_DatabaseAccessURL.GetWiredOutPhyPorts(MACHINE_TYPE_VOICE_CARD, MachineIDIndex + 1, WiredPorts, VIRTUAL_PORT_NUMS, &WiredCount) == ERROR_CODE_SUCCESS)
		{
			for (int w = 0; w < WiredCount; w++)
			{
				if (WiredPorts[w] > VoiceCardOutPortNum[MachineIDIndex])
				{
					Ret = m_DatabaseAccessURL.UnassignPhyOutPortInfo(MACHINE_TYPE_VOICE_CARD, MachineIDIndex + 1, WiredPorts[w], NULL);
					if (Ret != ERROR_CODE_SUCCESS)
						FailedCount++;
				}
			}
		}
	}

	return FailedCount;
}

// 傳回值同 SetDBOutVPortSetting()：0 = 全部成功，>0 = 有幾個實體內線埠
// 指派失敗（同樣是因為遷移後的資料庫只能搬動既有佈線，沒辦法新增）。
int SetDBExtVPortSetting()
{
	CDatabaseAccessURL m_DatabaseAccessURL;

	LoadMachinePorts();
	/*根據Machine的資料重新設定外線資料庫內容*/
	int PortNums = GetDBExtVPortNums();
	int PortIndex = 1;
	int Ret;
	int FailedCount = 0;
	for (int MachineIDIndex = 0; MachineIDIndex < SUBPROGRAM_NUMBERS; MachineIDIndex++)
	{
		if (PBXExtPortNum[MachineIDIndex] > 0)
		{
			for (int PhyPort = 1; PhyPort <= PBXExtPortNum[MachineIDIndex]; PhyPort++)
			{
				Ret = m_DatabaseAccessURL.AssignPhyExtPortInfo(PortIndex, MachineIDIndex + 1, PhyPort, NULL);
				if (Ret != ERROR_CODE_SUCCESS)
					FailedCount++;
				PortIndex++;
			}
		}
	}

	// 清除超出新數量的舊指派，理由跟 SetDBOutVPortSetting() 裡的同一段一樣。
	for (int MachineIDIndex = 0; MachineIDIndex < SUBPROGRAM_NUMBERS; MachineIDIndex++)
	{
		int WiredPorts[VIRTUAL_PORT_NUMS];
		int WiredCount = 0;

		if (m_DatabaseAccessURL.GetWiredExtPhyPorts(MachineIDIndex + 1, WiredPorts, VIRTUAL_PORT_NUMS, &WiredCount) == ERROR_CODE_SUCCESS)
		{
			for (int w = 0; w < WiredCount; w++)
			{
				if (WiredPorts[w] > PBXExtPortNum[MachineIDIndex])
				{
					Ret = m_DatabaseAccessURL.UnassignPhyExtPortInfo(MachineIDIndex + 1, WiredPorts[w], NULL);
					if (Ret != ERROR_CODE_SUCCESS)
						FailedCount++;
				}
			}
		}
	}

#if 0
	for (int VPort = 1; VPort <= VIRTUAL_PORT_NUMS; VPort++)
	{
		if (PortNums >= VPort)
		{
			Ret = m_DatabaseAccessURL.SetOutPortInUsed(VPort, TRUE, NULL);
			if (Ret != 0)
				printf("dewe");
		}
		else
		{
			Ret = m_DatabaseAccessURL.SetOutPortInUsed(VPort, FALSE, NULL);
			if (Ret != 0)
				printf("dewe");
		}
	}
#endif

	return FailedCount;
}

int GetDBOutVPortNums()
{
	int OutVPortNums = 0;

	LoadMachinePorts();
	for (int i = 0; i < SUBPROGRAM_NUMBERS; i++)
	{
		if (PBXOutPortNum[i] > 0)
		{
			OutVPortNums += PBXOutPortNum[i];
		}
		else if (PBXOutPortNum[i] == 0)
		{
			if (CallerIDBoxOutPortNum[i] > 0)
			{
				OutVPortNums += CallerIDBoxOutPortNum[i];
			}
			if (VoiceCardOutPortNum[i] > 0)
			{
				OutVPortNums += VoiceCardOutPortNum[i];
			}
		}
	}

	return OutVPortNums;
}

int GetDBExtVPortNums()
{
	int ExtVPortNums = 0;

	LoadMachinePorts();
	for (int i = 0; i < SUBPROGRAM_NUMBERS; i++)
	{
		if (PBXExtPortNum[i] > 0)
		{
			ExtVPortNums += PBXExtPortNum[i];
		}
	}

	return ExtVPortNums;
}
