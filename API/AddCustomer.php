<?php
	error_reporting(0);
	$jsondata = array();
	$CustomerId1 = htmlspecialchars($_GET["CustomerId"]);
	$Name1 = htmlspecialchars($_GET["Name"]);
	$Gender1 = htmlspecialchars($_GET["Gender"]);
	$TelNo1 = htmlspecialchars($_GET["TelNo"]);
	$County1 = htmlspecialchars($_GET["County"]);
	$Townships1 = htmlspecialchars($_GET["Townships"]);
	$Address1 = htmlspecialchars($_GET["Address"]);
	$Birthday1 = htmlspecialchars($_GET["Birthday"]);
	$CustomerId2 = htmlspecialchars($_POST["CustomerId"]);
	$Name2 = htmlspecialchars($_POST["Name"]);
	$Gender2 = htmlspecialchars($_POST["Gender"]);
	$TelNo2 = htmlspecialchars($_POST["TelNo"]);
	$County2 = htmlspecialchars($_POST["County"]);
	$Townships2 = htmlspecialchars($_POST["Townships"]);
	$Address2 = htmlspecialchars($_POST["Address"]);
	$Birthday2 = htmlspecialchars($_POST["Birthday"]);

	if($CustomerId1 != NULL || $Name1 != NULL || $Gender1 != NULL || $TelNo1 != NULL || $County1 != NULL || $Townships1 != NULL || $Address1 != NULL || $Birthday1 != NULL)
	{
		$CustomerId = $CustomerId1;
		$Name = $Name1;
		$Gender = $Gender1;
		$TelNo = $TelNo1;
		$County = $County1;
		$Townships = $Townships1;
		$Address = $Address1;
		$Birthday = $Birthday1;
	}
	else if($CustomerId2 != NULL || $Name2 != NULL || $Gender2 != NULL || $TelNo2 != NULL || $County2 != NULL || $Townships2 != NULL || $Address2 != NULL || $Birthday2 != NULL)
	{
		$CustomerId = $CustomerId2;
		$Name = $Name2;
		$Gender = $Gender2;
		$TelNo = $TelNo2;
		$County = $County2;
		$Townships = $Townships2;
		$Address = $Address2;
		$Birthday = $Birthday2;
	}
	
	if($CustomerId == NULL || $Name == NULL || $Gender == NULL || $TelNo == NULL || $County == NULL || $Townships == NULL || $Address == NULL || $Birthday == NULL)
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

	$StrName = iconv("UTF-8", "big5", $Name);
	$StrCounty = iconv("UTF-8", "big5", $County);	
	$StrTownships = iconv("UTF-8", "big5", $Townships);	
	$StrAddress = iconv("UTF-8", "big5", $Address);	
	$String = "$CustomerId:$TelNo@adicti.com.tw";   
 	$CustomerUUID = $tool->GenUUID($String);
  list($starttime, $sec) = explode(" ", microtime());
	$Ret = $tool->AddCustomer($Connected, $CustomerId, $CustomerUUID, $StrName, $Gender, $TelNo, $StrCounty, $StrTownships, $StrAddress, $Birthday);
	if($Ret != 'ok')
	{
		$jsondata['status'] = false;
		$jsondata['message'] = $Ret;		
		goto endsql;
	}
	else
	{
  	list($endtime, $sec) = explode(" ", microtime());
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['UUID'] = $CustomerUUID;		
	}
endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>