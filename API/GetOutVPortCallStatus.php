<?php
	error_reporting(0);
	$jsondata = array();
	$MachineID1 = htmlspecialchars($_GET["MachineID"]);
	$MachineID2 = htmlspecialchars($_POST["MachineID"]);
	if($MachineID1 != NULL)
	{
		$MachineID = $MachineID1;
	}
	else if($MachineID2 != NULL)
	{
		$MachineID = $MachineID2;
	}
	
	if($MachineID == NULL)
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

 	$TableName = 'outline';
	$sql="SELECT * FROM $TableName WHERE machine_id='$MachineID'";
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
	$OutVPort = array();
	$Status = array();
	$Type = array();
	$TelNo = array();
	$ExtNum = array();
	$PhyPortNo = array();
	while ($row = odbc_fetch_row($result))
	{
		$OutVPort[$Counter] = odbc_result($result, 'line_no');
		$Status[$Counter] = odbc_result($result, 'line_status');
		$Type[$Counter] = odbc_result($result, 'call_type');
		$TelNo[$Counter] = odbc_result($result, 'tel_no');
		$ExtNum[$Counter] = odbc_result($result, 'ext_no');
		$PhyPortNo[$Counter] = odbc_result($result, 'phy_port');
		$Counter++;
	}
	$jsondata['status'] = true;		
	$jsondata['numbers'] = "$Counter";
	$jsondata['esptime'] = $endtime - $starttime;
	if($Counter > 0)
	{	
		$jsondata['fields'] = array('OutVPort','Call Status','Call Type','Tel No', 'Ext Num', 'Phy Port No');
		for($x = 0; $x < $Counter; $x++)
		{
			$jsondata['data'][$x] = array($OutVPort[$x], $Status[$x], $Type[$x], $TelNo[$x], $ExtNum[$x], $PhyPortNo[$x]);
		}
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>