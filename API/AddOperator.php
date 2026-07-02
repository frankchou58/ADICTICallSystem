<?php
	error_reporting(0);
	$jsondata = array();
	$employeeid1 = htmlspecialchars($_GET["EmployeeId"]);
	$password1 = htmlspecialchars($_GET["Password"]);
	$employeeid2 = htmlspecialchars($_POST["EmployeeId"]);
	$password2 = htmlspecialchars($_POST["Password"]);
	if($employeeid1 != NULL || $password1 != NULL)
	{
		$employeeid = $employeeid1;
		$password = $password1;
	}
	else if($employeeid2 != NULL || $password2 != NULL)
	{
		$employeeid = $employeeid2;
		$password = $password2;
	}
	
	if($employeeid == NULL || $password == NULL)
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

	$String = "$employeeid:$password@adicti.com.tw";   
 	$EmployeeUUID = $tool->GenUUID($String);
  list($starttime, $sec) = explode(" ", microtime());
	$Ret = $tool->AddOperator($Connected, $employeeid, $password, $EmployeeUUID);
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
		$jsondata['UUID'] = $EmployeeUUID;		
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>