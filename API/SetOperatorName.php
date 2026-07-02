<?php
	error_reporting(0);
	$jsondata = array();
	$UUID1 = htmlspecialchars($_GET["Key"]);
	$Name1 = htmlspecialchars($_GET["Name"]);
	$UUID2 = htmlspecialchars($_POST["Key"]);
	$Name2 = htmlspecialchars($_POST["Name"]);
	if($UUID1 != NULL || $Name1 != NULL)
	{
		$UUID = $UUID1;
		$Name = $Name1;
	}
	else if($UUID2 != NULL || $Name2 != NULL)
	{
		$UUID = $UUID2;
		$Name = $Name2;
	}
	
	if($UUID == NULL || $Name == NULL)
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

	$StrName = iconv("UTF-8", "big5", $Name);	
	$TableName = 'operators';
	$sql="UPDATE $TableName SET name='$StrName' WHERE operator_uuid='$UUID'";
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
	$jsondata['message'] = "Success Set [$Name] to [$UUID]!!!";		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>