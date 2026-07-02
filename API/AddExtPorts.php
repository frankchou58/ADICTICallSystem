<?php
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
	//for($PortNo = 1; $PortNo <= 3; $PortNo++)
	{
		$Ret = $tool->AddExtPort($Connected, 14);
		if($Ret != 'ok')
		{
			$jsondata['status'] = false;		
			$jsondata['message'] = $Ret;		
			goto endsql;
		}			
	}
  list($endtime, $sec) = explode(" ", microtime());
	$jsondata['status'] = true;		
	$jsondata['esptime'] = ($endtime - $starttime) * 1000;
	$jsondata['message'] = 'Success Create extline table';		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>