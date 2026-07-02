<?php
	error_reporting(0);
	$jsondata = array();
	$MachineType1 = strtolower(htmlspecialchars($_GET["MachineType"]));
	$MachineID1 = htmlspecialchars($_GET["MachineID"]);
	$PhyPort1 = htmlspecialchars($_GET["PhyPort"]);
	$MachineType2 = strtolower(htmlspecialchars($_POST["MachineType"]));
	$MachineID2 = htmlspecialchars($_POST["MachineID"]);
	$PhyPort2 = htmlspecialchars($_POST["PhyPort"]);
	if($MachineType1 != NULL || $PhyPort1 != NULL || $MachineID1 != NULL)
	{
		$MachineType = $MachineType1;
		$MachineID = $MachineID1;
		$PhyPort = $PhyPort1;
	}
	else if($MachineType2 != NULL || $PhyPort2 != NULL || $MachineID2 != NULL)
	{
		$MachineType = $MachineType2;
		$MachineID = $MachineID2;
		$PhyPort = $PhyPort2;
	}
	
	if($MachineType == NULL || $MachineID == NULL || $PhyPort == NULL)
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

 	$TableName = 'outline';
	$sql="SELECT * FROM $TableName WHERE $FieldPhyPort='$PhyPort' AND machine_id='$MachineID'";
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
	$OutVPort = array();
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$OutVPort[$Counter] = odbc_result($result, 'line_no');
		$Counter++;
	}
	if($Counter == 1)
	{
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['numbers'] = "$Counter";
		$jsondata['fields'] = array('ID','Out Virtual Port');
		for($x = 0; $x < $Counter; $x++)
		{
			$jsondata['data'][$x] = array($ID[$x], $OutVPort[$x]);
		}
	}
	else
	{
		$jsondata['status'] = false;
		$jsondata['numbers'] = 0;
		$jsondata['message'] = "ExtNo:$ExtNo Not Found";		
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>