<?php
	error_reporting(0);
	$jsondata = array();
	require_once 'tools.php';
	$tool = new tools();
	
	$Connected = $tool->SqlConnect();
	if($Connected < 0)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = $Connected;		
		goto endsql;
	}

	$TableName = 'customers';
	$sql="SELECT * FROM $TableName";
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
  list($endtime, $sec) = explode(" ", microtime());
	if(!$result)
	{
		if(odbc_error() == 'S0002')
		{
			$ret = $tool->CreateCustomerTable($Connected);
			if($ret == 'ok')
			{
				$jsondata['status'] = true;
				$jsondata['numbers'] = 0;
			}
			else
			{
				$jsondata['status'] = false;
				$jsondata['message'] = "Create $table table Error: " . $ret . "!!!";
			}
			goto endsql;
		}
		else
		{
			$jsondata['status'] = false;
			$jsondata['message'] = "Database SELECT $table Error: " . odbc_error() . "!!!";
			goto endsql;
		}
	}
	$Counter = 0;
	$ID = array();
	$UUID = array();
	$CustomerId = array();
	$Name = array();
	$County = array();
	$Townships = array();
	$Address = array();
	$TelNo = array();
	$Gender = array();
	$Birthday = array();
	$InBlackList = array();
	while ($row = odbc_fetch_row($result))
	{
		$ID[$Counter] = odbc_result($result, 'ID');
		$UUID[$Counter] = odbc_result($result, 'customer_uuid');
		$CustomerId[$Counter] = odbc_result($result, 'customer_id');
		$Name[$Counter] = odbc_result($result, 'name');
		$County[$Counter] = odbc_result($result, 'county');
		$Townships[$Counter] = odbc_result($result, 'townships');
		$Address[$Counter] = odbc_result($result, 'address1'); 
		$TelNo[$Counter] = odbc_result($result, 'telno'); 
		$Gender[$Counter] = odbc_result($result, 'gender'); 
		$Birthday[$Counter] = odbc_result($result, 'birthday'); 
		$InBlackList[$Counter] = odbc_result($result, 'in_black_list'); 
		$Counter++;
	}
	if($Counter > 0)
	{
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['numbers'] = $Counter;
		$jsondata['fields'] = array('ID','Customer ID','UUID','Name','Gender','Telephone No','County','Townships','Address','Birthday','InBlackList');
		for($x = 0; $x < $Counter; $x++)
		{
			$StrName = iconv("big5","UTF-8",$Name[$x]);
			$StrCounty = iconv("big5","UTF-8",$County[$x]);
			$StrATownships = iconv("big5","UTF-8",$Townships[$x]);
			$StrAddress = iconv("big5","UTF-8",$Address[$x]);	
			$jsondata['data'][$x] = array($ID[$x], $CustomerId[$x], $UUID[$x], $StrName, $Gender[$x], $TelNo[$x], $StrCounty, $StrATownships, $StrAddress, $Birthday[$x], $InBlackList[$x]);
		}
	}
	else
	{
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['numbers'] = 0;		
	}

endsql:
	odbc_close($Connected);
end:
	echo json_encode($jsondata);	
	header('Content-Type: application/json;charset=UTF-8');		
	header('Access-Control-Allow-Origin: *');		
?>