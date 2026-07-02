$(function () {
	//找到表格中除瞭第一個tr以外的所有偶數行
	//使用even為瞭通過tbody tr返回所有tr元素
	$("tbody tr:even").css("background-color", "#ece9d8");
	//找到所有的學號單元格
	var numId = $("tbody td");
	//給單元格註冊鼠標點擊事件
	numId.click(function () {
		//找到對應當前鼠標點擊的td，this對應的就是響應瞭click的那個td
		var tdObj = $(this);
		//判斷td中是否有文本框
		console.log("debug: " + tdObj.data() );
		if (tdObj.children("input").length>0) {
			return false;
		}
		//獲取表格中的內容
		var text = tdObj.html();
		//清空td中的內容
		tdObj.html("");
		//創建文本框
		//去掉文本框的邊框
		//設置文本框中字體與表格中的文字大小相同。
		//設置文本框的背景顏色與表格的背景顏色一樣
		//是文本框的寬度和td的寬度相同
		//並將td中值放入文本框中
		//將文本框插入到td中
		var inputObj = $("<input type='text'>").css("border-width", "0").css("font-size", tdObj.css("font-size")).css("background-color", tdObj.css("background-color")).width(tdObj.width()).val(text).appendTo(tdObj);
		//文本框插入後先獲得焦點、後選中
		inputObj.trigger("focus").trigger("select")
		//文本框插入後不能被觸發單擊事件
		inputObj.click(function () {
			return false;
		});
		//處理文本框上回車和esc按鍵的操作
		inputObj.keyup(function (event) {
			//獲取當前按下鍵盤的鍵值
			var keycode = event.which;
			console.log("KeyCode: " + keycode);
			//處理回車的情況
			if (keycode==13) {
				//獲取當前文本框中的內容
				var inputtext = $(this).val();
				//將td中的內容修改為文本框的內容
				tdObj.html(inputtext);
			}
			//處理esc的內容
			if (keycode==27) {
				//將td中的內容還原成原來的內容
				tdObj.html(text);
			}
		});
	});
});