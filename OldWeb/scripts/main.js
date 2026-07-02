let BackEndURL = 'http://211.75.76.136:8830/API/';
let WSURL = 'ws://211.75.76.136:3000';

function GetWSURL()
{
	return WSURL;
}

function AdminLoginResponse(evt)
{
	let KeyId;
	const para = document.querySelector('h3');
  if(this.readyState === XMLHttpRequest.DONE && this.status === 200)
  {
  	console.log("this.responseText: " + this.responseText);
  	var Response = JSON.parse(this.responseText);
		console.log('Response: ' + Response.status);
		if(Response.status == false)
		{
			para.textContent = '登入失敗:' + Response.message;
			sessionStorage.setItem('LoginResult', false);
		}
		else if(Response.status == true)
		{
			para.textContent = '登入成功';
			AdminUUID = Response.data[1];
			sessionStorage.setItem('AdminKey', AdminUUID);
			sessionStorage.setItem("Mode", 2);
			sessionStorage.setItem('LoginResult', true);
		}
	}
}

function UserLoginResponse(evt)
{
	let UserKeyId;
	const para = document.querySelector('h3');
	//console.log('Complete User Login');
 	//console.log(this.status);       	
  if(this.readyState === XMLHttpRequest.DONE && this.status === 200)
  {
  	console.log(this.responseText);
  	var Response = JSON.parse(this.responseText);

		if(Response.status == false)
		{
			para.textContent = '登入失敗:' + Response.message;
			sessionStorage.setItem('LoginResult', false);
		}
		else if(Response.status == true)
		{
			console.log("data : " + Response.data);
			OperatorKey = Response.data[0];
			console.log("OperatorKey : " + OperatorKey);
			para.textContent = '登入成功';			
			sessionStorage.setItem('OperatorKey', OperatorKey);
			sessionStorage.setItem("Mode", 3);
			sessionStorage.setItem('LoginResult', true);
		}
	}
}

function AddDusersOption()
{
	var DusersJsonArray = sessionStorage.getItem("Dusers");
  console.log("DusersJsonArray: " + DusersJsonArray);
  var Dusers = JSON.parse(DusersJsonArray);
	var select = document.getElementById("duserlistoption");
	var DuserNumbers = Dusers.numbers;
	for(let i = 0; i < DuserNumbers; i++)
	{
   	var opt = document.createElement('option');
		opt.value = Dusers.data[i][0];
		opt.innerHTML = Dusers.data[i][1];
		select.appendChild(opt);
	}
}
			
function CheckAdminDataResponse()
{
	const $result = $("#result");
	if(this.readyState === XMLHttpRequest.DONE && this.status === 200)
	{
		console.log(this.responseText);
		Val = JSON.parse(this.responseText);
				
		if(Val.status == false)
		{
			$result.text('第一次登入管理者帳號，請輸入您的管理者密碼。');
			$result.css("color", "rgb(189 10 0)");
	    sessionStorage.setItem('LoginFrist', true);
		}
		else
		{
	    sessionStorage.setItem('LoginFrist', false);
		}
	}			
}
			
function CheckAdminData()
{
	var xmlHttp = new XMLHttpRequest();
	var URL = BackEndURL + 'CheckAdminData.php';
	console.log('URL = ' + URL);
	xmlHttp.open( "GET", URL, false);
	xmlHttp.onreadystatechange = CheckAdminDataResponse;
	xmlHttp.send();
}			

function Login()
{
	var chkAdmin = document.getElementById("chkAdmin");
	var	chkUser  = document.getElementById("chkUser");
  const form = document.forms['myLogin'];
  const username = form.elements.username.value;
  const Password = form.elements.pwd.value;
  var xmlHttp = new XMLHttpRequest();

 	if(chkAdmin.checked)
 	{
    var LoginFrist = sessionStorage.getItem('LoginFrist');
    console.log('LoginFrist: ' + LoginFrist);
		if(LoginFrist == "true")
		{
    	var URL = BackEndURL + 'SetAdminPassword.php?Password=' + Password;
 			console.log("URL:" + URL);
    	xmlHttp.open( "GET", URL, false);
    	xmlHttp.onreadystatechange = AdminLoginResponse;
    	xmlHttp.send();
    	var AdminLoginResult = sessionStorage.getItem('LoginResult');
    	console.log(AdminLoginResult);
			if(AdminLoginResult == "true")
			{	
				sessionStorage.setItem("Page", 1);
				top.location.href="index.html";
			}
		}
		else
		{
    	//var URL = 'http://172.16.0.109:8841/API/AdminLogin.php?Id=' + username + '&Password=' + Password;
    	var URL = BackEndURL + 'AdminLogin.php?Password=' + Password;
 			console.log("URL:" + URL);
    	xmlHttp.open( "GET", URL, false);
    	xmlHttp.onreadystatechange = AdminLoginResponse;
    	xmlHttp.send();
    	var AdminLoginResult = sessionStorage.getItem('LoginResult');
    	console.log(AdminLoginResult);
			if(AdminLoginResult == "true")
			{	
				sessionStorage.setItem("Page", 1);
				top.location.href="index.html";
			}
		}
 	}
 	else if(chkUser.checked)
 	{
 		{
			console.log('User Login');
			console.log("ID:" + username);
	 		console.log("Password:" + Password);
	    //var URL = 'http://172.16.0.109:8841/API/Login.php?Id=' + username + '&Password=' + Password;
	    var URL = BackEndURL + 'Login.php?Id=' + username + '&Password=' + Password;
	    xmlHttp.open( "GET", URL, false);
	    xmlHttp.onreadystatechange = UserLoginResponse;
	    xmlHttp.send();
	    var UserLoginResult = sessionStorage.getItem('LoginResult');
	    console.log(UserLoginResult);
			//QueryDusersByUser(UserKeyId);
	    if(UserLoginResult == "true")
	    {
			sessionStorage.setItem("Page", 1);
			top.location.href="index.html";			
		}
		}
 	}	
}

function GetEmpsResponse()
{
	if(this.readyState === XMLHttpRequest.DONE && this.status === 200)
	{
		var OperatorsArray = this.responseText;
		console.log('OperatorsArray = ' + OperatorsArray);
		sessionStorage.setItem("Operators", OperatorsArray);
	}
}

function GetEmps()
{
	var xmlHttp = new XMLHttpRequest();
	var URL = BackEndURL + 'GetEmpInfo.php';
	console.log('URL = ' + URL);
	xmlHttp.open( "GET", URL, false);
	xmlHttp.onreadystatechange = GetEmpsResponse;
	xmlHttp.send();
}

function QueryCustomersInfoResponse()
{
	if(this.readyState === XMLHttpRequest.DONE && this.status === 200)
	{
		Customers = this.responseText;
		console.log('Customers = ' + Customers);
		sessionStorage.setItem("Customers", Customers);
	}
}

function GetCustomersInfo()
{
	var xmlHttp = new XMLHttpRequest();
  var URL = BackEndURL + 'GetCustomers.php';
	console.log('URL = ' + URL);
	xmlHttp.open( "GET", URL, false);
	xmlHttp.onreadystatechange = QueryCustomersInfoResponse;
	xmlHttp.send();
}


function QueryCallRecordInfoResponse()
{
	if(this.readyState === XMLHttpRequest.DONE && this.status === 200)
	{
		CallRecordInfo = this.responseText;
		sessionStorage.setItem("CallRecordInfo", CallRecordInfo);
	}
}
	
function SetEmployeeInfoResponse()
{
	console.log(this.responseText);       	
	if(this.readyState === XMLHttpRequest.DONE && this.status === 200)
	{
		const $result = $("#result");
		var RespArray = JSON.parse(this.responseText);
		console.log("Response: " + RespArray.status);
		if(RespArray.status == false)
		{
			Response = 0;
			$result.text(RespArray.message);	
		}

	}
	sessionStorage.setItem("bWaitResponse", 0);
}

function SetEmployeeInfo(type, Value)
{
	console.log("ThisEmployeeID: " + ThisEmployeeID);
	var URL;
	if(type == "name")
	  URL = BackEndURL + 'SetOperatorName.php?Key=' + ThisUUID + '&Name=' + Value;
	else if(type == "machinesign")
	  URL = BackEndURL + 'SetOperatorMachineID.php?EmpID=' + ThisEmployeeID + '&MachineId=' + Value;
	else if(type == "machineunsign")
	  URL = BackEndURL + 'UnsetOperatorMachineID.php?EmpID=' + ThisEmployeeID + '&MachineId=' + Value;
	else if(type == "password")
	  URL = BackEndURL + 'ChangeOperatorPassword.php?Key=' + ThisUUID + '&Password=' + Value;
	else if(type == "ExtNum")
	  URL = BackEndURL + 'SetOperatorExtNum.php?ID=' + ThisEmployeeID + '&Ext=' + Value;

	console.log("URL: " + URL);
	var xmlHttp = new XMLHttpRequest();
	xmlHttp.open( "GET", URL, false);
	xmlHttp.onreadystatechange = SetEmployeeInfoResponse;
	xmlHttp.send();
}

function GetCallRecordInfo(StartUTC, EndUTC)
{
	//console.log('StartUTC = ' + StartUTC);
	//console.log('EndUTC = ' + EndUTC);
	var xmlHttp = new XMLHttpRequest();
	//var URL = 'http://172.16.0.109:8841/API/GetCallRecordInfo.php?StartTime=' + StartUTC + '&EndTime=' + EndUTC;
  var URL = BackEndURL + 'GetCallRecordInfo.php?StartTime=' + StartUTC + '&EndTime=' + EndUTC;
	//console.log('URL = ' + URL);
	xmlHttp.open( "GET", URL, false);
	xmlHttp.onreadystatechange = QueryCallRecordInfoResponse;
	xmlHttp.send();
}


function SetExtNnmResponse()
{
	const $result = $("#result");
	if(this.readyState === XMLHttpRequest.DONE && this.status === 200)
	{
		console.log(this.responseText);
		var Response = JSON.parse(this.responseText);

		if(Response.status == false)
		{
			$result.text('失敗:' + Response.message);
			$result.css("color", "rgb(166 166 23)");
		}
		else
		{
			$result.text('成功設定分機號碼' + ThisExtNum + '到您的工號:' + ThisEmployeeID);
			$result.css("color", "rgb(22 45 171)");
		}
	}
}
			
function SetExtNnm(ThisEmployeeID, ThisExtNum)			
{
  var URL = BackEndURL + 'SetOperatorExtNum.php?ID=' + ThisEmployeeID + '&Ext=' + ThisExtNum;
	console.log("URL: " + URL);
	var xmlHttp = new XMLHttpRequest();
  xmlHttp.open( "GET", URL, false);
  xmlHttp.onreadystatechange = SetExtNnmResponse;
  xmlHttp.send();
}


function GetOutPortNoResponse(evt)
{	
	const para = document.querySelector('p1');
	//console.log('Complete User Login');
	//console.log(this.status);       	
	if(this.readyState === XMLHttpRequest.DONE && this.status === 200)
	{
		console.log(this.responseText);
		var Response = JSON.parse(this.responseText);

		if(Response.status == false)
		{
			para.textContent = '失敗:' + Response.message;
		}
		else if(Response.status == true)
		{
			var OutPortsByMachine = Response.numbers;
			//console.log("OutPortsByMachine : " + OutPortsByMachine);
			for(let i = 0; i < OutPortsByMachine; i++)
			{
				var VPortIndex = Response.data[i][1];
				m_MachineID[VPortIndex] = m_MachineIDIndex;
				m_OutVPortNo[VPortIndex] = VPortIndex;
				m_OutPhyPortNo[VPortIndex] = Response.data[i][2];
				//console.log("Machine VPort/VPort: " + m_MachineID[VPortIndex] + ' ' + m_OutVPortNo[VPortIndex] + '/' + m_OutPhyPortNo[VPortIndex]);
			}
			OutPortNums *= 1.0;
			OutPortsByMachine *= 1.0;
			OutPortNums = OutPortNums + OutPortsByMachine;
			console.log("OutPortNums : " + OutPortNums);
		}
	}
}

function GetOutPortNums(MachineType, MachineID)
{
	m_MachineIDIndex = MachineID;
  var xmlHttp = new XMLHttpRequest();
	const para = document.querySelector('p1');
	if(para != null)
		para.textContent = "連線中";
	console.log("MachineID: " + MachineID);
	var URL = BackEndURL + 'GetOutLinesByMachine.php?MachineType=' + MachineType + '&MachineID=' + MachineID;
	xmlHttp.open( "GET", URL, false);
	xmlHttp.onreadystatechange = GetOutPortNoResponse;
	xmlHttp.send();
}
 			
function ConvertUTC(ThisDay)
{
	var ReturnArray = [];
	var StartUTCDay; 
	var EndUTCDay;
	var StartUTCOfWeek;
	var EndUTCOfWeek;
	var StartUTCOfMonth;
	var EndUTCOfMonth;
	var StartUTCOfYear;
	var EndUTCOfYear;
	
	var ThisYear = ThisDay.getYear() + 1900;
	var ThisMonth = ThisDay.getMonth() + 1;
	var DayOfWeek = ThisDay.getDay();
	var ThisDate = ThisDay.getDate();
	if(ThisMonth / 10 < 1)
		var MonthString = '0' + ThisMonth;
	else
		var MonthString = ThisMonth;
	if(ThisDate / 10 < 1)
		var DateString = '0' + ThisDate;
	else
		var DateString = ThisDate;

	var DateValue = ThisYear + '-' + MonthString + '-' + DateString;
	console.log("DateValue: " + DateValue);
	StartUTCDay = Date.parse(DateValue + 'T00:00:00') / 1000;
	EndUTCDay = Date.parse(DateValue + 'T23:59:59') / 1000;
	StartUTCOfWeek = StartUTCDay - (86400 * DayOfWeek);
	EndUTCOfWeek = StartUTCDay + (86400 * (7 - DayOfWeek) - 1);
	var MonthString = ThisYear + '-' + MonthString + '-01T00:00:00';
	console.log("StartMonthString: "+MonthString);
	StartUTCOfMonth = Date.parse(MonthString) / 1000;
	var NextMonth = ThisMonth + 1;
	var NextYear = ThisYear;
	if(ThisMonth == 12)
	{
		NextMonth = 1;
		NextYear++;
	}
	if(NextMonth / 10 < 1)
		var MonthString = '0' + NextMonth;
	else
		var MonthString = NextMonth;
	var MonthString = NextYear + '-' + MonthString + '-01T00:00:00';
	console.log("EndMonthString: " + MonthString);
	EndUTCOfMonth = Date.parse(MonthString) / 1000 - 1;
	console.log("ThisYear: " + ThisYear);
	var MonthString = ThisYear + '-01-01T00:00:00';
	StartUTCOfYear = Date.parse(MonthString) / 1000;
	var MonthString = ThisYear + '-12-31T23:59:59';
	EndUTCOfYear = Date.parse(MonthString) / 1000;
	
	ReturnArray[0] = StartUTCDay;
	ReturnArray[1] = EndUTCDay;
	ReturnArray[2] = StartUTCOfWeek;
	ReturnArray[3] = EndUTCOfWeek;
	ReturnArray[4] = StartUTCOfMonth;
	ReturnArray[5] = EndUTCOfMonth;
	ReturnArray[6] = StartUTCOfYear;
	ReturnArray[7] = EndUTCOfYear;
	
	return ReturnArray;
}
