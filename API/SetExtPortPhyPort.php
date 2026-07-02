<?php
	error_reporting(0);
	$jsondata = array();
	$ExtVPort1 = htmlspecialchars($_GET["ExtVPort"]);
	$MachineID1 = htmlspecialchars($_GET["MachineID"]);
	$PhyPort1 = htmlspecialchars($_GET["PhyPort"]);
	$ExtVPort2 = htmlspecialchars($_POST["ExtVPort"]);
	$MachineID2 = htmlspecialchars($_POST["MachineID"]);
	$PhyPort2 = htmlspecialchars($_POST["PhyPort"]);
	if($ExtVPort1 != NULL || $MachineID1 != NULL || $PhyPort1 != NULL)
	{
		$ExtVPort = $ExtVPort1;
		$MachineID = $MachineID1;
		$PhyPort = $PhyPort1;
	}
	else if($ExtVPort2 != NULL || $MachineID2 != NULL || $PhyPort2 != NULL)
	{
		$ExtVPort = $ExtVPort2;
		$MachineID = $MachineID2;
		$PhyPort = $PhyPort2;
	}
	
	if($ExtVPort == NULL || $MachineID == NULL || $PhyPort == NULL)
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

	$TableName = 'extline';
	$sql="UPDATE $TableName SET sub_program_id='$MachineID',phy_port='$PhyPort' WHERE port_no='$ExtVPort'";
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
	$jsondata['message'] = "Success Set Ext Physical Port[$PhyPort] of MachineID[$MachineID] to Ext Virtual Port[$ExtVPort]!!!";		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>