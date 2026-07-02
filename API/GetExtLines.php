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

 	$TableName = 'extline';
	$sql="SELECT * FROM $TableName ORDER BY 'port_no'";
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
	$ExtPortNo = array();
	$ExtNo = array();
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$ExtPortNo[$Counter] = odbc_result($result, 'port_no');
		$ExtNo[$Counter] = odbc_result($result, 'ext_no');
		$MachineID[$Counter] = odbc_result($result, 'sub_program_id');
		$Counter++;
	}
	$jsondata['status'] = true;		
	$jsondata['numbers'] = "$Counter";
	$jsondata['esptime'] = $endtime - $starttime;

	if($Counter > 0)
	{	
		$jsondata['fields'] = array('ID','Ext Virtual Port No','Extension Number','MachineID');
		for($x = 0; $x < $Counter; $x++)
		{
			$jsondata['data'][$x] = array($ID[$x], $ExtPortNo[$x], $ExtNo[$x], $MachineID[$x]);
		}
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>