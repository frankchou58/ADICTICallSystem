<?php
	error_reporting(0);
	$jsondata = array();
	$SubProgramID1 = htmlspecialchars($_GET["MachineID"]);
	$ExtNo1 = htmlspecialchars($_GET["ExtNum"]);
	$SubProgramID2 = htmlspecialchars($_POST["MachineID"]);
	$ExtNo2 = htmlspecialchars($_POST["ExtNum"]);
	if($ExtNo1 != NULL || $SubProgramID1 != NULL)
	{
		$SubProgramID = $SubProgramID1;
		$ExtNo = $ExtNo1;
	}
	else if($ExtNo2 != NULL || $SubProgramID2 != NULL)
	{
		$SubProgramID = $SubProgramID2;
		$ExtNo = $ExtNo2;
	}
	
	if($SubProgramID == NULL || $ExtNo == NULL)
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
	$sql="SELECT * FROM $TableName WHERE ext_no='$ExtNo' AND sub_program_id='$SubProgramID'";
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
	$ExtNum = array();
	$ExtVPort = array();
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$ExtNum[$Counter] = odbc_result($result, 'ext_no');
		$ExtVPort[$Counter] = odbc_result($result, 'port_no');
		$Counter++;
	}
	if($Counter == 1)
	{
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['numbers'] = "$Counter";
		$jsondata['fields'] = array('ID','Extension Number','Extension Virtual Port');
		for($x = 0; $x < $Counter; $x++)
		{
			$jsondata['data'][$x] = array($ID[$x], $ExtNum[$x], $ExtVPort[$x]);
		}
	}
	else
	{
		$jsondata['status'] = false;
		$jsondata['numbers'] = 0;
		$jsondata['message'] = "Extension Number:$ExtNo Not Found";		
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>