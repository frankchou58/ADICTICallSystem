<?php
	error_reporting(0);
	$jsondata = array();
	$EmployeeId1 = htmlspecialchars($_GET["Id"]);
	$Password1 = htmlspecialchars($_GET["Password"]);
	$EmployeeId2 = htmlspecialchars($_POST["Id"]);
	$Password2 = htmlspecialchars($_POST["Password"]);
	if($EmployeeId1 != NULL || $Password1 != NULL)
	{
		$EmployeeId = $EmployeeId1;
		$Password = $Password1;
	}
	else if($EmployeeId2 != NULL || $Password2 != NULL)
	{
		$EmployeeId = $EmployeeId2;
		$Password = $Password2;
	}
	
	if($EmployeeId == NULL || $Password == NULL)
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

	$String = "$EmployeeId:$Password@adicti.com.tw";   
 	$operatoruuid = $tool->GenUUID($String);
	$TableName = 'operators';
	$sql="SELECT * FROM $TableName WHERE operator_uuid='$operatoruuid'";
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
	$MachineID = array();
	$LoginTime = array();
	$LogoutTime = array();
	$GroupID;
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$UUID[$Counter] = odbc_result($result, 'operator_uuid');
		$Name[$Counter] = odbc_result($result, 'name');
		$MachineID[$Counter] = odbc_result($result, 'machine_id');
		$LoginTime[$Counter] = odbc_result($result, 'login_time');
		$LogoutTime[$Counter] = odbc_result($result, 'logout_time');
		$GroupID[$Counter] = odbc_result($result, 'group_id');
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
		$jsondata['status'] = false;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['message'] = "Login Fail, Please Check Employee Id or Password!!!";		
	}
	
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>