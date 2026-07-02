<?php
	error_reporting(0);
	$jsondata = array();
	$ID1 = htmlspecialchars($_GET["ID"]);
	$Timestamp1 = htmlspecialchars($_GET["Timestamp"]);
	$ID2 = htmlspecialchars($_POST["ID"]);
	$Timestamp2 = htmlspecialchars($_POST["Timestamp"]);

	if($ID1 != NULL || $Timestamp1 != NULL)
	{
		$ID = $ID1;
		$Timestamp = $Timestamp1;
	}
	else if($ID2 != NULL || $Timestamp2 != NULL)
	{
		$ID = $ID2;
		$Timestamp = $Timestamp2;
	}
	
	if($ID == NULL || $Timestamp == NULL)
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

	$TableName = 'record';
	$sql="UPDATE $TableName SET call_connect_time='$Timestamp' WHERE ID='$ID'";
	//echo $sql;
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
  list($endtime, $sec) = explode(" ", microtime());
	if(!$result)
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "Database INSERT $table Error: " . odbc_error() . "!!!";
		goto endsql;
	}
	$jsondata['status'] = true;
	$jsondata['esptime'] = $endtime - $starttime;
	$jsondata['message'] = "Successful update Talking Time=$Timestamp into Database";
		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>
