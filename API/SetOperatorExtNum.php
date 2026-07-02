<?php
	error_reporting(0);
	$jsondata = array();
	$EmployeeID1 = htmlspecialchars($_GET["ID"]);
	$ExtNum1 = strtolower(htmlspecialchars($_GET["Ext"]));
	$EmployeeID2 = htmlspecialchars($_POST["ID"]);
	$ExtNum12 = strtolower(htmlspecialchars($_POST["Ext"]));
	if($EmployeeID1 != NULL || $ExtNum1 != NULL)
	{
		$EmployeeID = $EmployeeID1;
		$ExtNum = $ExtNum1;
	}
	else if($EmployeeID2 != NULL || $ExtNum2 != NULL)
	{
		$EmployeeID = $EmployeeID2;
		$ExtNum = $ExtNum2;
	}
	
	if($EmployeeID == NULL || $ExtNum== NULL)
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
	$sql="SELECT * FROM $TableName WHERE employee_id='$EmployeeID'";
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
	if(!$result)
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "Database SELECT $table Error: " . odbc_error() . "!!!";
		goto endsql;
	}
	$Counter = 0;
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$UUID[$Counter] = odbc_result($result, 'operator_uuid');
		$EmployeeId[$Counter] = odbc_result($result, 'employee_id');
		$Name[$Counter] = odbc_result($result, 'name');
		$MachineID[$Counter] = odbc_result($result, 'machine_id'); 
		$LoginTime[$Counter] = odbc_result($result, 'login_time'); 
		$LogoutTime[$Counter] = odbc_result($result, 'logout_time'); 
		$Level[$Counter] = odbc_result($result, 'level'); 
		$Counter++;
	}

	if($Counter == 1)
	{
		$sql="UPDATE $TableName SET ext_num='$ExtNum' WHERE employee_id='$EmployeeID'";
		$result = odbc_exec($Connected, $sql);
  	list($endtime, $sec) = explode(" ", microtime());
		if(!$result)
		{
			$jsondata['status'] = false;
			$jsondata['message'] = "Database UPDATE $table Error: " . odbc_error() . "!!!";
			goto endsql;
		}
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['message'] = "Success Set Extension Number[$ExtNum] to Employee[$EmployeeID]!!!";		
	}
	else
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "This Employee is not existed!!!";
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>