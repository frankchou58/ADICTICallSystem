<?php
	error_reporting(0);
	$jsondata = array();
	$ID1 = htmlspecialchars($_GET["ID"]);
	$TelNo1 = htmlspecialchars($_GET["TelNo"]);
	$ID2 = htmlspecialchars($_POST["ID"]);
	$TelNo2 = htmlspecialchars($_POST["TelNo"]);

	if($ID1 != NULL || $TelNo1 != NULL)
	{
		$ID = $ID1;
		$TelNo = $TelNo1;
	}
	else if($ID2 != NULL || $TelNo2 != NULL)
	{
		$ID = $ID2;
		$TelNo = $TelNo2;
	}
	
	if($ID == NULL || $TelNo == NULL)
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
	$sql="UPDATE $TableName SET tel_no='$TelNo' WHERE ID='$ID'";
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
	$jsondata['message'] = "Successful update TelNo=$TelNo into Database";
		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>