<?php
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
	error_reporting(E_ERROR | E_PARSE | E_CORE_ERROR | E_COMPILE_ERROR | E_USER_ERROR | E_RECOVERABLE_ERROR);
	$jsondata = array();
	$StartDate1 = htmlspecialchars(@$_GET['StartDate']);
	$EndDate1 = htmlspecialchars(@$_GET['EndDate']);
	$TEL1 = htmlspecialchars(@$_GET['TEL']);
	$CustID1 = htmlspecialchars(@$_GET['CustID']); 
	$CustName1 = htmlspecialchars(@$_GET['CustName']); 
	$Status1 = htmlspecialchars(@$_GET['Status']);
	$ExtNum1 = htmlspecialchars(@$_GET['ExtNum']);
	$Addr1 = htmlspecialchars(@$_GET['Addr']);
	$StartDate2 = htmlspecialchars(@$_POST['StartDate']);
	$EndDate2 = htmlspecialchars(@$_POST['EndDate']);
	$TEL2 = htmlspecialchars(@$_POST['TEL']);
	$CustID2 = htmlspecialchars(@$_POST['CustID']); 
	$CustName2 = htmlspecialchars(@$_POST['CustName']); 
	$Status2 = htmlspecialchars(@$_POST['Status']);
	$ExtNum2 = htmlspecialchars(@$_POST['ExtNum']);
	$Addr2 = htmlspecialchars(@$_POST['Addr']);

	if($StartDate1 != NULL || $TEL1 != NULL || $CustID1 != NULL || $CustName1 != NULL || $Status1 != NULL || $ExtNum1 != NULL || $EndDate1 != NULL)
	{
		$StartDate = $StartDate1;
		$EndDate = $EndDate1;
		$TEL = $TEL1;
		$CustID = $CustID1;
		$CustName = $CustName1;
		$Status = $Status1;
		$ExtNum = $ExtNum1;
		$Addr = $Addr1;
	}
	else if($StartDate2 != NULL || $TEL2 != NULL || $CustID2 != NULL || $CustName2 != NULL || $Status2 != NULL || $ExtNum2 != NULL || $EndDate2 != NULL )
	{
		$StartDate = $StartDate2;
		$EndDate = $EndDate2;
		$TEL = $TEL2;
		$CustID = $CustID2;
		$CustName = $CustName2;
		$Status = $Status2;
		$ExtNum = $ExtNum2;
		$Addr = $Addr2;
	}

	if($StartDate == NULL || $EndDate == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Params Must Have [StartDate] & [EndDate] | [TEL] | [CustID] | [CustName] | [Status] | [ExtNum]';		
		goto end;
	}
	
	require_once 'tools.php';
	$tool = new tools();
	$Connected = $tool->SqlConnectCPF144();
	if($Connected < 0)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'SQL Connect Fail';		
		goto end;
	}

	$TableName = 'Inbound';
	$sql="SELECT * FROM $TableName WHERE InboundDate>='$StartDate' AND InboundDate<='$EndDate'";
	if($TEL)
		$sql = $sql . " AND InboundTEL LIKE '%$TEL%'";
	if($CustID)
		$sql = $sql . " AND InboundCustID='$CustID'";
	if($CustName)
	{
		$Strbig5 = iconv("UTF-8", "big5", $CustName);
		$sql = $sql . " AND InboundCustName LIKE '%$Strbig5%'";
	}
	if($ExtNum)
		$sql = $sql . " AND InboundExtNum='$ExtNum'";
	if($Addr)
	{
		$Strbig5 = iconv("UTF-8", "big5", $Addr);
		$sql = $sql . " AND InboundCustAddr LIKE '%$Strbig5%'";
	}
	if($Status)
	{
		if($Status == 1)
			$sql = $sql . " AND InboundDirection='撥入' AND InboundTotalTime IS NOT NULL";
		else if($Status == 2)
			$sql = $sql . " AND InboundDirection='撥入' AND InboundTotalTime IS NULL";
		else if($Status == 3)
			$sql = $sql . " AND InboundDirection='撥入' AND InboundTotalTime IS NULL AND InboundNeedReturnCall='1'";
		else if($Status == 4)
			$sql = $sql . " AND InboundDirection='撥出'";
		else if($Status == 5)
			$sql = $sql . " AND InboundTotalTime > '1900-01-01 00:01:30.000'";
		else if($Status == 6)
			$sql = $sql . " AND InboundTotalTime > '1900-01-01 00:02:00.000'";
		else if($Status == 7)
			$sql = $sql . " AND InboundTotalTime > '1900-01-01 00:03:00.000'";
		else if($Status == 8)
			$sql = $sql . " AND InboundTotalTime > '1900-01-01 00:04:00.000'";
		else
		{
			$jsondata['status'] = false;		
			$jsondata['message'] = 'Status Range = 1 ~ 8';		
			goto endsql;
		}
	}

	//echo 'Sql = ' . $sql . "\n";

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
			$Counter++;
		}
		$jsondata['status'] = true;		
		$jsondata['number'] = $Counter;
		$jsondata['fields'] = array('[ID]', '[Date]', '[Time]', '[Direction]', '[LineID]', '[TEL]'
				, '[CustID]', '[CustName]', '[Memos]', '[CustAddr]', '[TotalTime]', '[RecordFile]'
				, '[Identify]', '[NeedReturnCall]', '[ExtNum]', '[ReturnExtNum]');
		for($x = 0; $x < $Counter; $x++)
		{
			//使用NVARCHAR或NCHAR的欄位都要先轉碼
			$InboundIDUni = iconv("big5", "UTF-8", $InboundID[$x]);
			//$InboundDateUni = iconv("big5", "UTF-8", $InboundDate[$x]); 
			//$InboundTimeUni = iconv("big5", "UTF-8", $InboundTime[$x]); 
			$date = new DateTime($InboundDate[$x]);
			$time = new DateTime($InboundTime[$x]);
			$InboundDateUni = $date->format('Y-m-d');
			$InboundTimeUni = $time->format('H:i:s');
			$InboundDirectionUni = iconv("big5", "UTF-8", $InboundDirection[$x]);
			$InboundLineIDUni = iconv("big5", "UTF-8", $InboundLineID[$x]);
			$InboundTELUni = iconv("big5", "UTF-8", $InboundTEL[$x]);
			$InboundCustIDUni = iconv("big5", "UTF-8", $InboundCustID[$x]);
			$InboundCustNameUni = iconv("big5", "UTF-8", $InboundCustName[$x]);
			$InboundMemosUni = iconv("big5", "UTF-8", $InboundMemos[$x]);
			$InboundCustAddrUni = iconv("big5", "UTF-8", $InboundCustAddr[$x]);
			$Dur = iconv("big5", "UTF-8", '沒接通');
			if(!empty($InboundTotalTime[$x]))
			{
				$Duration = new DateTime($InboundTotalTime[$x]);
				$Dur = $Duration->format('H:i:s');
			}
			//$InboundTotalTimeUni = iconv("big5", "UTF-8", $InboundTotalTime[$x]);
			$InboundRecordFileUni = iconv("big5", "UTF-8", $InboundRecordFile[$x]);
			$InboundIdentifyUni = iconv("big5", "UTF-8", $InboundIdentify[$x]);
			$InboundNeedReturnCallUni = iconv("big5", "UTF-8", $InboundNeedReturnCall[$x]);
			$InboundExtNumUni = iconv("big5", "UTF-8", $InboundExtNum[$x]);
			$InboundReturnExtNumUni = iconv("big5", "UTF-8", $InboundReturnExtNum[$x]);
			$jsondata['data'][$x] = array($InboundIDUni, $InboundDateUni, $InboundTimeUni, $InboundDirectionUni, $InboundLineIDUni
				, $InboundTELUni, $InboundCustIDUni, $InboundCustNameUni, $InboundMemosUni, $InboundCustAddrUni
				, $Dur, $InboundRecordFileUni, $InboundIdentifyUni, $InboundNeedReturnCallUni, $InboundExtNumUni, $InboundReturnExtNumUni);
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
