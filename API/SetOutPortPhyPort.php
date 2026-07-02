<?php
	error_reporting(0);
	$jsondata = array();
	$OutPort1 = htmlspecialchars($_GET["OutVPort"]);
	$MachineType1 = strtolower(htmlspecialchars($_GET["MachineType"]));
	$MachineID1 = htmlspecialchars($_GET["MachineID"]);
	$PhyPort1 = htmlspecialchars($_GET["PhyPort"]);
	$OutPort2 = htmlspecialchars($_POST["OutVPort"]);
	$MachineType2 = strtolower(htmlspecialchars($_POST["MachineType"]));
	$MachineID2 = htmlspecialchars($_POST["MachineID"]);
	$PhyPort2 = htmlspecialchars($_POST["PhyPort"]);
	if($OutPort1 != NULL || $MachineType1 != NULL || $MachineID1 != NULL || $PhyPort1 != NULL)
	{
		$OutPort = $OutPort1;
		$MachineType = $MachineType1;
		$MachineID = $MachineID1;
		$PhyPort = $PhyPort1;
	}
	else if($OutPort2 != NULL || $MachineType2 != NULL || $MachineID2 != NULL || $PhyPort2 != NULL)
	{
		$OutPort = $OutPort2;
		$MachineType = $MachineType2;
		$MachineID = $MachineID2;
		$PhyPort = $PhyPort2;
	}
	
	if($OutPort == NULL || $MachineType == NULL || $MachineID == NULL || $PhyPort == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!!';		
		goto end;
	}
	
	if($MachineType == "1")
	{
		$FieldPhyPort = "phy_port_typeA";
	}
	else if($MachineType == "2")
	{
		$FieldPhyPort = "phy_port_typeB";
	}
	else if($MachineType == "3")
	{
		$FieldPhyPort = "phy_port_typeC";
	}
	else
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please input correct Machine Type 1 ~ 3';		
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

	$TableName = 'outline';
	$sql="UPDATE $TableName SET machine_id='$MachineID',$FieldPhyPort='$PhyPort' WHERE line_no='$OutPort'";
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
	if($MachineType == 1)
		$StrMachineType = "PBX";
	else if($MachineType == 2)
		$StrMachineType = "CallerID Box";
	else if($MachineType == 3)
		$StrMachineType = "Voice Card";
	$jsondata['message'] = "Success Set Out Physical Port[$PhyPort] of MachineType:MachineID[$StrMachineType:$MachineID] to Out Virtual Port[$OutPort]!!!";		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>