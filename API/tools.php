<?php
	class tools
	{
		function GenUUID($String)
		{
			$UUID = strtoupper(md5($String));
			$hyphen = "";
			$key =
      	    substr($UUID, 0, 8).$hyphen
        	  .substr($UUID, 8, 4).$hyphen
						.substr($UUID,12, 4).$hyphen
           	.substr($UUID,16, 4).$hyphen
           	.substr($UUID,20,12)
           	;
    		return $key;
		}

		function SqlConnect()
		{		
			$user = 'frank';
			$passwd = 'frank*5816';
			$database = 'ADICTICallCenter';
		
			$constr="Driver={SQL Server};Server=localhost;Client_CSet=UTF-8;Server_CSet=UTF-8;Database=" . $database;
			$link = odbc_connect($constr,$user,$passwd,SQL_CUR_USE_ODBC);
			if(!$link)
			{
				return -1;
			}
			return $link;
		}		
	
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

		function CreateOutlineTable($link)
		{
    		$TableName = 'outline';
			$sql="CREATE TABLE $TableName 
							(
								ID INT IDENTITY(1,1) PRIMARY KEY, 
								line_no	INT unique,
								line_in_used tinyint,
								line_status INT,
								group_id INT,
								machine_id INT,
								phy_port_typeA INT,
								phy_port_typeB INT,
								phy_port_typeC INT
							)";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}

		function CreateExtlineTable($link)
		{
    		$TableName = 'extline';
			$sql="CREATE TABLE $TableName 
							(
								ID INT IDENTITY(1,1) PRIMARY KEY,
								sub_program_id INT,
								port_no INT unique,
								ext_no INT,
								phy_port INT,
								ext_status INT,
								operator_uuid	CHAR(33),
							)";
			$result = odbc_exec($link, $sql);	
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}

		function CreateOperatorTable($link)
		{
    		$TableName = 'operators';
			$sql="CREATE TABLE $TableName 
							(
								ID INT IDENTITY(1,1) PRIMARY KEY,
								employee_id INT unique,
								operator_uuid	NVARCHAR(33),
								name NVARCHAR(20),
								machine_id INT,
								login_time INT,
								logout_time INT,
								level INT,
								ext_num INT
							)";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}
    
		function CreateCustomerTable($link)
		{
    		$TableName = 'customers';
			$sql="CREATE TABLE $TableName 
							(
								ID INT IDENTITY(1,1) PRIMARY KEY,
								customer_id INT unique,
								name NVARCHAR(20),
								birthday date,
								gender CHAR(1),
								telno NVARCHAR(50),
								email NVARCHAR(100),
								customer_uuid	NVARCHAR(33),
								county NVARCHAR(50),
								townships NVARCHAR(50),
								address1 NVARCHAR(200),
								in_black_list INT
							)";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}
    
		function CreateAdminTable($link)
		{
    		$TableName = 'admin';
			$sql="CREATE TABLE $TableName 
							(
								ID INT IDENTITY(1,1) PRIMARY KEY,
								employee_id	NVARCHAR(50),
								name NVARCHAR(20),
								supervisor_uuid	NVARCHAR(33)
							)";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}
    	
		function CreateRecordTable($link)
		{
    		$TableName = 'record';
			$sql="CREATE TABLE $TableName 
							(
								ID INT IDENTITY(1,1) PRIMARY KEY,
								out_vport_no INT,
								ext_num INT,
								customer_uuid	CHAR(33),
								operator_uuid	CHAR(33),
								call_type INT,
								call_start_time INT,
								call_connect_time INT,
								call_end_time INT,
								call_duration_time INT,
								call_second_dtmf CHAR(100),
								voice_record CHAR(100),
								machine_id INT,
								tel_no CHAR(50),
								out_phyport_no INT,
								ring_times INT,
								internal_call_from_ext_no INT,
								internal_call_to_ext_no INT
							)";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}
    
		function CreateMachineTable($link)
		{
    		$TableName = 'machine';
			$sql="CREATE TABLE $TableName 
							(
								ID INT IDENTITY(1,1) PRIMARY KEY,
								machine_type INT,
								machine_id INT,
								alias	NCHAR(50) COLLATE Chinese_Taiwan_Stroke_CS_AS,
								out_port_num INT,
								ext_port_num INT
							)";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}
    
		function CreateLogTable($link)
		{
    		$TableName = 'log';
			$sql="CREATE TABLE $TableName 
							(
								ID INT IDENTITY(1,1) PRIMARY KEY,
								timestamp INT,
								type INT,
								message	NCHAR(200) COLLATE Chinese_Taiwan_Stroke_CS_AS,
							)";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}
    
		function AddOutPort($link, $LinePort)
		{
    		$TableName = 'outline';
			$sql="INSERT INTO $TableName (line_no,line_in_used) VALUES('$LinePort',0)";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
				return odbc_error();
			}
			else
			return 'ok';    	
		}
    
		function AddOperator($link, $employeeid, $password, $EmployeeUUID)
		{
			require_once 'tools.php';
			$tool = new tools();
    		$TableName = 'operators';
			$sql="INSERT INTO $TableName (operator_uuid,employee_id) VALUES('$EmployeeUUID','$employeeid')";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}
    
		function AddExtPort($link, $ExtPort)
		{
    		$TableName = 'extline';
			$sql="INSERT INTO $TableName (port_no,phy_port) VALUES ('$ExtPort','$ExtPort')";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}

		function AddCustomer($link, $Id, $CustomerUUID, $Name, $Gender, $TelNo, $County, $Townships, $Address, $Birthday)
		{
    		$TableName = 'customers';
			$sql="INSERT INTO $TableName (telno,name,customer_uuid,customer_id,gender,birthday,county,townships,address1) 
					VALUES('$TelNo','$Name','$CustomerUUID','$Id','$Gender','$Birthday','$County','$Townships','$Address')";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
				return odbc_error();
			}
			else
				return 'ok';    	
		}


		function AddMachine($link, $MachineType, $MachineID)
		{
			require_once 'tools.php';
			$tool = new tools();

    		$TableName = 'machine';
			$sql="INSERT INTO $TableName (machine_type,machine_id) VALUES('$MachineType','$MachineID')";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}
		
		function LinkExtToOperator($ExtNo, $OperatorUUID)
		{
    		/* Find ExtNo */
    		$TableName = 'extline';
			$sql="SELECT * FROM $TableName WHERE ext_no=\"$ExtNo\"";
			//echo $sql;
			$result = mysql_query($sql);		
			if (!$result)
			{
		    return mysql_error();
			}
			$Counter = 0;
			while ($row = mysql_fetch_assoc($result))
			{
				$Counter++;
			}
			//echo "Counter = $Counter";
			if($Counter == 0)
			{
				return "ExtNo:$ExtNo is not found";
			}
    		/* Find OperatorUUID */
    		$TableName = 'operators';
			$sql="SELECT * FROM $TableName WHERE operator_uuid=\"$OperatorUUID\"";
			//echo $sql;
			$result = mysql_query($sql);		
			if (!$result)
			{
				return mysql_error();
			}
			$Counter = 0;
			while ($row = mysql_fetch_assoc($result))
			{
				$Counter++;
			}
			//echo "Counter = $Counter";
			if($Counter == 0)
			{
				return "OperatorUUID:$OperatorUUID is not found";
			}
    		$TableName = 'extline';
			$sql="UPDATE $TableName SET operator_uuid=\"$OperatorUUID\" WHERE ext_no=\"$ExtNo\"";
			$result = mysql_query($sql);		
			if (!$result)
			{
				return mysql_error();
			}
			else
				return 'ok';    	
		}
    
		function SetExtPortNumber($link, $SubProgramID, $ExtPort, $ExtNo)
	    {
			/* Find Ext Port */
    		$TableName = 'extline';
			$sql="SELECT * FROM $TableName WHERE port_no='$ExtPort'";
			//echo $sql;
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
				return odbc_error();
			}
			$Counter = 0;
			while ($row = odbc_fetch_row($result))
			{
				$Counter++;
			}
			//echo "Counter = $Counter";
			if($Counter == 0)
			{
				return "Ext Port:$ExtPort is not found";
			}
    	
			$sql="UPDATE $TableName SET ext_no='$ExtNo',sub_program_id='$SubProgramID' WHERE port_no='$ExtPort'";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
		    return odbc_error();
			}
			else
				return 'ok';    	
		}	

	    function AddSupervisor($link, $employeeid, $password)
		{
			require_once 'tools.php';
			$tool = new tools();
			$String = "$employeeid:$password@adicti.com.tw";   
    		$AdminUUID = $tool->GenUUID($String);
    		$TableName = 'admin';
			$sql="INSERT INTO $TableName (admin_uuid,employee_id) VALUES('$AdminUUID','$employeeid')";
			$result = odbc_exec($link, $sql);		
			if (!$result)
			{
				return odbc_exec();
			}
			else
				return 'ok';    	
		}
	}
?>
