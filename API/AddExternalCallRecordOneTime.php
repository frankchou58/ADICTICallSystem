<?php
	error_reporting(0);
	$jsondata = array();
	$MachineID1 = htmlspecialchars($_GET["MachineID"]);
	$StartTime1 = htmlspecialchars($_GET["StartTime"]);
	$OutVPort1 = htmlspecialchars($_GET["OutVPort"]);
	$OutPhyPort1 = htmlspecialchars($_GET["OutPhyPort"]);
	$TelNo1 = htmlspecialchars($_GET["TelNo"]);
	$ExtNum1 = htmlspecialchars($_GET["ExtNum"]);
	$CallType1 = htmlspecialchars($_GET["CallType"]);
	$EndTime1 = htmlspecialchars($_GET["EndTime"]);
	$ConnectTime1 = htmlspecialchars($_GET["ConnectTime"]);
	$Duration1 = htmlspecialchars($_GET["Duration"]);
	$RingTimes1 = htmlspecialchars($_GET["RingTimes"]);
	$VoiceFileName1 = htmlspecialchars($_GET["FileName"]);
	
	$MachineID2 = htmlspecialchars($_POST["MachineID"]);
	$StartTime2 = htmlspecialchars($_POST["StartTime"]);
	$OutVPort2 = htmlspecialchars($_POST["OutVPort"]);
	$OutPhyPort2 = htmlspecialchars($_POST["OutPhyPort"]);
	$TelNo2 = htmlspecialchars($_POST["TelNo"]);
	$ExtNum2 = htmlspecialchars($_POST["ExtNum"]);
	$CallType2 = htmlspecialchars($_POST["CallType"]);
	$EndTime2 = htmlspecialchars($_POST["EndTime"]);
	$ConnectTime2 = htmlspecialchars($_POST["ConnectTime"]);
	$Duration2  = htmlspecialchars($_POST["Duration"]);
	$RingTimes2  = htmlspecialchars($_POST["RingTimes"]);
	$VoiceFileName2 = htmlspecialchars($_POST["FileName"]);

	if($MachineID1 != NULL || $StartTime1 != NULL || $OutVPort1 != NULL || $OutPhyPort1 != NULL || $TelNo1 != NULL || $ExtNum1 != NULL || 
		$CallType1 != NULL || $EndTime1 != NULL || $ConnectTime1 != NULL || $Duration1 != NULL || $RingTimes1 != NULL || $VoiceFileName1 != NULL
	)
	{
		$MachineID = $MachineID1;
		$StartTime = $StartTime1;
		$OutVPort = $OutVPort1;
		$OutPhyPort = $OutPhyPort1;
		$TelNo = $TelNo1;
		$ExtNum = $ExtNum1;
		$CallType = $CallType1;
		$EndTime = $EndTime1;
		$ConnectTime = $ConnectTime1;
		$Duration = $Duration1;
		$RingTimes = $RingTimes1;
		$VoiceFileName = $VoiceFileName1;
	}
	else if($MachineID2 != NULL || $StartTime2 != NULL || $OutVPort2 != NULL || $OutPhyPort2 != NULL ||	$TelNo2 != NULL || $ExtNum2 != NULL || 
		$CallType2 != NULL || $EndTime2 != NULL || $ConnectTime2 != NULL || $Duration2 != NULL || $RingTimes2 != NULL || $VoiceFileName2 != NULL
	)
	{
		$MachineID = $MachineID2;
		$StartTime = $StartTime2;
		$OutVPort = $OutVPort2;
		$OutPhyPort = $OutPhyPort2;
		$TelNo = $TelNo2;
		$ExtNum = $ExtNum2;
		$CallType = $CallType2;
		$EndTime = $EndTime2;
		$ConnectTime = $ConnectTime2;
		$Duration = $Duration2;
		$RingTimes = $RingTimes2;
		$VoiceFileName = $VoiceFileName2;
	}
	
	if($MachineID == NULL || $StartTime == NULL || $OutVPort == NULL || $OutPhyPort == NULL || $TelNo == NULL || $ExtNum == NULL || 
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
	$sql="SELECT * FROM $TableName WHERE call_start_time='$StartTime' AND machine_id='$MachineID' AND out_vport_no='$OutVPort' AND out_phyport_no='$OutPhyPort'";
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
	$Counter = 0;
	while ($row = odbc_fetch_row($result))
	{
		$Counter++;
	}
	
	if($Counter == 0)
	{
		$sql="INSERT INTO $TableName (call_start_time,machine_id,out_vport_no,out_phyport_no,tel_no,ext_num,call_type,call_connect_time,call_end_time,call_duration_time,voice_record,ring_times) 
		VALUES('$StartTime','$MachineID','$OutVPort','$OutPhyPort','$TelNo','$ExtNum','$CallType','$ConnectTime','$EndTime','$Duration','$VoiceFileName','$RingTimes')";
		//echo $sql;
		$result = odbc_exec($Connected, $sql);
		if(!$result)
		{
			$jsondata['status'] = false;
			$jsondata['message'] = "Database INSERT $table Error: " . odbc_error() . "!!!";
			goto endsql;
		}
		$sql="SELECT * FROM $TableName WHERE call_start_time='$StartTime' AND machine_id='$MachineID' AND out_vport_no='$OutVPort' AND out_phyport_no='$OutPhyPort'";
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
//			$jsondata['numbers'] = "$Counter";
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