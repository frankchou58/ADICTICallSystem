<?php
	error_reporting(0);
	$jsondata = array();
	$MachineType1 = htmlspecialchars($_GET["MachineType"]);
	$MachineID1 = htmlspecialchars($_GET["MachineID"]);
	$Alias1 = htmlspecialchars($_GET["Alias"]);
	$MachineType2 = htmlspecialchars($_POST["MachineType"]);
	$MachineID2 = htmlspecialchars($_POST["MachineID"]);
	$Alias2 = htmlspecialchars($_POST["Alias"]);
	if($MachineType1 != NULL || $MachineID1 != NULL || $Alias1 != NULL)
	{
		$MachineType = $MachineType1;
		$MachineID = $MachineID1;
		$Alias = $Alias1;
	}
	else if($MachineType2 != NULL || $MachineID2 != NULL || $Alias2 != NULL)
	{
		$MachineType = $MachineType2;
		$MachineID = $ExtPort2;
		$Alias = $Alias2;
	}
	
	if($MachineType == NULL || $MachineID == NULL)
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
	
	
	$StrAlias = iconv("UTF-8", "big5", $Alias);	
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
	$sql="UPDATE $TableName SET alias='$StrAlias' WHERE machine_type='$MachineType' AND machine_id='$MachineID'";
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
	$jsondata['message'] = "Success Set Alias($Alias) in MachineType:MachineID($StrMachineType:$MachineID)";		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json');		
	header('Access-Control-Allow-Origin: *');		
?>