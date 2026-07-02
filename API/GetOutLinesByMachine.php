<?php
	error_reporting(0);
	$jsondata = array();
	$MachineType1 = strtolower(htmlspecialchars($_GET["MachineType"]));
	$MachineID1 = htmlspecialchars($_GET["MachineID"]);
	$MachineType2 = strtolower(htmlspecialchars($_POST["MachineType"]));
	$MachineID2 = htmlspecialchars($_POST["MachineID"]);
	if($MachineType1 != NULL || $MachineID1 != NULL)
	{
		$MachineType = $MachineType1;
		$MachineID = $MachineID1;
	}
	else if($MachineType2 != NULL || $MachineID2 != NULL)
	{
		$MachineType = $MachineType2;
		$MachineID = $MachineID2;
	}
	
	if($MachineType == NULL || $MachineID == NULL)
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
		$jsondata['message'] = 'SQL Connection Fail';		
		goto endsql;
	}

 	$TableName = 'outline';
	$sql="SELECT * FROM $TableName WHERE machine_id='$MachineID' AND line_in_used='1' ORDER BY 'line_no' ASC";
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
	$LineNo = array();
	$MachinePhyPort = array();
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$LineNo[$Counter] = odbc_result($result, 'line_no');
		$MachinePhyPort[$Counter] = odbc_result($result, $FieldPhyPort);
		$Counter++;
	}
	$jsondata['status'] = true;		
	$jsondata['esptime'] = $endtime - $starttime;
	$jsondata['numbers'] = "$Counter";
	if($Counter > 0)
	{	
		$jsondata['fields'] = array('ID','Virtual Port No','Machine Physical Port');
		for($x = 0; $x < $Counter; $x++)
		{
			$jsondata['data'][$x] = array($ID[$x], $LineNo[$x], $MachinePhyPort[$x]);
		}
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>
