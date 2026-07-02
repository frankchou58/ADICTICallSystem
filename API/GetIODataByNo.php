<?php
	error_reporting(0);
	$jsondata = array();
	$IO1 = htmlspecialchars($_GET["IO"]);
	$IO2 = htmlspecialchars($_POST["IO"]);

	if($IO1 != NULL)
	{
		$IO = $IO1;
	}
	else if($IO2 != NULL)
	{
		$IO = $IO2;
	}
	
	if($IO == NULL)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = 'Please check parameters!!!';		
		goto end;
	}

	require_once 'tools.php';
	$tool = new tools();
	
	$Connected = $tool->SqlConnectCPF144();
	if($Connected < 0)
	{
		$jsondata['status'] = false;		
		$jsondata['message'] = $Connected;		
		goto endsql;
	}

	$sql="SELECT dbo.IO1.IO1ID, dbo.IO1.IO1CorpID, dbo.IO2.IO2GoodsID, dbo.IO2.IO2BoxQuantity, dbo.IO2.IO2Quantity, 
                 dbo.IO2.IO2Price, dbo.IO1.IO1CustID, dbo.IO1.IO1Date, dbo.IO1.IO1Type, dbo.IO1.IO1DeliveryAddr, 
                 dbo.IO1.IO1ChargeDate, dbo.IO1.IO1InvoiceNo, dbo.Goods.GoodsID, dbo.Goods.GoodsName, dbo.Cust.CustName, 
                 dbo.Cust.CustAddr
			FROM dbo.IO1 INNER JOIN
                 dbo.IO2 ON dbo.IO1.IO1ID = dbo.IO2.IO1ID INNER JOIN
                 dbo.Goods ON dbo.IO2.IO2GoodsID = dbo.Goods.GoodsID INNER JOIN
                 dbo.Cust ON dbo.IO1.IO1CustID = dbo.Cust.CustID
			WHERE (dbo.IO1.IO1ID = '$IO')";
  list($starttime, $sec) = explode(" ", microtime());
	$result = odbc_exec($Connected, $sql);
  list($endtime, $sec) = explode(" ", microtime());
	if(!$result)
	{
		$jsondata['status'] = false;
		$jsondata['message'] = "Database SELECT $table Error: " . odbc_error() . "!!!";
		goto endsql;
	}
	$Counter = 0;
	$IO1ID = array();
	$IO1Type = array();
	$IO1Date = array();
	$IO1DeliveryAddr = array();
	$GoodsName = array();
	$CustName = array();
	$CustAddr = array();
	while ($row = odbc_fetch_row($result))
	{
		$IO1ID[$Counter] = odbc_result($result, 'IO1ID');
		$IO1Type[$Counter] = odbc_result($result, 'IO1Type');
		$IO1Date[$Counter] = odbc_result($result, 'IO1Date');
		$IO1DeliveryAddr[$Counter] = odbc_result($result, 'IO1DeliveryAddr');
		$GoodsName[$Counter] = odbc_result($result, 'GoodsName');
		$CustName[$Counter] = odbc_result($result, 'CustName');
		$CustAddr[$Counter] = odbc_result($result, 'CustAddr');
		$Counter++;
	}
	if($Counter > 0)
	{
		$jsondata['status'] = true;		
		$jsondata['esptime'] = $endtime - $starttime;
		$jsondata['numbers'] = $Counter;
		$jsondata['fields'] = array('IO', 'Type', 'Date', 'Delivery Address', 'Goods Name', 'Customer', 'Customer Address');
		for($x = 0; $x < $Counter; $x++)
		{
			$IO1TypeStr = iconv("big5","UTF-8",$IO1Type[$x]);
			$IO1DeliveryAddrStr = iconv("big5","UTF-8",$IO1DeliveryAddr[$x]);
			$GoodsNameStr = iconv("big5","UTF-8",$GoodsName[$x]);
			$CustNameStr = iconv("big5","UTF-8",$CustName[$x]);
			$CustAddrStr = iconv("big5","UTF-8",$CustAddr[$x]);
			$jsondata['data'][$x] = array($IO1ID[$x], $IO1TypeStr, $IO1Date[$x], $IO1DeliveryAddrStr, $GoodsNameStr, $CustNameStr, $CustAddrStr);
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
