<?php
	error_reporting(0);
	$jsondata = array();
	$UUID1 = htmlspecialchars($_GET["Key"]);
	$MachineID1 = htmlspecialchars($_GET["MachineId"]);
	$UUID2 = htmlspecialchars($_POST["Key"]);
	$MachineID2 = htmlspecialchars($_POST["MachineId"]);
	if($UUID1 != NULL || $MachineID1 != NULL)
	{
		$UUID = $UUID1;
		$MachineID = $MachineID1;
	}
	else if($UUID2 != NULL || $MachineID2 != NULL)
	{
		$UUID = $UUID2;
		$MachineID = $MachineID2;
	}
	
	if($UUID == NULL || $MachineID == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!!';		
		goto end;
	}
	
	if($MachineID <= 0 || $MachineID > 10)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please input correct MachineID [1 ~ 10]!!!';		
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
	$sql="SELECT * FROM $TableName WHERE operator_uuid='$UUID'";
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
	if(!$result)
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "Database SELECT $table Error: " . odbc_error() . "!!!";
		goto endsql;
	}
	$Counter = 0;
	$ID = array();
	$MachineBits = array();
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$MachineBits[$Counter] = odbc_result($result, 'machine_id');
		$Counter++;
	}
	if($Counter == 1)
	{	
		$BitMask = 1 << ($MachineID - 1);
		$Bits = $MachineBits[0] & ~$BitMask;
		$sql="UPDATE $TableName SET machine_id='$Bits' WHERE operator_uuid='$UUID'";
		$result = odbc_exec($Connected, $sql);
	  list($endtime, $sec) = explode(" ", microtime());
		if(!$result)
		{
			$jsondata['status'] = false;
			$jsondata['message'] = "Database SELECT $table Error: " . odbc_error() . "!!!";
			goto endsql;
		}
		$Bin = decbin($Bits);	
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['message'] = "Success Set Machine Bitmask[$Bin] to [$UUID]!!!";		
	}
	else
	{
			$jsondata['status'] = false;
			$jsondata['message'] = "Duplicate this Operator!!!";		
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>