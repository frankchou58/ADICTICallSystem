<?php
	error_reporting(0);
	$jsondata = array();
	$ID1 = htmlspecialchars($_GET["ID"]);
	$FromExt1 = htmlspecialchars($_GET["FromExt"]);
	$ID2 = htmlspecialchars($_POST["ID"]);
	$FromExt2 = htmlspecialchars($_POST["FromExt"]);

	if($ID1 != NULL || $FromExt1 != NULL)
	{
		$ID = $ID1;
		$FromExt = $FromExt1;
	}
	else if($ID2 != NULL || $FromExt2 != NULL)
	{
		$ID = $ID2;
		$FromExt = $FromExt2;
	}
	
	if($ID == NULL || $FromExt == NULL)
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
	$sql="UPDATE $TableName SET ext_num='$FromExt' WHERE ID='$ID'";
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
	$jsondata['message'] = "Successful update Internal Call From Extension Number=$FromExt into Database";
		
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>