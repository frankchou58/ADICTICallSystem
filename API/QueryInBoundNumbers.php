<?php
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
	error_reporting(E_ERROR | E_PARSE | E_CORE_ERROR | E_COMPILE_ERROR | E_USER_ERROR | E_RECOVERABLE_ERROR);
	$jsondata = array();
/*
	$Telphone1 = htmlspecialchars(@$_GET['Telphone']);
	$Telphone2 = htmlspecialchars(@$_POST['Telphone']);

	if($Telphone1 != NULL)
	{
		$Telphone = $Telphone1;
	}
	else if($Telphone2 != NULL)
	{
		$Telphone = $Telphone2;
	}

	if($Telphone == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Params Must Have [Telphone]';		
		goto end;
	}
*/	
	require_once 'tools.php';
	$tool = new tools();
	$Connected = $tool->SqlConnectCPF144();
	if($Connected < 0)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = $Connected;		
		goto end;
	}
	$TableName = 'Inbound';
	$sql="SELECT * FROM $TableName";
	$result = $tool->SqlExec($Connected, $sql);
	if($result)
	{
		$Counter = 0;
		$InboundID = array();
		$InboundDate = array();
		$InboundTime = array();
		$InboundDirection = array();
		$InboundLineID = array();
		$InboundTEL = array();
		$InboundCustID = array();
		$InboundCustName = array();
		$InboundMemos = array();
		$InboundCustAddr = array();
		$InboundTotalTime = array();
		$InboundRecordFile = array();
		$InboundIdentify = array();
		$InboundNeedReturnCall = array();
		$InboundExtNum = array();
		$InboundReturnExtNum = array();
		while ($row = $tool->SqlFetch($result))
		{
/*
			$InboundID[$Counter] = $tool->FetchOdbcData($result, 'InboundID');
			$InboundDate[$Counter] = $tool->FetchOdbcData($result, 'InboundDate');
			$InboundTime[$Counter] = $tool->FetchOdbcData($result, 'InboundTime');
			$InboundDirection[$Counter] = $tool->FetchOdbcData($result, 'InboundDirection');
			$InboundLineID[$Counter] = $tool->FetchOdbcData($result, 'InboundLineID');
			$InboundTEL[$Counter] = $tool->FetchOdbcData($result, 'InboundTEL');
			$InboundCustID[$Counter] = $tool->FetchOdbcData($result, 'InboundCustID');
			$InboundCustName[$Counter] = $tool->FetchOdbcData($result, 'InboundCustName');
			$InboundMemos[$Counter] = $tool->FetchOdbcData($result, 'InboundMemos');
			$InboundCustAddr[$Counter] = $tool->FetchOdbcData($result, 'InboundCustAddr');
			$InboundTotalTime[$Counter] = $tool->FetchOdbcData($result, 'InboundTotalTime');
			$InboundRecordFile[$Counter] = $tool->FetchOdbcData($result, 'InboundRecordFile');
			$InboundIdentify[$Counter] = $tool->FetchOdbcData($result, 'InboundIdentify');
			$InboundNeedReturnCall[$Counter] = $tool->FetchOdbcData($result, 'InboundNeedReturnCall');
			$InboundExtNum[$Counter] = $tool->FetchOdbcData($result, 'InboundExtNum');
			$InboundReturnExtNum[$Counter] = $tool->FetchOdbcData($result, 'InboundReturnExtNum');
*/
			$Counter++;
		}
		$jsondata['status'] = true;		
		$jsondata['number'] = $Counter;
/*
		$jsondata['fields'] = array('[InboundID]', '[InboundDate]', '[InboundTime]', '[InboundDirection]', '[InboundLineID]', '[InboundTEL]'
				, '[InboundCustID]', '[InboundCustName]', '[InboundMemos]', '[InboundCustAddr]', '[InboundTotalTime]', '[InboundRecordFile]'
				, '[InboundIdentify]', '[InboundNeedReturnCall]', '[InboundExtNum]', '[InboundReturnExtNum]');
*/
		for($x = 0; $x < $Counter; $x++)
		{
/*
			//使用NVARCHAR或NCHAR的欄位都要先轉碼
			$InboundIDUni = iconv("big5", "UTF-8", $InboundID[$x]);

			$InboundDateUni = iconv("big5", "UTF-8", $InboundDate[$x]); 
			$InboundTimeUni = iconv("big5", "UTF-8", $InboundTime[$x]); 
			$InboundDirectionUni = iconv("big5", "UTF-8", $InboundDirection[$x]);
			$InboundLineIDUni = iconv("big5", "UTF-8", $InboundLineID[$x]);
			$InboundTELUni = iconv("big5", "UTF-8", $InboundTEL[$x]);
			$InboundCustIDUni = iconv("big5", "UTF-8", $InboundCustID[$x]);
			$InboundCustNameUni = iconv("big5", "UTF-8", $InboundCustName[$x]);
			$InboundMemosUni = iconv("big5", "UTF-8", $InboundMemos[$x]);
			$InboundCustAddrUni = iconv("big5", "UTF-8", $InboundCustAddr[$x]);
			$InboundTotalTimeUni = iconv("big5", "UTF-8", $InboundTotalTime[$x]);
			$InboundRecordFileUni = iconv("big5", "UTF-8", $InboundRecordFile[$x]);
			$InboundIdentifyUni = iconv("big5", "UTF-8", $InboundIdentify[$x]);
			$InboundNeedReturnCallUni = iconv("big5", "UTF-8", $InboundNeedReturnCall[$x]);
			$InboundExtNumUni = iconv("big5", "UTF-8", $InboundExtNum[$x]);
			$InboundReturnExtNumUni = iconv("big5", "UTF-8", $InboundReturnExtNum[$x]);

			$jsondata['data'][$x] = array($InboundIDUni, $InboundDateUni, $InboundTimeUni, $InboundDirectionUni, $InboundLineIDUni
				, $InboundTELUni, $InboundCustIDUni, $InboundCustNameUni, $InboundMemosUni, $InboundCustAddrUni
				, $InboundTotalTimeUni, $InboundRecordFileUni, $InboundIdentifyUni, $InboundNeedReturnCallUni, $InboundExtNumUni, $InboundReturnExtNumUni);
*/
		}
	}
	else
	{
		$ErrorCode = $tool->SqlError();
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Query Fail: ' . $ErrorCode;		
	}
endsql:
	$tool->SqlClose($Connected);
end:
	echo json_encode($jsondata);	
?>
