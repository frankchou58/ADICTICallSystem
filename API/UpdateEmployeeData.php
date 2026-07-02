<?php
	error_reporting(0);
	$jsondata = array();
	$EmpID1 = htmlspecialchars($_GET["EmpID"]);
	$FieldName1 = $_GET["FieldName"];
	$FieldContents1 = $_GET["FieldContents"];

	$EmpID2 = htmlspecialchars($_POST["EmpID"]);
	$FieldName2 = $_POST["FieldName"];
	$FieldContents2 = $_POST["FieldContents"];

	if($EmpID1 != NULL || $FieldName1 != NULL || $FieldContents1 != NULL)
	{
		$EmpID = $EmpID1;
		$FieldName = $FieldName1;
		$FieldContents = $FieldContents1;
	}
	else if($EmpID2 != NULL || $FieldName2 != NULL || $FieldContents2 != NULL)
	{
		$EmpID = $EmpID2;
		$FieldName = $FieldName2;
		$FieldContents = $FieldContents2;
	}
	
	if($EmpID == NULL || $FieldName == NULL || $FieldContents == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!! Must Have EmpID & FieldName & FieldContents';		
		goto end;
	}

    $FieldNameArray = json_decode($FieldName, true);  // decode as associative hash
    $FieldContentsArray = json_decode($FieldContents, true);  // decode as associative hash
	$Counter = count($FieldNameArray);
	//echo "Numbers = $Counter" . PHP_EOL;
	$SqlData;
	/* Check Field Name is not include 'EmpID' and 'EmpIdentityID' */
	for($i = 0; $i < $Counter; $i++)
	{
		if($FieldNameArray[$i] == 'EmpID')
		{
			$jsondata['status'] = false;		
			$jsondata['message'] = 'FieldName could not EmpID Or EmpIdentityID!!!!!';		
			goto end;
		}
		$Name = $FieldNameArray[$i];
		$Contents = $FieldContentsArray[$Name];
		$FieldContentsStr = iconv("UTF-8", "big5", $Contents);
		$SqlData = $SqlData . "$Name='$FieldContentsStr',";
    }

	require_once 'tools.php';
	$tool = new tools();
	
	$Connected = $tool->SqlConnectCPF144();
	if($Connected < 0)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'SQL Connection Fail';		
		goto endsql;
	}

	list($starttime, $sec) = explode(" ", microtime());
	$Current = date('Y-m-d H:i:s');
	$TableName = 'Emp';
	$SqlData = $SqlData . "EmpLastEdit='$Current'";
	$sql="UPDATE $TableName SET $SqlData WHERE EmpID='$EmpID'";
	//echo "Sql = $sql\n";
	$result = odbc_exec($Connected, $sql);		
	if(!$result)
	{
		if(odbc_error() == S0022)
		{
			$jsondata['status'] = false;
			$jsondata['message'] = 'Please Check Field Name And Try Again!!!!!';		
		}
		else
		{
			$jsondata['status'] = false;
			$jsondata['message'] = odbc_error();		
		}
	}
	else
	{
  		list($endtime, $sec) = explode(" ", microtime());
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>
