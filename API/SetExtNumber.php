<?php
	error_reporting(0);
	$jsondata = array();
	$SubProgramID1 = htmlspecialchars($_GET["MachineID"]);
	$ExtPort1 = htmlspecialchars($_GET["ExtVPort"]);
	$ExtNo1 = htmlspecialchars($_GET["ExtNum"]);
	$SubProgramID2 = htmlspecialchars($_POST["MachineID"]);
	$ExtPort2 = htmlspecialchars($_POST["ExtVPort"]);
	$ExtNo2 = htmlspecialchars($_POST["ExtNum"]);
	if($SubProgramID1 != NULL || $ExtPort1 != NULL || $ExtNo1 != NULL)
	{
		$SubProgramID = $SubProgramID1;
		$ExtPort = $ExtPort1;
		$ExtNo = $ExtNo1;
	}
	else if($SubProgramID2 != NULL || $ExtPort2 != NULL || $ExtNo2 != NULL)
	{
		$SubProgramID = $SubProgramID2;
		$ExtPort = $ExtPort2;
		$ExtNo = $ExtNo2;
	}
	
	if($SubProgramID == NULL || $ExtPort == NULL || $ExtNo == NULL)
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
	if(!$result)
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "Database SELECT $table Error: " . odbc_error() . "!!!";
		goto endsql;
	}
	$Counter = 0;
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$LineNo[$Counter] = odbc_result($result, 'line_no');
		$MachinePhyPort[$Counter] = odbc_result($result, $FieldPhyPort);
		$Counter++;
	}
	
	if($Counter == 0)
	{
		$Ret = $tool->SetExtPortNumber($Connected, $SubProgramID, $ExtPort, $ExtNo);
	  list($endtime, $sec) = explode(" ", microtime());
		if($Ret != 'ok')
		{
			$jsondata['status'] = false;		
			$jsondata['message'] = $Ret;		
			goto endsql;
		}
		else
		{
			$jsondata['status'] = true;		
			$jsondata['esptime'] = $endtime - $starttime;
			$jsondata['message'] = "Success Set Ext Physical Port:$ExtPort to Extension No:$ExtNo into MachineID:$SubProgramID";		
		}
	}
	else
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = "This Extnstion Number[$ExtNo] already existed!!!";		
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>