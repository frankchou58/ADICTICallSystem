<?php
	error_reporting(0);
	$jsondata = array();
	require_once 'tools.php';
	$tool = new tools();
	
	$Connected = $tool->SqlConnect();
	if($Connected < 0)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'SQL Connection Fail';		
		goto endsql;
	}

  list($starttime, $sec) = explode(" ", microtime());
	$Ret = $tool->CreateOperatorTable($Connected);
	if($Ret != 'ok')
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = $Ret;		
		goto endsql;
	}
	else
	{
  	list($endtime, $sec) = explode(" ", microtime());
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['message'] = 'Success Create Operator table';		
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>