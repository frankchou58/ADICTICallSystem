<?php
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
	//error_reporting(0);
	$jsondata = array();
	require_once 'tools.php';
	$tool = new tools();
	
	$Connected = $tool->SqlConnect();
	if($Connected < 0)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = $Connected;		
		goto end;
	}

	$TableName = 'operators';
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
	$ID = array();
	$UUID = array();
	$EmployeeId = array();
	$Name = array();
	$MachineID = array();
	$LoginTime = array();
	$LogoutTime = array();
	$Level = array();
	$ExtNum = array();
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
		$ExtNum[$Counter] = odbc_result($result, 'ext_num'); 
		$Counter++;
	}
	if($Counter > 0)
	{
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['numbers'] = $Counter;
		$jsondata['fields'] = array('ID','Employee ID','UUID','Name','Machine Bitmask','Login Timestamp','Logout Timestamp','Level','Ext Number');
		for($x = 0; $x < $Counter; $x++)
		{
			$StrName = iconv("big5","UTF-8",$Name[$x]);	
			$jsondata['data'][$x] = array($ID[$x], $EmployeeId[$x], $UUID[$x], $StrName, $MachineID[$x], $LoginTime[$x], $LogoutTime[$x], $Level[$x], $ExtNum[$x]);
		}
	}
	else
	{
		$jsondata['status'] = false;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['message'] = "Not Found This Employee[$UUID], Please Check Employee Id or Password!!!";		
	}
	
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
?>