<?php
	error_reporting(0);
	$jsondata = array();
	$ID1 = htmlspecialchars($_GET["ID"]);
	$ExtNum1 = htmlspecialchars($_GET["ExtNum"]);
	$ID2 = htmlspecialchars($_POST["ID"]);
	$ExtNum2 = htmlspecialchars($_POST["ExtNum"]);

	if($ID1 != NULL || $ExtNum1 != NULL)
	{
		$ID = $ID1;
		$ExtNum = $ExtNum1;
	}
	else if($ID2 != NULL || $ExtNum2 != NULL)
	{
		$ID = $ID2;
		$ExtNum = $ExtNum2;
	}
	
	if($ID == NULL || $ExtNum == NULL)
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
	$sql="UPDATE $TableName SET ext_num='$ExtNum' WHERE ID='$ID'";
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
	$jsondata['message'] = "Successful update Extension Number=$ExtNum into Database";
		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>