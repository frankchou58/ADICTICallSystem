<?php
	error_reporting(0);
	$jsondata = array();
	$ID1 = htmlspecialchars($_GET["ID"]);
	$Name1 = htmlspecialchars($_GET["Name"]);
	$Gender1 = htmlspecialchars($_GET["Gender"]);
	$TelNo1 = htmlspecialchars($_GET["TelNo"]);
	$County1 = htmlspecialchars($_GET["County"]);
	$Townships1 = htmlspecialchars($_GET["Townships"]);
	$Address1 = htmlspecialchars($_GET["Address"]);
	$Birthday1 = htmlspecialchars($_GET["Birthday"]);
	$InBlackList1 = htmlspecialchars($_GET["InBlackList"]);
	$ID2 = htmlspecialchars($_POST["ID"]);
	$Name2 = htmlspecialchars($_POST["Name"]);
	$Gender2 = htmlspecialchars($_POST["Gender"]);
	$TelNo2 = htmlspecialchars($_POST["TelNo"]);
	$County2 = htmlspecialchars($_POST["County"]);
	$Townships2 = htmlspecialchars($_POST["Townships"]);
	$Address2 = htmlspecialchars($_POST["Address"]);
	$Birthday2 = htmlspecialchars($_POST["Birthday"]);
	$InBlackList2 = htmlspecialchars($_POST["InBlackList"]);

	if($ID1 != NULL || $Name1 != NULL || $Gender1 != NULL || $TelNo1 != NULL || $County1 != NULL || $Townships1 != NULL || $Address1 != NULL || $Birthday1 != NULL || $InBlackList1 != NULL)
	{
		$ID = $ID1;
		$Name = $Name1;
		$Gender = $Gender1;
		$TelNo = $TelNo1;
		$County = $County1;
		$Townships = $Townships1;
		$Address = $Address1;
		$Birthday = $Birthday1;
		$InBlackList = $InBlackList1;
	}
	else if($ID2 != NULL || $Name2 != NULL || $Gender2 != NULL || $TelNo2 != NULL|| $County2 != NULL || $Townships2 != NULL  || $Address2 != NULL || $Birthday2 != NULL || $InBlackList2 != NULL)
	{
		$ID = $ID2;
		$Name = $Name2;
		$Gender = $Gender2;
		$TelNo = $TelNo2;
		$County = $County2;
		$Townships = $Townships2;
		$Address = $Address2;
		$Birthday = $Birthday2;
		$InBlackList = $InBlackList2;
	}
	
	if($ID == NULL || $Name == NULL || $Gender == NULL || $TelNo == NULL || $County == NULL || $Townships == NULL  || $Address == NULL || $Birthday == NULL || $InBlackList == NULL)
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
	$TableName = 'customers';
	$sql="UPDATE $TableName SET name='$StrName',telno='$TelNo',customer_uuid='$CustomerUUID',gender='$Gender',birthday='$Birthday',
			county='$StrCounty',townships='$StrTownships',address1='$StrAddress',in_black_list='$InBlackList' WHERE ID='$ID'";
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
  list($endtime, $sec) = explode(" ", microtime());
	if(!$result)
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "Database SELECT $table Error: " . odbc_error() . "!!!";
		goto endsql;
	}

 	list($endtime, $sec) = explode(" ", microtime());
	$jsondata['status'] = true;		
	$jsondata['esptime'] = $endtime - $starttime;
	$jsondata['message'] = 'Update Info Success!!!';		

endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>