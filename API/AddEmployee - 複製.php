<?php
	error_reporting(0);
	$jsondata = array();
	$EmpID1 = htmlspecialchars($_GET["EmpID"]);
	$Name1 = htmlspecialchars($_GET["Name"]);
	$Attrib1 = htmlspecialchars($_GET["Attrib"]);
	$ID1 = htmlspecialchars($_GET["ID"]);
	$DepID1 = htmlspecialchars($_GET["DepID"]);
	$Tel11 = htmlspecialchars($_GET["Tel1"]);
	$Contact1 = htmlspecialchars($_GET["Contact"]);
	$Addr11 = htmlspecialchars($_GET["Addr1"]);
	$Addr21 = htmlspecialchars($_GET["Addr2"]);
	$Corp1 = htmlspecialchars($_GET["Corp"]);
	$NickName1 = htmlspecialchars($_GET["NickName"]);
	$Tel21 = htmlspecialchars($_GET["Tel2"]);
	$Note1 = htmlspecialchars($_GET["Note"]);

	$EmpID2 = htmlspecialchars($_POST["EmpID"]);
	$Name2 = htmlspecialchars($_POST["Name"]);
	$Attrib2 = htmlspecialchars($_POST["Attrib"]);
	$ID2 = htmlspecialchars($_POST["ID"]);
	$DepID2 = htmlspecialchars($_POST["DepID"]);
	$Tel12 = htmlspecialchars($_POST["Tel1"]);
	$Contact2 = htmlspecialchars($_POST["Contact"]);
	$Addr12 = htmlspecialchars($_POST["Addr1"]);
	$Addr22 = htmlspecialchars($_POST["Addr2"]);
	$Corp2 = htmlspecialchars($_POST["Corp"]);
	$NickName2 = htmlspecialchars($_POST["NickName"]);
	$Tel22 = htmlspecialchars($_POST["Tel2"]);
	$Note2 = htmlspecialchars($_POST["Note"]);

	if($EmpID1 != NULL || $Name1 != NULL || $Attrib1 != NULL || $ID1 != NULL || $DepID1 != NULL || $Tel11 != NULL || $Contact1 != NULL || $Addr11 != NULL  || $Addr21 != NULL)
	{
		$EmpID = $EmpID1;
		$Name = $Name1;
		$Attrib = $Attrib1;
		$ID = $ID1;
		$DepID = $DepID1;
		$Tel1 = $Tel11;
		$Contact = $Contact1;
		$Addr1 = $Addr11;
		$Addr2 = $Addr21;
		$Corp = $Corp1;
		$NickName = $NickName1;
		$Tel2 = $Tel21;
		$Note = $Note1;
	}
	else if($EmpID2 != NULL || $Name2 != NULL || $Attrib2 != NULL  || $ID2 != NULL || $DepID2 != NULL || $Tel12 != NULL || $Contact2 != NULL || $Addr12 != NULL  || $Addr22 != NULL)
	{
		$EmpID = $EmpID2;
		$Name = $Name2;
		$Attrib = $Attrib2;
		$ID = $ID2;
		$DepID = $DepID2;
		$Tel1 = $Tel12;
		$Contact = $Contact2;
		$Addr1 = $Addr12;
		$Addr2 = $Addr22;
		$Corp = $Corp2;
		$NickName = $NickName2;
		$Tel2 = $Tel22;
		$Note = $Note2;
	}
	
	if($EmpID == NULL || $Name == NULL || $Attrib == NULL  || $ID == NULL || $DepID == NULL || $Tel1 == NULL || $Contact == NULL || $Addr1 == NULL  || $Addr2 == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!! Must Have EmpID & Name & Attrib & ID & DepID & Tel1 & Contact & Addr1 & Addr2';		
		goto end;
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
	$SqlFields = '(EmpID,EmpName,EmpState,EmpIdentityID,EmpDepID,EmpTEL1,EmpConnect,EmpAddr1,EmpAddr2,EmpLastEdit';
	$NameStr = iconv("UTF-8", "big5", $Name);
	$AttribStr = iconv("UTF-8", "big5", $Attrib);
	$IDStr = iconv("UTF-8", "big5", $ID);
	$Tel1Str = iconv("UTF-8", "big5", $Tel1);
	$ContactStr = iconv("UTF-8", "big5", $Contact);
	$Addr1Str = iconv("UTF-8", "big5", $Addr1);
	$Addr2Str = iconv("UTF-8", "big5", $Addr2);

	$Current = date('Y-m-d H:i:s');
	$SqlContents = "VALUES('$EmpID','$NameStr','$AttribStr','$IDStr','$DepID','$Tel1Str','$ContactStr','$Addr1Str','$Addr2Str','$Current'";
	if($Corp != NULL)
	{
		$SqlFields = $SqlFields . ',EmpCorpID';
		$SqlContents = $SqlContents . ",'$Corp'";
	}
	if($NickName != NULL)
	{
		$SqlFields = $SqlFields . ',EmpNickname';
		$NickNameStr = iconv("UTF-8", "big5", $NickName);
		$SqlContents = $SqlContents . ",'$NickNameStr'";
	}
	if($Tel2 != NULL)
	{
		$SqlFields = $SqlFields . ',EmpTEL2';
		$Tel2Str = iconv("UTF-8", "big5", $Tel2);
		$SqlContents = $SqlContents . ",'$Tel2Str'";
	}
	if($Note != NULL)
	{
		$SqlFields = $SqlFields . ',EmpMemo';
		$NoteStr = iconv("UTF-8", "big5", $Note);
		$SqlContents = $SqlContents . ",'$NoteStr'";
	}

	$SqlFields = $SqlFields . ')';
	$SqlContents = $SqlContents . ')';
	$sql="INSERT INTO $TableName $SqlFields $SqlContents";
	//echo "Sql = $sql\n";
	$result = odbc_exec($Connected, $sql);		
	if(!$result)
	{
		if(odbc_error() == 23000)
		{
			$jsondata['status'] = false;
			$jsondata['message'] = 'Existed!!!!!!';		
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
