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

 	$TableName = 'admin';
	$sql="SELECT * FROM $TableName";
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
  list($endtime, $sec) = explode(" ", microtime());
	if(!$result)
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "Database SELECT $table Error: " . odbc_error() . "!!!";
		goto endsql;
	}
	$Counter = 0;
	while ($row = odbc_fetch_row($result))
	{
		$Counter++;
	}
	if($Counter > 0)
	{	
		$jsondata['status'] = true;		
		$jsondata['numbers'] = "$Counter";
		$jsondata['esptime'] = $endtime - $starttime;
	}
	else
	{
		$jsondata['status'] = false;		
		$jsondata['numbers'] = "$Counter";
		$jsondata['esptime'] = $endtime - $starttime;
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>