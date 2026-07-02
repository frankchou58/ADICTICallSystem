<?php
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
	error_reporting(0);
	$jsondata = array();
	$StartTime1 = htmlspecialchars($_GET["StartTime"]);
	$EndTime1 = htmlspecialchars($_GET["EndTime"]);
	$StartTime2 = htmlspecialchars($_POST["StartTime"]);
	$EndTime2 = htmlspecialchars($_POST["EndTime"]);
	if($StartTime1 != NULL || $EndTime1 != NULL)
	{
		$StartTime = $StartTime1;
		$EndTime = $EndTime1;
	}
	else if($StartTime2 != NULL || $EndTime2 != NULL)
	{
		$StartTime = $StartTime2;
		$EndTime = $EndTime2;
	}
	
	if($StartTime == NULL || $EndTime == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!!';		
		goto end;
	}
	
	if($StartTime > $EndTime)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = "$StartTime > $EndTime, please check StartTime and EndTime!!!";
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

 	$TableName = 'record';
	$sql="SELECT * FROM $TableName WHERE call_start_time>='$StartTime' AND call_start_time<='$EndTime' ORDER BY call_start_time ASC";
	//echo $sql;
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
	$ExtNum = array();
	$CallType = array();
	$CallStartTime = array();
	$CallConnectTime = array();
	$CallEndTime = array();
	$CallDuration = array();
	$MachineID = array();
	$TelNo = array();
	$OutPhyPort = array();
	$RingTimes = array();
	$InterFromExtNum = array();
	$InterToExtNum = array();
	$OperatorUUID = array();
	$VoiceFileName = array();
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$OutVPort[$Counter] = odbc_result($result, 'out_vport_no');
		$ExtNum[$Counter] = odbc_result($result, 'ext_num');
		$CallType[$Counter] = odbc_result($result, 'call_type');
		$CallStartTime[$Counter] = odbc_result($result, 'call_start_time');
		$CallConnectTime[$Counter] = odbc_result($result, 'call_connect_time');
		$CallEndTime[$Counter] = odbc_result($result, 'call_end_time');
		$CallDuration[$Counter] = odbc_result($result, 'call_duration_time');
		$MachineID[$Counter] = odbc_result($result, 'machine_id');
		$TelNo[$Counter] = odbc_result($result, 'tel_no');
		$OutPhyPort[$Counter] = odbc_result($result, 'out_phyport_no');
		$RingTimes[$Counter] = odbc_result($result, 'ring_times');
		$InterFromExtNum[$Counter] = odbc_result($result, 'internal_call_from_ext_no');
		$InterToExtNum[$Counter] = odbc_result($result, 'internal_call_to_ext_no');
		$OperatorUUID[$Counter] = odbc_result($result, 'operator_uuid');
		$VoiceFileName[$Counter] = odbc_result($result, 'voice_record');
		
		$Counter++;
	}

	$jsondata['status'] = true;		
	$jsondata['esptime'] = $endtime - $starttime;
	$jsondata['numbers'] = "$Counter";
	$jsondata['fields'] = array('ID','Out Virture Port','Extension Number','Call Type','Call Start Time','Call Connection Time','Call End Time','Duration','Machine ID','Tel No','Out Physical Port','Ring Times','Internal Call From Extension Number','Internal Call To Extension Number','Operator UUID','Voice File Name');
	for($x = 0; $x < $Counter; $x++)
	{
		$jsondata['data'][$x] = array($ID[$x], $OutVPort[$x], $ExtNum[$x], $CallType[$x], $CallStartTime[$x], $CallConnectTime[$x], $CallEndTime[$x], $CallDuration[$x], $MachineID[$x], $TelNo[$x], $OutPhyPort[$x], $RingTimes[$x], $InterFromExtNum[$x], $InterToExtNum[$x], $OperatorUUID[$x], $VoiceFileName[$x]);
	}

endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
?>