<?php
	error_reporting(0);
	$jsondata = array();
	$MachineID1 = htmlspecialchars($_GET["MachineID"]);
	$StartTime1 = htmlspecialchars($_GET["StartTime"]);
	$FromExtNum1 = htmlspecialchars($_GET["FromExtNum"]);
	$ToExtNum1 = htmlspecialchars($_GET["ToExtNum"]);
	$CallType1 = htmlspecialchars($_GET["CallType"]);
	$EndTime1 = htmlspecialchars($_GET["EndTime"]);
	$ConnectTime1 = htmlspecialchars($_GET["ConnectTime"]);
	$Duration1 = htmlspecialchars($_GET["Duration"]);
	$RingTimes1 = htmlspecialchars($_GET["RingTimes"]);
	$VoiceFileName1 = htmlspecialchars($_GET["FileName"]);

	$MachineID2 = htmlspecialchars($_POST["MachineID"]);
	$StartTime2 = htmlspecialchars($_POST["StartTime"]);
	$FromExtNum2 = htmlspecialchars($_POST["FromExtNum"]);
	$ToExtNum2 = htmlspecialchars($_POST["ToExtNum"]);
	$CallType2 = htmlspecialchars($_POST["CallType"]);
	$EndTime2 = htmlspecialchars($_POST["EndTime"]);
	$ConnectTime2 = htmlspecialchars($_POST["ConnectTime"]);
	$Duration2  = htmlspecialchars($_POST["Duration"]);
	$RingTimes2  = htmlspecialchars($_POST["RingTimes"]);
	$VoiceFileName2 = htmlspecialchars($_POST["FileName"]);

	if($MachineID1 != NULL || $StartTime1 != NULL || $FromExtNum1 != NULL || $ToExtNum1 != NULL ||
		$CallType1 != NULL || $EndTime1 != NULL || $ConnectTime1 != NULL || $Duration1 != NULL || $RingTimes1 != NULL || $VoiceFileName1 != NULL
	)
	{
		$MachineID = $MachineID1;
		$StartTime = $StartTime1;
		$FromExtNum = $FromExtNum1;
		$ToExtNum = $ToExtNum1;
		$CallType = $CallType1;
		$EndTime = $EndTime1;
		$ConnectTime = $ConnectTime1;
		$Duration = $Duration1;
		$RingTimes = $RingTimes1;
		$VoiceFileName = $VoiceFileName1;
	}
	else if($MachineID2 != NULL || $StartTime2 != NULL || $FromExtNum2 != NULL || $ToExtNum2 != NULL ||
		$CallType2 != NULL || $EndTime2 != NULL || $ConnectTime2 != NULL || $Duration2 != NULL || $RingTimes2 != NULL || $VoiceFileName2 != NULL
	)
	{
		$MachineID = $MachineID2;
		$StartTime = $StartTime2;
		$FromExtNum = $FromExtNum2;
		$ToExtNum = $ToExtNum2;
		$CallType = $CallType2;
		$EndTime = $EndTime2;
		$ConnectTime = $ConnectTime2;
		$Duration = $Duration2;
		$RingTimes = $RingTimes2;
		$VoiceFileName = $VoiceFileName2;
	}
	
	if($MachineID == NULL || $StartTime == NULL || $FromExtNum == NULL || $ToExtNum == NULL ||
	  $CallType == NULL || $EndTime == NULL || $StartTime == NULL || $ConnectTime == NULL || $Duration == NULL || $RingTimes == NULL
	)	
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!!';		
		goto end;
	}
	
	if($MachineID < 1 || $MachineID > 10)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'MachineID sholud be 1 ~ 10!!!';		
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
	$sql="SELECT * FROM $TableName WHERE call_start_time='$StartTime' AND machine_id='$MachineID' AND internal_call_from_ext_no='$FromExtNum' AND internal_call_to_ext_no='$ToExtNum'";
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
	$Counter = 0;
	while ($row = odbc_fetch_row($result))
	{
		$Counter++;
	}
	
	//echo "Counter=$Counter";
	if($Counter == 0)
	{
		$sql="INSERT INTO $TableName (call_start_time,machine_id,internal_call_from_ext_no,internal_call_to_ext_no,call_type,call_connect_time,call_end_time,call_duration_time,voice_record,ring_times) 
		VALUES('$StartTime','$MachineID','$FromExtNum','$ToExtNum','$CallType','$ConnectTime','$EndTime','$Duration','$VoiceFileName','$RingTimes')";
		//echo $sql;
		$result = odbc_exec($Connected, $sql);
		if(!$result)
		{
			$jsondata['status'] = false;
			$jsondata['message'] = "Database INSERT $table Error: " . odbc_error() . "!!!";
			goto endsql;
		}
		$sql="SELECT * FROM $TableName WHERE call_start_time='$StartTime' AND machine_id='$MachineID' AND internal_call_from_ext_no='$FromExtNum' AND internal_call_to_ext_no='$ToExtNum'";
		$result = odbc_exec($Connected, $sql);
	  list($endtime, $sec) = explode(" ", microtime());
		$Counter = 0;
		$MachineID = array();
		$StartTime = array();
		$ID = array();
		while ($row = odbc_fetch_row($result))
		{
			$ID[$Counter] = odbc_result($result,'ID');
			$MachineID[$Counter] = odbc_result($result,'machine_id');
			$StartTime[$Counter] = odbc_result($result,'call_start_time');
			$Counter++;
		}
		//echo "Counter=$Counter";
		//if($Counter == 1)
		{
			$jsondata['status'] = true;		
			$jsondata['numbers'] = "$Counter";
			$jsondata['esptime'] = $endtime - $starttime;
			$jsondata['fields'] = array('ID','MachineID');
			for($x = 0; $x < $Counter; $x++)
			{
				$jsondata['data'][$x] = array($ID[$x], $MachineID[$x]);
			}
		}
	}
	else
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "This Data already Existed!!!";
	}
	
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>