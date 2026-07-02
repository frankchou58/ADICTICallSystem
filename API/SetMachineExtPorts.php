<?php
	error_reporting(0);
	$jsondata = array();
	$MachineType1 = htmlspecialchars($_GET["MachineType"]);
	$MachineID1 = htmlspecialchars($_GET["MachineID"]);
	$ExtPorts1 = htmlspecialchars($_GET["ExtPorts"]);
	$MachineType2 = htmlspecialchars($_POST["MachineType"]);
	$MachineID2 = htmlspecialchars($_POST["MachineID"]);
	$ExtPorts2 = htmlspecialchars($_POST["ExtPorts"]);
	if($MachineType1 != NULL || $MachineID1 != NULL || $ExtPorts1 != NULL)
	{
		$MachineType = $MachineType1;
		$MachineID = $MachineID1;
		$ExtPorts = $ExtPorts1;
	}
	else if($MachineType2 != NULL || $MachineID2 != NULL || $ExtPorts2 != NULL)
	{
		$MachineType = $MachineType2;
		$MachineID = $ExtPort2;
		$ExtPorts = $ExtNo2;
	}
	
	if($MachineType == NULL || $MachineID == NULL || $ExtPorts == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!!';		
		goto end;
	}
	
	if($MachineType <= 0 || $MachineType > 3)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please input correct MachineType [1 ~ 3]!!!';		
		goto end;
	}

	if($MachineID <= 0 || $MachineID > 10)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please input correct MachineID [1 ~ 10]!!!';		
		goto end;
	}
	
	if($ExtPorts < 0 || $ExtPorts > 240)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please input correct ExtPorts [1 ~ 240]!!!';		
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

 	$TableName = 'machine';
	$sql="UPDATE $TableName SET ext_port_num='$ExtPorts' WHERE machine_type='$MachineType' AND machine_id='$MachineID'";
  list($starttime, $sec) = explode(" ", microtime());
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
	if($MachineType == 1)
		$StrMachineType = "PBX";
	else if($MachineType == 2)
		$StrMachineType = "CallerID Box";
	else if($MachineType == 3)
		$StrMachineType = "Voice Card";
	$jsondata['message'] = "Success Ext Ports($ExtPorts) in MachineType:MachineID($StrMachineType:$MachineID)";		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>