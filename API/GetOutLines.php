<?php
	error_reporting(0);
	$jsondata = array();
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
	$sql="SELECT * FROM $TableName ORDER BY 'line_no'";
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
	$InUsed = array();
	$MachineID = array();
	$TypeAMachinePhyPort = array();
	$TypeBMachinePhyPort = array();
	$TypeCMachinePhyPort = array();
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$LineNo[$Counter] = odbc_result($result, 'line_no');
		$InUsed[$Counter] = odbc_result($result, 'line_in_used');
		$MachineID[$Counter] = odbc_result($result, 'machine_id');
		$TypeAMachinePhyPort[$Counter] = odbc_result($result, 'phy_port_typeA');
		$TypeBMachinePhyPort[$Counter] = odbc_result($result, 'phy_port_typeB');
		$TypeCMachinePhyPort[$Counter] = odbc_result($result, 'phy_port_typeC');
		$Counter++;
	}
	$jsondata['status'] = true;		
	$jsondata['numbers'] = "$Counter";
	$jsondata['esptime'] = $endtime - $starttime;
	if($Counter > 0)
	{	
		$jsondata['fields'] = array('ID','LineNo','IsInUsed', 'MachineID', 'MachineTypeAPhyPort', 'MachineTypeBPhyPort', 'MachineTypeCPhyPort');
		for($x = 0; $x < $Counter; $x++)
		{
			$jsondata['data'][$x] = array($ID[$x], $LineNo[$x], $InUsed[$x], $MachineID[$x], $TypeAMachinePhyPort[$x], $TypeBMachinePhyPort[$x], $TypeCMachinePhyPort[$x]);
		}
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>