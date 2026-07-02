<?php
	error_reporting(0);
	$jsondata = array();
	$OutPort1 = htmlspecialchars($_GET["OutVPort"]);
	$CallStatus1 = htmlspecialchars($_GET["CallStatus"]);
	$CallType1 = htmlspecialchars($_GET["CallType"]);
	$PhyPort1 = htmlspecialchars($_GET["PhyPort"]);
	$TelNo1 = htmlspecialchars($_GET["TelNo"]);
	$ExtNum1 = htmlspecialchars($_GET["ExtNum"]);	
	$OutPort2 = htmlspecialchars($_POST["OutVPort"]);
	$CallStatus2 = htmlspecialchars($_POST["CallStatus"]);
	$CallType2 = htmlspecialchars($_POST["CallType"]);
	$PhyPort2 = htmlspecialchars($_POST["PhyPort"]);
	$TelNo2 = htmlspecialchars($_POST["TelNo"]);
	$ExtNum2 = htmlspecialchars($_POST["ExtNum"]);	
	if($OutPort1 != NULL || $CallStatus1 != NULL || $CallType1 != NULL || $PhyPort1 != NULL || $TelNo1 != NULL || $ExtNum1 != NULL)
	{
		$OutPort = $OutPort1;
		$CallStatus = $CallStatus1;
		$CallType = $CallType1;
		$PhyPort = $PhyPort1;
		$TelNo = $TelNo1;
		$ExtNum = $ExtNum1;	
	}
	else if($OutPort2 != NULL || $CallStatus2 != NULL || $CallType2 != NULL || $PhyPort2 != NULL || $TelNo2 != NULL || $ExtNum2 != NULL)
	{
		$OutPort = $OutPort2;
		$CallStatus = $CallStatus2;
		$CallType = $CallType2;
		$PhyPort = $PhyPort2;
		$TelNo = $TelNo2;
		$ExtNum = $ExtNum2;	
	}
	
	if($OutPort == NULL)
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
	$sql="UPDATE $TableName SET line_status='$CallStatus',call_type='$CallType',tel_no='$TelNo',ext_no='$ExtNum',phy_port='$PhyPort' WHERE line_no='$OutPort'";
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
	$jsondata['message'] = "Success Set Out Virtual Port:$OutPort to InUsed";		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>