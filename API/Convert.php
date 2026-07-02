<?php
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
	error_reporting(E_ERROR | E_PARSE | E_CORE_ERROR | E_COMPILE_ERROR | E_USER_ERROR | E_RECOVERABLE_ERROR);
	$jsondata = array();

	require_once 'tools.php';
	$tool = new tools();
	$Connected = $tool->SqlConnectCPF144();
	if($Connected < 0)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = $Connected;		
		goto end;
	}

	$starttime = microtime(true);
	$TableName = 'Cust';
	$sql="SELECT [CustNo] FROM $TableName";
	
	$result = $tool->SqlExec($Connected, $sql);
	if($result)
	{
		$Counter = 0;
		$CustNo = array();
		while ($row = $tool->SqlFetch($result))
		{
			$CustNo[$Counter] = $tool->FetchOdbcData($result, 'CustNo');
			$Counter++;
		}
		for($x = 0; $x < $Counter; $x++)
		{
			$sql="UPDATE [dbo].[Cust] SET [CustID] = $CustNo[$x] WHERE [CustNo] = $CustNo[$x]";
			$result = $tool->SqlExec($Connected, $sql);
		}
		$endtime = microtime(true);
		$jsondata['status'] = true;		
		$jsondata['esptime(sec)'] = $endtime - $starttime;
		$jsondata['message'] = 'Completed!!!!!';
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
