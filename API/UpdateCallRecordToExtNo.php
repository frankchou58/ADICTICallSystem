<?php
	error_reporting(0);
	$jsondata = array();
	$ID1 = htmlspecialchars($_GET["ID"]);
	$ToExt1 = htmlspecialchars($_GET["ToExt"]);
	$ID2 = htmlspecialchars($_POST["ID"]);
	$ToExt2 = htmlspecialchars($_POST["ToExt"]);

	if($ID1 != NULL || $ToExt1 != NULL)
	{
		$ID = $ID1;
		$ToExt = $ToExt1;
	}
	else if($ID2 != NULL || $ToExt2 != NULL)
	{
		$ID = $ID2;
		$ToExt = $ToExt2;
	}
	
	if($ID == NULL || $ToExt == NULL)
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
	$sql="UPDATE $TableName SET ext_num='$ToExt' WHERE ID='$ID'";
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
	$jsondata['message'] = "Successful update Internal Call To Extension Number=$ToExt into Database";
		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>