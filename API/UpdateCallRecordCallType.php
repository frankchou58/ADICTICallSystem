<?php
	error_reporting(0);
	$jsondata = array();
	$ID1 = htmlspecialchars($_GET["ID"]);
	$Type1 = htmlspecialchars($_GET["Type"]);
	$ID2 = htmlspecialchars($_POST["ID"]);
	$Type2 = htmlspecialchars($_POST["Type"]);

	if($ID1 != NULL || $Type1 != NULL)
	{
		$ID = $ID1;
		$Type = $Type1;
	}
	else if($ID2 != NULL || $Type2 != NULL)
	{
		$ID = $ID2;
		$Type = $Type2;
	}
	
	if($ID == NULL || $Type == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!!';		
		goto end;
	}
	
	if($Type < 0 || $Type > 2)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Call Type 0~2 [0:Call In | 1: Call Out | 2: Internal Call]!!!';		
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
	$sql="UPDATE $TableName SET call_type='$Type' WHERE ID='$ID'";
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
	$jsondata['message'] = "Successful update Call Type=$Type [0:Call In | 1: Call Out | 2: Internal Call] into Database";
		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>