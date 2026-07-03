#include "pch.h"
#include "CallStateRecordEngine.h"
#include "ADICTICallCenterDlg.h"
#include "CShowLinesStatusDlg.h"
#include "CDatabaseAccessURL.h"

#define BufferSize 100

typedef struct DBBuffer_Tag
{
    BOOL IsBuffering;
    CallBlock_T CallBlock;
}DBBuffer_T, *PDBBuffer_T;

typedef struct PackageEventBuffer_Tag
{
    BOOL IsInUsed;
    EventParamsT EventParams;
    UINT EventID;
} PackageEventBuffer_T, *PPackageEventBuffer_T;

static int CallBlockPushIndex = 1;
static int CallBlockPopIndex = 1;
static HANDLE hThreadCallRecordHandler;
static HANDLE hThreadEventBufferHandler;
typedef int STATE_FUNC(PCallBlock_T pCallBlock, PEventParamsT pEvenParams, UINT MSecond);
static CallBlock_T gCallBlock[240];
static CADICTICallCenterDlg* pADICTICallCenterDlg;
static DBBuffer_T gDBBuffer[BufferSize];
static PackageEventBuffer_T gPackageEventBuffer[BufferSize];
static int SentBufferEvent(PPackageEventBuffer_T pPackageEventBuffer);

static void SaveToDBBuffer(PCallBlock_T pCallBlock)
{
    PDBBuffer_T pDBBuffer;
    pDBBuffer = &gDBBuffer[CallBlockPushIndex];

    pDBBuffer->IsBuffering = TRUE;
    memcpy(&pDBBuffer->CallBlock, pCallBlock, sizeof(CallBlock_T));

    if (++CallBlockPushIndex >= BufferSize)
        CallBlockPushIndex = 1;
}

static void AddEventToBuffer(PPackageEventBuffer_T pPackageEventBuffer, PEventParamsT pEventParams, UINT EventID)
{
    pPackageEventBuffer->IsInUsed = TRUE;
    pPackageEventBuffer->EventID = EventID;
    memcpy(&pPackageEventBuffer->EventParams, pEventParams, sizeof(EventParamsT));
}

static void ThreadCallRecordHandler(LPVOID lParam)
{
    class CDatabaseAccessURL DatabaseAccessURL;
    PCallBlock_T pCallBlock;
    CHAR Message[100];
    PDBBuffer_T pDBBuffer;
    BOOL HaveData = FALSE;

    while (1)
    {
        if (
            (CallBlockPushIndex != 1 || CallBlockPopIndex != 1) &&
            (CallBlockPushIndex != CallBlockPopIndex)
            )
        {
            HaveData = TRUE;
        }
        else
        {
            CallBlockPushIndex = 1;
            CallBlockPopIndex = 1;
            HaveData = FALSE;
        }

        if (HaveData == TRUE)
        {
            pDBBuffer = &gDBBuffer[CallBlockPopIndex];
            if (pDBBuffer->IsBuffering == TRUE)
            {
                pCallBlock = &pDBBuffer->CallBlock;
                if (pCallBlock)
                {
                    if (pCallBlock->CallType == TYPE_INTERNAL_CALL)
                        DatabaseAccessURL.AddInternalCallRecordOneTime(pCallBlock, NULL);
                    else
                        DatabaseAccessURL.AddExternalCallRecordOneTime(pCallBlock, NULL);
                }
                pDBBuffer->IsBuffering = FALSE;
            }
            if (++CallBlockPopIndex >= BufferSize)
                CallBlockPopIndex = 1;
        }
        Sleep(1);
    }
}

static void ThreadEventBufferHandler(LPVOID lParam)
{
    PPackageEventBuffer_T pPackageEventBuffer;
    int EventBufferIndex = 0;

    while (1)
    {
        for (EventBufferIndex = 0; EventBufferIndex < BufferSize; EventBufferIndex++)
        {
            pPackageEventBuffer = (PPackageEventBuffer_T)&gPackageEventBuffer[EventBufferIndex];
            if (pPackageEventBuffer->IsInUsed == TRUE)
            {
                SentBufferEvent(pPackageEventBuffer);
                pPackageEventBuffer->IsInUsed = FALSE;
            }
        }
        //if (EventBufferIndex >= BufferSize)
            //EventBufferIndex = 0;
        Sleep(1);
    }
}

static PCallBlock_T FindCallBlockByOutVPort(UINT OutVPort)
{
    PCallBlock_T pCallBlock = NULL;
    BOOL Found = FALSE;

    for (int i = 0; i < 240; i++)
    {
        pCallBlock = (PCallBlock_T)&gCallBlock[i];
        if (pCallBlock->OutVPort == OutVPort && pCallBlock->isInUsed == TRUE)
        {
            Found = TRUE;
            break;
        }
    }
    if (Found == FALSE)
        pCallBlock = NULL;

    return pCallBlock;
}

static PCallBlock_T FindCallBlockByFromExtVPort(UINT FromExtVPort)
{
    PCallBlock_T pCallBlock = NULL;
    BOOL Found = FALSE;

    for (int i = 0; i < 240; i++)
    {
        pCallBlock = (PCallBlock_T)&gCallBlock[i];
        if (pCallBlock->FromExtVPort == FromExtVPort && pCallBlock->isInUsed == TRUE)
        {
            Found = TRUE;
            break;
        }
    }
    if (Found == FALSE)
        pCallBlock = NULL;

    return pCallBlock;
}

static PCallBlock_T NewCallBlockByOutVPort(UINT OutVPort)
{
    PCallBlock_T pCallBlock = NULL;
    BOOL Found = FALSE;

    for (int i = 0; i < 240; i++)
    {
        pCallBlock = (PCallBlock_T)&gCallBlock[i];
        if (pCallBlock->isInUsed == FALSE)
        {
            memset(pCallBlock, 0x00, sizeof(CallBlock_T));
            pCallBlock->isInUsed = TRUE;
            Found = TRUE;
            break;
        }
    }
    if (Found == FALSE)
        pCallBlock = NULL;

    return pCallBlock;
}

static int ReleaseCallBlockByOutVPort(UINT OutVPort)
{
    PCallBlock_T pCallBlock = NULL;

    for (int i = 0; i < 240; i++)
    {
        pCallBlock = (PCallBlock_T)&gCallBlock[i];
        if (pCallBlock->OutVPort == OutVPort)
        {
            pCallBlock->isInUsed = FALSE;
            break;
        }
    }

    return 0;
}

static PCallBlock_T NewCallBlockByFromExtVPort(UINT FromExtVPort)
{
    PCallBlock_T pCallBlock = NULL;
    BOOL Found = FALSE;

    for (int i = 0; i < 240; i++)
    {
        pCallBlock = (PCallBlock_T)&gCallBlock[i];
        if (pCallBlock->isInUsed == FALSE)
        {
            memset(pCallBlock, 0x00, sizeof(CallBlock_T));
            pCallBlock->isInUsed = TRUE;
            Found = TRUE;
            break;
        }
    }
    if (Found == FALSE)
        pCallBlock = NULL;

    return pCallBlock;
}

static int ReleaseCallBlockByFromExtVPort(UINT FromExtVPort)
{
    PCallBlock_T pCallBlock = NULL;

    for (int i = 0; i < 240; i++)
    {
        pCallBlock = (PCallBlock_T)&gCallBlock[i];
        if (pCallBlock->FromExtVPort == FromExtVPort)
        {
            pCallBlock->isInUsed = FALSE;
            break;
        }
    }

    return 0;
}

static void CopyExternalCallIn(PCallBlock_T pCallBlock, PEventParamsT pEvenParams)
{
    if (pEvenParams)
    {
        pCallBlock->MachineType = pEvenParams->MachineType;
        pCallBlock->MachineID = pEvenParams->MachineID;
        pCallBlock->OutVPort = pEvenParams->OutVPort;
        pCallBlock->PhyOutPort = pEvenParams->PhyOutPort;
        pCallBlock->CallStartTime = pEvenParams->Timestamp;
        pCallBlock->SaveToDB = FALSE;
        memcpy(pCallBlock->CallerId, pEvenParams->TelNo, 50);
        /*外線打內線*/
        pCallBlock->CallType = TYPE_CALL_IN;
        /*沒有內線From只有內線To*/
        pCallBlock->FromExtNo = -1;
        pCallBlock->FromExtVPort = -1;
        pCallBlock->ToExtNo = pEvenParams->ToExt.ExtNo;
        pCallBlock->ToExtVPort = pEvenParams->ToExt.ExtVPort;
    }
}

static void CopyExternalCallOut(PCallBlock_T pCallBlock, PEventParamsT pEvenParams)
{
    if (pEvenParams)
    {
        pCallBlock->MachineType = pEvenParams->MachineType;
        pCallBlock->MachineID = pEvenParams->MachineID;
        pCallBlock->OutVPort = pEvenParams->OutVPort;
        pCallBlock->PhyOutPort = pEvenParams->PhyOutPort;
        pCallBlock->CallStartTime = pEvenParams->Timestamp;
        pCallBlock->SaveToDB = FALSE;
        memcpy(pCallBlock->CallerId, pEvenParams->TelNo, 50);
        /*內線打外線*/
        pCallBlock->CallType = TYPE_CALL_OUT;
        /*沒有內線To只有內線From*/
        pCallBlock->FromExtNo = pEvenParams->FromExt.ExtNo;
        pCallBlock->FromExtVPort = pEvenParams->FromExt.ExtVPort;
        pCallBlock->ToExtNo = -1;
        pCallBlock->ToExtVPort = -1;
    }
}

static void CopyInternalCallMake(PCallBlock_T pCallBlock, PEventParamsT pEvenParams)
{
    if (pEvenParams)
    {
        pCallBlock->SaveToDB = FALSE;
        pCallBlock->MachineType = pEvenParams->MachineType;
        pCallBlock->MachineID = pEvenParams->MachineID;
        pCallBlock->CallType = TYPE_INTERNAL_CALL;
        pCallBlock->CallStartTime = pEvenParams->Timestamp;
        pCallBlock->OutVPort = pEvenParams->OutVPort;
        pCallBlock->FromExtNo = pEvenParams->FromExt.ExtNo;
        pCallBlock->FromExtVPort = pEvenParams->FromExt.ExtVPort;
        pCallBlock->ToExtNo = pEvenParams->ToExt.ExtNo;
        pCallBlock->ToExtVPort = pEvenParams->ToExt.ExtVPort;
    }
}

static void CopyExternalCallTalking(PCallBlock_T pCallBlock, PEventParamsT pEvenParams)
{
    if (pEvenParams)
    {
        if (pCallBlock->CallStartTime == 1644481546)
            printf("dwedwe");
        pCallBlock->CallConnectionTime = pEvenParams->Timestamp;
        pCallBlock->RingTimes = pEvenParams->RingTimes;
        pCallBlock->FromExtNo = pEvenParams->FromExt.ExtNo;
        pCallBlock->FromExtVPort = pEvenParams->FromExt.ExtVPort;
        pCallBlock->ToExtNo = pEvenParams->FromExt.ExtNo;
        pCallBlock->ToExtVPort = pEvenParams->FromExt.ExtVPort;

    }
}

static void CopyInternalCallTalking(PCallBlock_T pCallBlock, PEventParamsT pEvenParams)
{
    if (pEvenParams)
    {
        pCallBlock->CallType = TYPE_INTERNAL_CALL;
        pCallBlock->CallConnectionTime = pEvenParams->Timestamp;
        pCallBlock->RingTimes = pEvenParams->RingTimes;
        /*內線打內線*/
        pCallBlock->OutVPort = -1;
        /*內線From及To都有*/
        pCallBlock->FromExtNo = pEvenParams->FromExt.ExtNo;
        pCallBlock->FromExtVPort = pEvenParams->FromExt.ExtVPort;
        pCallBlock->ToExtNo = pEvenParams->ToExt.ExtNo;
        pCallBlock->ToExtVPort = pEvenParams->ToExt.ExtVPort;
    }
}

static void CopyExternalCallEnd(PCallBlock_T pCallBlock, PEventParamsT pEvenParams)
{
    if (pEvenParams)
    {
        pCallBlock->CallEndTime = pEvenParams->Timestamp;
        pCallBlock->CallDuration = pEvenParams->TalkingDuration;
        pCallBlock->SaveToDB = TRUE;
    }
}

static void CopyInternalCallEnd(PCallBlock_T pCallBlock, PEventParamsT pEvenParams)
{
    if (pEvenParams)
    {
        pCallBlock->CallType = TYPE_INTERNAL_CALL;
        pCallBlock->CallEndTime = pEvenParams->Timestamp;
        pCallBlock->CallDuration = pEvenParams->TalkingDuration;
        pCallBlock->SaveToDB = TRUE;
    }
}

static int GotoState(PCallBlock_T pCallBlock, UINT StateID)
{
    if (pCallBlock != NULL)
    {
        UINT PrevStateID = pCallBlock->PrevState = pCallBlock->CurrentState;
        UINT CurrentStateID = pCallBlock->CurrentState = StateID;
        //printf("Line[%d]: From [%s] ==> [%s]\n", OutNo + 1, StateString[PrevStateID], StateString[CurrentStateID]);
    }

    return 0;
}

static int StateIdle(PCallBlock_T pCallBlock, PEventParamsT pEvenParams, UINT EventID)
{
    CShowLinesStatusDlg* pShowLinesStatusDlg = (CShowLinesStatusDlg*)pADICTICallCenterDlg->m_pWndModleDlg[CTRL_ID_SHOW_PORT_STATUS];
    switch (EventID)
    {
    case EVENT_ID_OUTLINE_IN:
        /*跳轉到STATE_ID_WAIT_LINE_TALKING狀態*/
        GotoState(pCallBlock, STATE_ID_WAIT_LINE_TALKING);
        /*拷貝相關的訊息到該CallBlock*/
        CopyExternalCallIn(pCallBlock, pEvenParams);
        /*顯示LED燈號*/
        if(pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_IN);
        break;
    case EVENT_ID_OUTLINE_OUT:
        /*跳轉到STATE_ID_WAIT_LINE_TALKING狀態*/
        GotoState(pCallBlock, STATE_ID_WAIT_LINE_TALKING);
        /*拷貝相關的訊息到該CallBlock*/
        CopyExternalCallOut(pCallBlock, pEvenParams);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_OUT);
        break;
    case EVENT_ID_OUTLINE_DISCONNECT:
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_OFF);
        ReleaseCallBlockByOutVPort(pCallBlock->OutVPort);
        break;
    case EVENT_ID_EXTLINE_MAKE:
        /*跳轉到STATE_ID_WAIT_EXT_TALKING狀態*/
        GotoState(pCallBlock, STATE_ID_WAIT_EXT_TALKING);
        /*拷貝相關的訊息到該CallBlock*/
        CopyInternalCallMake(pCallBlock, pEvenParams);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_OUT);
        break;
    case EVENT_ID_EXTLINE_DISCONNECT:
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_OFF);
        ReleaseCallBlockByFromExtVPort(pCallBlock->FromExtVPort);
        break;
    case EVENT_ID_VOICE_REC_FILE_UUID:
        break;
    }

    return 0;
}

static int StateWaitLineTalking(PCallBlock_T pCallBlock, PEventParamsT pEvenParams, UINT EventID)
{
    int TelNoPrioritySelectIndex;

    CShowLinesStatusDlg* pShowLinesStatusDlg = (CShowLinesStatusDlg*)pADICTICallCenterDlg->m_pWndModleDlg[CTRL_ID_SHOW_PORT_STATUS];
    switch (EventID)
    {
    case EVENT_ID_OUTLINE_DISCONNECT:
        /*拷貝相關的訊息到該CallBlock*/
        CopyExternalCallEnd(pCallBlock, pEvenParams);
        SaveToDBBuffer(pCallBlock);
    case EVENT_ID_CALL_RELEASE:
        /*跳轉到STATE_ID_IDLE狀態*/
        GotoState(pCallBlock, STATE_ID_IDLE);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_OFF);
        ReleaseCallBlockByOutVPort(pCallBlock->OutVPort);
        break;
    case EVENT_ID_OUTLINE_OUT:
        /*拷貝相關的訊息到該CallBlock*/
        CopyExternalCallOut(pCallBlock, pEvenParams);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_OUT);
        /*
            在Wait Line Talking時又收到Call Out時
            要判斷之前儲存的電話號碼是否與這個事件的電話號碼是否相同,
            如果不同則依據電話號碼優先的設定來更新CallBlock的電話號碼
        */
        TelNoPrioritySelectIndex = AfxGetApp()->GetProfileInt("SystemSetting", "TelNoPrioritySelect", 0);
        if (strcmp(pCallBlock->CallerId, pEvenParams->TelNo) != 0)
        {
            if (
                (TelNoPrioritySelectIndex == 0 && pEvenParams->MachineType == PACKAGE_MACHINE_TYPE_PBX) ||
                (TelNoPrioritySelectIndex == 1 && pEvenParams->MachineType == PACKAGE_MACHINE_TYPE_CALLERID_BOX) ||
                (TelNoPrioritySelectIndex == 2 && pEvenParams->MachineType == PACKAGE_MACHINE_TYPE_VOICE_CARD)
                )
            {
                memset(pCallBlock->CallerId, 0x00, 50);
                memcpy(pCallBlock->CallerId, pEvenParams->TelNo, 50);
            }
        }
        break;
    case EVENT_ID_OUTLINE_IN:
        /*拷貝相關的訊息到該CallBlock*/
        CopyExternalCallIn(pCallBlock, pEvenParams);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_IN);
        /*
            在Wait Line Talking時又收到Call In時
            要判斷之前儲存的電話號碼是否與這個事件的電話號碼是否相同,
            如果不同則依據電話號碼優先的設定來更新CallBlock的電話號碼
        */
        TelNoPrioritySelectIndex = AfxGetApp()->GetProfileInt("SystemSetting", "TelNoPrioritySelect", 0);
        if (strcmp(pCallBlock->CallerId, pEvenParams->TelNo) != 0)
        {
            if (
                (TelNoPrioritySelectIndex == 0 && pEvenParams->MachineType == PACKAGE_MACHINE_TYPE_PBX) ||
                (TelNoPrioritySelectIndex == 1 && pEvenParams->MachineType == PACKAGE_MACHINE_TYPE_CALLERID_BOX) ||
                (TelNoPrioritySelectIndex == 2 && pEvenParams->MachineType == PACKAGE_MACHINE_TYPE_VOICE_CARD)
                )
            {
                memset(pCallBlock->CallerId, 0x00, 50);
                memcpy(pCallBlock->CallerId, pEvenParams->TelNo, 50);
            }
        }
        break;
    case EVENT_ID_OUTLINE_TALKING:
        /*跳轉到STATE_ID_RING_TALKING狀態*/
        GotoState(pCallBlock, STATE_ID_RING_TALKING);
        /*拷貝相關的訊息到該CallBlock*/
        CopyExternalCallTalking(pCallBlock, pEvenParams);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_TALKING);
        break;
    case EVENT_ID_VOICE_REC_FILE_UUID:
        memcpy(pCallBlock->VoiceRecFileUUID, pEvenParams->VoiceRecFileName, 32);
        break;
    }

    return 0;
}

static int StateWaitExtTalking(PCallBlock_T pCallBlock, PEventParamsT pEvenParams, UINT EventID)
{
    CShowLinesStatusDlg* pShowLinesStatusDlg = (CShowLinesStatusDlg*)pADICTICallCenterDlg->m_pWndModleDlg[CTRL_ID_SHOW_PORT_STATUS];
    switch (EventID)
    {
    case EVENT_ID_EXTLINE_TALKING:
        /*跳轉到STATE_ID_RING_TALKING狀態*/
        GotoState(pCallBlock, STATE_ID_RING_TALKING);
        /*拷貝相關的訊息到該CallBlock*/
        CopyInternalCallTalking(pCallBlock, pEvenParams);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_TALKING);
        break;
    case EVENT_ID_EXTLINE_MAKE:
        /*拷貝相關的訊息到該CallBlock*/
        CopyInternalCallMake(pCallBlock, pEvenParams);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_OUT);
        break;
    case EVENT_ID_CALL_RELEASE:
    case EVENT_ID_EXTLINE_DISCONNECT:
        /*跳轉到STATE_ID_IDLE狀態*/
        GotoState(pCallBlock, STATE_ID_IDLE);
        /*拷貝相關的訊息到該CallBlock*/
        CopyInternalCallEnd(pCallBlock, pEvenParams);
        SaveToDBBuffer(pCallBlock);
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_OFF);
        ReleaseCallBlockByFromExtVPort(pCallBlock->FromExtVPort);
        break;
    case EVENT_ID_OUTLINE_IN:
        SaveToDBBuffer(pCallBlock);
        /*跳轉到STATE_ID_WAIT_LINE_TALKING狀態*/
        GotoState(pCallBlock, STATE_ID_WAIT_LINE_TALKING);
        /*拷貝相關的訊息到該CallBlock*/
        CopyExternalCallIn(pCallBlock, pEvenParams);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_IN);
        break;
    case EVENT_ID_VOICE_REC_FILE_UUID:
        memcpy(pCallBlock->VoiceRecFileUUID, pEvenParams->VoiceRecFileName, 32);
        break;
    }

    return 0;
}

static int StateTalking(PCallBlock_T pCallBlock, PEventParamsT pEvenParams, UINT EventID)
{
    CShowLinesStatusDlg* pShowLinesStatusDlg = (CShowLinesStatusDlg*)pADICTICallCenterDlg->m_pWndModleDlg[CTRL_ID_SHOW_PORT_STATUS];
    switch (EventID)
    {
    case EVENT_ID_OUTLINE_TALKING:
    case EVENT_ID_OUTLINE_DISCONNECT:
        /*拷貝相關的訊息到該CallBlock*/
        CopyExternalCallEnd(pCallBlock, pEvenParams);
        SaveToDBBuffer(pCallBlock);
    case EVENT_ID_CALL_RELEASE:
    case EVENT_ID_OUTLINE_IN:
    case EVENT_ID_OUTLINE_OUT:
        /*跳轉到STATE_ID_IDLE狀態*/
        GotoState(pCallBlock, STATE_ID_IDLE);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_OFF);
        ReleaseCallBlockByOutVPort(pCallBlock->OutVPort);
        break;
    case EVENT_ID_EXTLINE_TALKING:
    case EVENT_ID_EXTLINE_DISCONNECT:
        /*拷貝相關的訊息到該CallBlock*/
        CopyInternalCallEnd(pCallBlock, pEvenParams);
        SaveToDBBuffer(pCallBlock);
    case EVENT_ID_EXTLINE_MAKE:
        /*跳轉到STATE_ID_IDLE狀態*/
        GotoState(pCallBlock, STATE_ID_IDLE);
        /*顯示LED燈號*/
        if (pShowLinesStatusDlg)
            pShowLinesStatusDlg->SetLineCallStatusLed(pCallBlock, STATUS_CALL_OFF);
        ReleaseCallBlockByFromExtVPort(pCallBlock->FromExtVPort);
        break;
    case EVENT_ID_VOICE_REC_FILE_UUID:
        memcpy(pCallBlock->VoiceRecFileUUID, pEvenParams->VoiceRecFileName, 32);
        break;
    }

    return 0;
}

STATE_FUNC* StateFunctionTable[] =
{
    StateIdle,
    StateWaitLineTalking,
    StateWaitExtTalking,
    StateTalking,
};

static int SentBufferEvent(PPackageEventBuffer_T pPackageEventBuffer)
{
    PEventParamsT pEventParams = &pPackageEventBuffer->EventParams;
    UINT EventID = pPackageEventBuffer->EventID;
    bool IsError = FALSE;
    class CDatabaseAccessURL DatabaseAccessURL;
    int Ret;

    PCallBlock_T pCallBlock = NULL;
    if (pEventParams)
    {
        memset(&pEventParams->FromExt, 0x00, sizeof(ExtData_T));
        memset(&pEventParams->ToExt, 0x00, sizeof(ExtData_T));
        /*在資料庫找到對應的虛擬OutPort*/
        // GetOutVPort 找不到對應虛擬線路時（這個實體埠在遷移後的資料庫裡
        // 沒有佈線資料，例如日後新增/改接的硬體）不會寫入
        // pEventParams->OutVPort，所以呼叫前先明確歸零——OutVPort 合法值是
        // 1~240，0 保證不會沿用這個緩衝區欄位裡殘留的舊值，把這次事件誤
        // 配對到別條線路上。這裡刻意不拿傳回值去設 IsError：內線事件本來
        // 就沒有實體外線埠，GetOutVPort 對這類事件「查不到」是正常現象、
        // 不是錯誤，只有真的需要用到 OutVPort 的外線事件（下面 switch 裡
        // 的 EVENT_ID_OUTLINE_*）才需要在意這次查詢是否成功，用
        // OutVPortFound 各自判斷。
        pEventParams->OutVPort = 0;
        BOOL OutVPortFound = (DatabaseAccessURL.GetOutVPort(pEventParams->MachineType, pEventParams->MachineID, pEventParams->PhyOutPort, &pEventParams->OutVPort, NULL) == ERROR_CODE_SUCCESS);
        /*在資料庫找到分機碼所對應的虛擬Port*/
        if (pEventParams->FromExtNum > 0)
        {
            Ret = DatabaseAccessURL.GetExtVPortByExtNum(pEventParams->MachineID, pEventParams->FromExtNum, &pEventParams->FromExt, NULL);
            if (Ret < 0)
            {
                IsError = TRUE;
            }
        }
        if (pEventParams->ToExtNum > 0)
        {
            Ret = DatabaseAccessURL.GetExtVPortByExtNum(pEventParams->MachineID, pEventParams->ToExtNum, &pEventParams->ToExt, NULL);
            if (Ret < 0)
            {
                IsError = TRUE;
            }
        }
        if (IsError == FALSE)
        {
            switch (EventID)
            {
            /*External Call*/
            case EVENT_ID_OUTLINE_IN:
            case EVENT_ID_OUTLINE_OUT:
                /*
                    先檢查這個事件的實體外線編號所對應的虛擬外線CallBlock是否有使用.
                    如果沒有使用表示這個是新的撥出或撥入
                */
                // OutVPortFound 為 FALSE 代表這個實體外線埠在資料庫裡查不到
                // 對應的虛擬線路（沒佈線/尚未指派），此時 OutVPort 只是歸零
                // 後的預留值，不能拿去比對/建立 CallBlock，否則可能誤配對
                // 到另一條真的 OutVPort 剛好還沒被指派、目前也是 0 的線路。
                if (OutVPortFound)
                {
                    pCallBlock = (PCallBlock_T)FindCallBlockByOutVPort(pEventParams->OutVPort);
                    if (pCallBlock == NULL)
                    {
                        /*根據虛擬外線編號建立一個新的CallBlock*/
                        pCallBlock = NewCallBlockByOutVPort(pEventParams->OutVPort);
                    }
                }
                break;
            case EVENT_ID_OUTLINE_DISCONNECT:
            case EVENT_ID_OUTLINE_TALKING:
                /*根據虛擬外線編號找出CallBlock指標*/
                if (OutVPortFound)
                {
                    pCallBlock = (PCallBlock_T)FindCallBlockByOutVPort(pEventParams->OutVPort);
                    if (pCallBlock)
                        strcpy_s(pCallBlock->CallerId, 50, pEventParams->TelNo);
                }
                break;
            case EVENT_ID_EXTLINE_MAKE:
                /*
                    先檢查這個事件的內線互撥的FromExt所對應的虛擬內線CallBlock是否有使用.
                    如果沒有使用表示這個是新的內線互撥
                */
                pCallBlock = (PCallBlock_T)FindCallBlockByFromExtVPort(pEventParams->FromExt.ExtVPort);
                if (pCallBlock == NULL)
                {
                    /*根據事件的FromExtNum建立一個新的CallBlock*/
                    pCallBlock = NewCallBlockByFromExtVPort(pEventParams->FromExtNum);
                }
                break;
            case EVENT_ID_EXTLINE_DISCONNECT:
            case EVENT_ID_EXTLINE_TALKING:
                /*跟據內線FromExt的虛擬內線編號找到CallBlock指標*/
                pCallBlock = (PCallBlock_T)FindCallBlockByFromExtVPort(pEventParams->FromExt.ExtVPort);
                break;
            case EVENT_ID_VOICE_REC_FILE_UUID:
                if (pEventParams->PhyOutPort != 0 && pEventParams->ToExtNum != 0 && OutVPortFound)
                {
                    /*跟據外線虛擬編號找到CallBlock指標*/
                    pCallBlock = (PCallBlock_T)FindCallBlockByOutVPort(pEventParams->OutVPort);
                }
                else if (pEventParams->FromExtNum != 0)
                {
                    /*跟據外線虛擬Port找到CallBlock指標*/
                    pCallBlock = (PCallBlock_T)FindCallBlockByFromExtVPort(pEventParams->FromExt.ExtVPort);
                }
                break;
            }
        }
        if (pCallBlock)
        {
            UINT StateID = pCallBlock->CurrentState;
            //printf("Line[%d]: Send [%s] in [%s]\n", OutVPort + 1, EventString[EventID], StateString[StateID]);
            StateFunctionTable[StateID](pCallBlock, pEventParams, EventID);
        }
    }
    return 0;
}

int InitCallStateRecordEngine()
{
    /*初始外部通話的CallBlock及內部通話的CallBlock*/
    PCallBlock_T pCallBlock;
    memset(gCallBlock, 0x00, sizeof(CallBlock_T) * 240);
    memset(gPackageEventBuffer, 0x00, sizeof(PackageEventBuffer_T) * BufferSize);
    hThreadCallRecordHandler = (HANDLE)CreateThread(NULL, 2000, (unsigned long(__stdcall*)(void*))ThreadCallRecordHandler, NULL, 0, NULL);
    hThreadEventBufferHandler = (HANDLE)CreateThread(NULL, 2000, (unsigned long(__stdcall*)(void*))ThreadEventBufferHandler, NULL, 0, NULL);
    return 0;
}

int SendEvent(PEventParamsT pEventParams, UINT EventID)
{
    class CDatabaseAccessURL DatabaseAccessURL;
    int Ret = 0;
    BOOL IsError = FALSE;
    ExtData_T ExtData;
    int ExtNum;
    int OutPortNum;

    for (int i = 0; i < BufferSize; i++)
    {
        PPackageEventBuffer_T pPackageEventBuffer = &gPackageEventBuffer[i];
        IsError = FALSE;
        if (pPackageEventBuffer->IsInUsed == FALSE)
        {
            if (pEventParams->PhyOutPort > 0)
            {
                Ret = DatabaseAccessURL.GetMachineOutPortsByMachineID(pEventParams->MachineType, pEventParams->MachineID, &OutPortNum, NULL);
                if (Ret < 0)
                {
                    //MessageBoxEx(0, Message, "錯誤", MB_ICONERROR, 0);
                    IsError = TRUE;
                }
                else
                {
                    if (Ret < pEventParams->PhyOutPort)
                    {
                        IsError = TRUE;
                        Ret = -2;
                    }
                }
            }

            if (pEventParams->MachineType == MACHINE_TYPE_PBX)
            {
                if (pEventParams->FromExtNum > 0)
                {
                    Ret = DatabaseAccessURL.GetExtVPortByExtNum(pEventParams->MachineID, pEventParams->FromExtNum, &ExtData, NULL);
                    if (Ret < 0)
                    {
                        //MessageBoxEx(0, Message, "錯誤", MB_ICONERROR, 0);
                        IsError = TRUE;
                        Ret = -1;
                    }
                }
                if (pEventParams->ToExtNum > 0)
                {
                    Ret = DatabaseAccessURL.GetExtVPortByExtNum(pEventParams->MachineID, pEventParams->ToExtNum, &ExtData, NULL);
                    if (Ret < 0)
                    {
                        //MessageBoxEx(0, Message, "錯誤", MB_ICONERROR, 0);
                        IsError = TRUE;
                        Ret = -1;
                    }
                }
            }
            if (IsError == FALSE)
                AddEventToBuffer(pPackageEventBuffer, pEventParams, EventID);
            break;
        }
    }

    return Ret;
}

PCallBlock_T GetGlobelCallBlockPtr()
{
    PCallBlock_T pCallBlock = (PCallBlock_T)gCallBlock;

    return pCallBlock;
}

void SetMainWndPtr()
{
    pADICTICallCenterDlg = (CADICTICallCenterDlg*)AfxGetMainWnd();
}

void ReleaseCallStateBlock(UINT MachineType, UINT MachineID)
{
    PCallBlock_T pCallBlock;

    for (int i = 0; i < 240; i++)
    {
        pCallBlock = (PCallBlock_T)&gCallBlock[i];
        if (pCallBlock->MachineType == MachineType && pCallBlock->MachineID == MachineID)
        {
            if (pCallBlock)
            {
                UINT StateID = pCallBlock->CurrentState;
                StateFunctionTable[StateID](pCallBlock, NULL, EVENT_ID_CALL_RELEASE);
            }
        }
    }

}