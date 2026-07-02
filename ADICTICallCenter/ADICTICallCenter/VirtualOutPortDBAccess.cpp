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

int SetDBOutVPortSetting(int MachineID)
{
	CDatabaseAccessURL m_DatabaseAccessURL;

	if (LoadMachinePorts() < 0)
	{
		return -1;
	}
	/*�ھ�Machine����ƭ��s�]�w�~�u��Ʈw���e*/
	int PortNums = GetDBOutVPortNums();
	int PortIndex = 1;
	int Ret;
	for (int MachineIDIndex = 0; MachineIDIndex < SUBPROGRAM_NUMBERS; MachineIDIndex++)
	{
		if (PBXOutPortNum[MachineIDIndex] > 0)
		{
			for (int PhyPort = 1; PhyPort <= PBXOutPortNum[MachineIDIndex]; PhyPort++)
			{
				Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_PBX, MachineIDIndex + 1, PhyPort, NULL);
				if (CallerIDBoxOutPortNum[MachineIDIndex] >= 0 && MachineIDIndex == MachineID - 1)
				{
					if (PhyPort <= CallerIDBoxOutPortNum[MachineIDIndex])
						Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_CALLER_ID_BOX, MachineIDIndex + 1, PhyPort, NULL);
					else
						Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_CALLER_ID_BOX, MachineIDIndex + 1, 0, NULL);
				}
				if (VoiceCardOutPortNum[MachineIDIndex] >= 0 && MachineIDIndex == MachineID - 1)
				{
					if (PhyPort <= VoiceCardOutPortNum[MachineIDIndex])
						Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_VOICE_CARD, MachineIDIndex + 1, PhyPort, NULL);
					else
						Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_VOICE_CARD, MachineIDIndex + 1, 0, NULL);
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
					Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_PBX, MachineIDIndex + 1, 0, NULL);
					PortIndex++;
				}
			}
			if (VoiceCardOutPortNum[MachineIDIndex] >= 0)
			{
				for (int PhyPort = 1; PhyPort <= VoiceCardOutPortNum[MachineIDIndex]; PhyPort++)
				{
					Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_VOICE_CARD, MachineIDIndex + 1, PhyPort, NULL);
					Ret = m_DatabaseAccessURL.AssignPhyOutPortInfo(PortIndex, MACHINE_TYPE_PBX, MachineIDIndex + 1, 0, NULL);
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

	return 0;
}

int SetDBExtVPortSetting()
{
	CDatabaseAccessURL m_DatabaseAccessURL;

	LoadMachinePorts();
	/*�ھ�Machine����ƭ��s�]�w�~�u��Ʈw���e*/
	int PortNums = GetDBExtVPortNums();
	int PortIndex = 1;
	int Ret;
	for (int MachineIDIndex = 0; MachineIDIndex < SUBPROGRAM_NUMBERS; MachineIDIndex++)
	{
		if (PBXExtPortNum[MachineIDIndex] > 0)
		{
			for (int PhyPort = 1; PhyPort <= PBXExtPortNum[MachineIDIndex]; PhyPort++)
			{
				Ret = m_DatabaseAccessURL.AssignPhyExtPortInfo(PortIndex, MachineIDIndex + 1, PhyPort, NULL);
				PortIndex++;
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

	return 0;
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
