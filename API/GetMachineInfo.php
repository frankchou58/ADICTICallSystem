<?php
	error_reporting(0);
	$jsondata = array();
	$MachineType1 = htmlspecialchars($_GET["MachineType"]);
	$MachineID1 = htmlspecialchars($_GET["MachineID"]);
	$MachineType2 = htmlspecialchars($_POST["MachineType"]);
	$MachineID2 = htmlspecialchars($_POST["MachineID"]);
	if($MachineID1 != NULL || $MachineType1 != NULL)
	{
		$MachineType = $MachineType1;
		$MachineID = $MachineID1;
	}
	else if($MachineID2 != NULL)
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
	$sql="SELECT * FROM $TableName WHERE machine_id='$MachineID' AND machine_type='$MachineType'";
	//echo "sql: $sql";
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
	$Alias = array();
	$OutPorts = array();
	$ExtPorts = array();
	while ($row = odbc_fetch_row($result))
	{
		$Alias[$Counter] = odbc_result($result, 'alias');
		$OutPorts[$Counter] = odbc_result($result, 'out_port_num');
		$ExtPorts[$Counter] = odbc_result($result, 'ext_port_num');
		$Counter++;
	}
	if($Counter == 1)
	{
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['fields'] = array('Alias','Out Ports','Ext Pots');
		for($x = 0; $x < $Counter; $x++)
		{
			$StrAlias = iconv("big5","UTF-8",$Alias[$x]);	
			$jsondata['data'][$x] = array($StrAlias, $OutPorts[$x], $ExtPorts[$x]);
		}
	}
	else
	{
		$jsondata['status'] = false;
		$jsondata['numbers'] = 0;
		$jsondata['message'] = "MachineType:MachineID $MachineType:$MachineID Not Found";		
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>