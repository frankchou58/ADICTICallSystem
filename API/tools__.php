<?php
	class tools
	{
		function SqlConnectCPF144()
		{		
			$user = 'frank';
			$passwd = 'frank*5816';
			$database = 'CPF144';
		
			$constr="Driver={SQL Server};Server=127.0.0.1;Client_CSet=UTF-8;Server_CSet=UTF-8;Database=" . $database;
			$link = odbc_connect($constr,$user,$passwd,SQL_CUR_USE_ODBC);
			if(!$link)
			{
				return -1;
			}
			return $link;
		}		

		function SqlExec($link, $sql)
		{
    		$result = odbc_exec($link, $sql);
    		return $result;
		}

		function SqlFetch($result)
		{
			return odbc_fetch_row($result);
		}

		function FetchOdbcData($result, $Field)
		{
			return odbc_result($result, $Field);
		}

		function SqlFetchArray($result)
		{
			return odbc_fetch_array($result);
		}

		function FetchMySqlData($result, $Field)
		{
			return $result[$Field];
		}

		function SqlError()
		{
			$Error = odbc_error();

			return $Error;
		}

		function SqlErrorMsg()
		{
			$ErrorMsg = odbc_errormsg();

			return $ErrorMsg;
		}

		function SqlClose($link)
		{	
			odbc_close($link);
		}
	}
?>
