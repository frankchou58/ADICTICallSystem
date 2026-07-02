<?php
	error_reporting(0);
	$jsondata = array();
	$UUID1 = htmlspecialchars($_GET["Key"]);
	$Password1 = htmlspecialchars($_GET["Password"]);
	$UUID2 = htmlspecialchars($_POST["Key"]);
	$Password2 = htmlspecialchars($_POST["Password"]);
	if($UUID1 != NULL || $Password1 != NULL)
	{
		$UUID = $UUID1;
		$Password = $Password1;
	}
	else if($UUID2 != NULL || $Password2 != NULL)
	{
		$UUID = $UUID2;
		$Password = $Password2;
	}
	
	if($UUID == NULL || $Password == NULL)
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
		$jsondata['message'] = $Connected;		
		goto endsql;
	}
	$TableName = 'operators';
	$sql="SELECT * FROM $TableName WHERE operator_uuid='$UUID'";
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
	if(!$result)
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "Database SELECT $table Error: " . odbc_error() . "!!!";
		goto endsql;
	}
	$EmployeeId = array();
	$Counter = 0;
	while ($row = odbc_fetch_row($result))
	{
		$EmployeeId[$Counter] = odbc_result($result, 'employee_id');
		$Counter++;
	}
	if($Counter == 1)
	{
		$Id = $EmployeeId[0];
		$String = "$Id:$Password@adicti.com.tw";   
		$EmployeeUUID = $tool->GenUUID($String);
		$sql="UPDATE $TableName SET operator_uuid='$EmployeeUUID' WHERE operator_uuid='$UUID'";
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
		$jsondata['UUID'] = $EmployeeUUID;		
	}
	else
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "Fail Set Password!!!";
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>