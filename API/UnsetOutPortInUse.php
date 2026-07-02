<?php
	error_reporting(0);
	$jsondata = array();
	$OutPort1 = htmlspecialchars($_GET["OutVPort"]);
	$OutPort2 = htmlspecialchars($_GET["OutVPort"]);
	if($OutPort1 != NULL)
	{
		$OutPort = $OutPort1;
	}
	else if($ExtPort2 != NULL)
	{
		$OutPort = $OutPort2;
	}
	
	if($OutPort == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!!';		
		goto end;
	}
	
	require_once 'tools.php';
	$tool = new tools();
	
	$Connected = $tool->SqlConnect();
	if($Connected < 0)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'SQL Connection Fail';		
		goto endsql;
	}

	$TableName = 'outline';
	$sql="UPDATE $TableName SET line_in_used='0' WHERE line_no='$OutPort'";
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
  list($endtime, $sec) = explode(" ", microtime());
	if(!$result)
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "Database SELECT $table Error: " . odbc_error() . "!!!";
		goto endsql;
	}
	
	$jsondata['status'] = true;		
	$jsondata['esptime'] = $endtime - $starttime;
	$jsondata['message'] = "Success Set Out Virtual Port:$OutPort to None Used";		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>