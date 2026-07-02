<?php
	error_reporting(0);
	$jsondata = array();
	$Password1 = htmlspecialchars($_GET["Password"]);
	$Password2 = htmlspecialchars($_POST["Password"]);
	if($Password1 != NULL)
	{
		$Password = $Password1;
	}
	else if($Password2 != NULL)
	{
		$Password = $Password2;
	}
	
	if($Password == NULL)
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

	$String = "admin:$Password@adicti.com.tw";   
 	$SupervisorUUID = $tool->GenUUID($String);
	$TableName = 'admin';
	$sql="SELECT * FROM $TableName WHERE supervisor_uuid='$SupervisorUUID'";
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
	$ID = array();
	$UUID = array();
	$Name = array();
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$UUID[$Counter] = odbc_result($result, 'supervisor_uuid');
		$Name[$Counter] = odbc_result($result, 'name');
		$Counter++;
	}
	
	if($Counter == 1)
	{
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['fields'] = array('ID','Key');
		for($x = 0; $x < $Counter; $x++)
		{
			$jsondata['data'] = array($ID[$x], $UUID[$x]);
		}
	}
	else
	{
		if($EmployeeId == 'admin')
		{
			$jsondata['status'] = false;		
			$jsondata['esptime'] = $endtime - $starttime;
			$jsondata['message'] = "admin Account is not exist, Please set the admin password first!!!";
		}
		else
		{
			$jsondata['status'] = false;		
			$jsondata['esptime'] = $endtime - $starttime;
			$jsondata['message'] = "AdminLogin Fail, Please Check Id or Password!!!";		
		}
	}
	
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>