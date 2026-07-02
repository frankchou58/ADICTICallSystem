<?php
	error_reporting(0);
	$jsondata = array();
	$QueryCondition1 = $_GET["QueryCondition"];
	$FieldName1 = $_GET["FieldName"];

	$QueryCondition2 = $_POST["QueryCondition"];
	$FieldName2 = $_POST["FieldName"];

	if($QueryCondition1 != NULL || $FieldName1 != NULL)
	{
		$QueryCondition = $QueryCondition1;
		$FieldName = $FieldName1;
	}
	else if($QueryCondition2 != NULL || $FieldName2 != NULL)
	{
		$QueryCondition = $QueryCondition2;
		$FieldName = $FieldName2;
	}
	
	if($QueryCondition == NULL || $FieldName == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!! Must Have QueryCondition={"EmpID","EmpName","EmpTel"} & FieldName as JSON Format';		
		goto end;
	}
	$QueryConditionArray = json_decode($QueryCondition, true);  // decode as associative hash
	$SqlCondition = 'WHERE ';
	$Conditions =  [
        'EmpID',
        'EmpName',
        'EmpTel',
    ];

	$Index = 0;
	for($i = 0; $i < count($Conditions); $i++)
	{
		$Name = $Conditions[$i];
		$Condition = $QueryConditionArray[$Name];
		//echo "Condition = $Condition\n";
		if($Condition != NULL)
		{
			//echo "$Name\n";
			$ConditionStr = iconv("UTF-8", "big5", $Condition);
			if($Index != 0)
				$SqlCondition = $SqlCondition . 'AND ';
			if($Condition == 'ALL')
			{
				$SqlCondition = '';
				break;
			}
			if($Name == 'EmpTel')
			{
				$SqlCondition = $SqlCondition . "EmpTEL1 like '$ConditionStr%' OR EmpTEL2 like '$ConditionStr%' ";
			}
			else
			{
				$SqlCondition = $SqlCondition . "$Name like '$ConditionStr%' ";
			}
			$Index++;
		}
    }
	//echo "$SqlCondition\n";

    $FieldNameArray = json_decode($FieldName, true);  // decode as associative hash
	$FieldNameCounter = count($FieldNameArray);
	//echo "Numbers = $Counter" . PHP_EOL;
	$SqlData;
	/* Check Field Name is not include 'EmpID' and 'EmpIdentityID' */
	for($i = 0; $i < $FieldNameCounter; $i++)
	{
		$Name = $FieldNameArray[$i];
		if($i == 0)
			$SqlData = $SqlData . "$Name";
		else
			$SqlData = $SqlData . ",$Name";
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
	$TableName = 'Emp';
	$sql="SELECT $SqlData FROM $TableName $SqlCondition";
	//echo "Sql = $sql\n";
	$result = $tool->SqlExec($Connected, $sql);		
	if(!$result)
	{
		if($tool->SqlError() == S0022)
		{
			$jsondata['status'] = false;
			$jsondata['message'] = 'Please check FieldName And then Try Again!!!!!';		
		}
		if($tool->SqlError() == 37000)
		{
			$jsondata['status'] = false;
			$jsondata['message'] = 'Please check JSON format as QueryCondition parameter maybe have % character at prefix of digit And then Try Again!!!!!';		
		}
		else
		{
			$jsondata['status'] = false;
			$jsondata['message'] = $tool->SqlError();		
		}
	}
	else
	{
 		list($endtime, $sec) = explode(" ", microtime());
		$Counter = 0;
		$RespData = array();
		$ValueArrayJson = array();
 		while ($row = $tool->SqlFetch($result))
		{
			$ValueArray = array();
			$ValueArrayAll = array();
			for($i = 0; $i < $FieldNameCounter; $i++)
			{
				$Name = $FieldNameArray[$i];
				$Value = $tool->FetchOdbcData($result, $Name);
				$ValueUni = iconv("big5", "UTF-8", $Value);
				$RespData[$i] = $ValueUni;
				$ValueArray = array($RespData[$i]);
				$ValueArrayAll = array_merge($ValueArrayAll, $ValueArray);
			}
			$ValueArrayJson[$Counter] = $ValueArrayAll;
			$Counter++;
		}
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['number'] = $Counter;
		$NameArray = array();
		$NameArrayAll = array();
		for($i = 0; $i < $FieldNameCounter; $i++)
		{
			$Name = $FieldNameArray[$i];
			$NameArray = array($Name);
			$NameArrayAll = array_merge($NameArrayAll, $NameArray);
		}
		$jsondata['fields'] = $NameArrayAll;

		for($x = 0; $x < $Counter; $x++)
		{
			$jsondata['data'][$x] = $ValueArrayJson[$x];
		}
	}
endsql:
	$tool->SqlClose($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>
