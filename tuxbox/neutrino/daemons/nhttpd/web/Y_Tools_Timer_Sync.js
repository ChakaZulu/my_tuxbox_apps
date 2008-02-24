function wiki_url(_page)
{
	return "<a href=\"http://wiki.godofgta.de/"+_page+"\"' target=\"_blank\"><b>[Help]<\/b><\/a>"
}
/*timer_list*/
var timer_body;
function timer_list_init()
{
	timer_body=document.getElementById("timer_list");
}
function timer_list_clear()
{
	while(timer_body.childNodes.length > 0)
	{
		aChild=timer_body.firstChild;
		timer_body.removeChild(aChild);
	}

	document.getElementById("checkall").checked = true;
}
function timer_list_addRow(_body, i, alDate, alTime, stDate, stTime, channel_name, progName, origin)
{
	var mycurrent_row = y_add_row_to_table(_body, ((i % 2) ==0)?"a":"b" );
	y_add_html_cell_to_row(mycurrent_row, "settimer", '<input type="checkbox" checked="true" name="settimer">');
	y_add_text_cell_to_row(mycurrent_row, "alDate", alDate);
	y_add_text_cell_to_row(mycurrent_row, "alTime", alTime);
	y_add_text_cell_to_row(mycurrent_row, "stDate", stDate);
	y_add_text_cell_to_row(mycurrent_row, "stTime", stTime);
	y_add_html_cell_to_row(mycurrent_row, "channel_name", channel_name);
	y_add_html_cell_to_row(mycurrent_row, "progName", progName);
	y_add_text_cell_to_row(mycurrent_row, "origin", origin);
}
/*sLog*/
var sLog_body;
var sLog_line_number;
function sLog_init()
{
	sLog_line_number = 0;
	sLog_body=document.getElementById("slog_list");
}
function sLog_clear()
{
	while(sLog_body.childNodes.length > 0)
	{
		aChild=sLog_body.firstChild;
		sLog_body.removeChild(aChild);
	}
	sLog_line_number =  0;
}
function sLog_addRow(_body, state, action_text, state_text)
{
	sLog_line_number++;
	var mycurrent_row = y_add_row_to_table(_body, ((sLog_line_number % 2) ==0)?"a":"b" );
	var __img ="/images/info.png";
	switch (state)
	{
		case "green":	__img = "/images/accept.png"; break;
		case "yellow":	__img = "/images/alert.gif"; break;
		case "ok":	__img = "/images/info.png"; break;
		case "red":	__img = "/images/remove.png"; break;
	}
	y_add_html_cell_to_row(mycurrent_row, "icon", "<img src='"+__img+"'>");
	y_add_html_cell_to_row(mycurrent_row, "action_text", action_text);
	y_add_text_cell_to_row(mycurrent_row, "state_text", state_text);
}
/*request*/
function processReqChange()
{
	if (g_req.readyState == 4) {
		if (g_req.status == 200)
		{
			if (document.f.planer[0].checked == true)
			{
				var xml = g_req.responseXML;
				var recProg_NodeList = xml.getElementsByTagName('recProg');

				for(i=0;i<recProg_NodeList.length;i++)
				{
					var recProg_Node = recProg_NodeList[i];

					var sender		= recProg_Node.getElementsByTagName('sender')[0].firstChild.nodeValue;
					var progName 		= recProg_Node.getElementsByTagName('progName')[0].firstChild.nodeValue;
					var progStartDate 	= recProg_Node.getElementsByTagName('progStartDate')[0].firstChild.nodeValue;
					var progStartTime 	= recProg_Node.getElementsByTagName('progStartTime')[0].firstChild.nodeValue;
					var progEndDate 	= recProg_Node.getElementsByTagName('progEndDate')[0].firstChild.nodeValue;
					var progEndTime 	= recProg_Node.getElementsByTagName('progEndTime')[0].firstChild.nodeValue;

					progName = decodeURI(progName);
					/*convert startdate*/
					var Ausdruck = /(.*)-(.*)-(.*).*$/;
					Ausdruck.exec(progStartDate);
					var alDate = RegExp.$3+"."+RegExp.$2+"."+RegExp.$1;

					Ausdruck = /(.*):(.*):(.*).*$/;
					Ausdruck.exec(progStartTime);
					var alTime = RegExp.$1+":"+RegExp.$2;

					/*convert enddate*/
					Ausdruck = /(.*)-(.*)-(.*).*$/;
					Ausdruck.exec(progEndDate);
					var stDate = RegExp.$3+"."+RegExp.$2+"."+RegExp.$1;

					Ausdruck = /(.*):(.*):(.*).*$/;
					Ausdruck.exec(progEndTime);
					var stTime = RegExp.$1+":"+RegExp.$2;

					timer_list_addRow(timer_body, i, alDate, alTime, stDate, stTime, sender, progName, "TVInfo");
					if(document.f.debug.checked == true)
						sLog_addRow(sLog_body, "green", "- Timer "+i+": "+alDate+" "+alTime+" "+sender+" "+progName, "analyzed");
				}
				if(recProg_NodeList.length>0)
					sLog_addRow(sLog_body, "green", "Analyze "+recProg_NodeList.length+" Timers", "finished");
				else
					sLog_addRow(sLog_body, "yellow", "No Timers found", "finished");
			}
			else if (document.f.planer[1].checked == true)
			{
				var agt=navigator.userAgent.toLowerCase();
				var is_ie     = ((agt.indexOf("msie") != -1) && (agt.indexOf("opera") == -1));
				var xml = g_req.responseXML;
				var recProg_NodeList = xml.getElementsByTagName('item');
				for(i=0;i<recProg_NodeList.length;i++)
				{
					var recProg_Node = recProg_NodeList[i];

					var progName 		= recProg_Node.getElementsByTagName('title')[0].firstChild.nodeValue;
					var description 	= recProg_Node.getElementsByTagName('description')[0].firstChild.nodeValue;
					if(is_ie)
					{
						var sDate	= recProg_Node.getElementsByTagName('dc:date')[0].firstChild.nodeValue;
						var sender	= recProg_Node.getElementsByTagName('dc:subject')[0].firstChild.nodeValue;
					}
					else
					{
						var sDate	= recProg_Node.getElementsByTagName('date')[0].firstChild.nodeValue;
						var sender	= recProg_Node.getElementsByTagName('subject')[0].firstChild.nodeValue;
					}

					var tmp = progName.replace(/(.*):/,"");
					progName = tmp;
					var stDate ="";

					/*convert startdate*/
					var Ausdruck = /(.*)-(.*)-(.*)T.*$/;
					Ausdruck.exec(sDate);
					var alYear = RegExp.$1;
					var Ausdruck = /^(.*)\.(.*) (.*):(.*) Uhr -.*$/;
					Ausdruck.exec(description);
					alDate = RegExp.$1+"."+RegExp.$2+"."+alYear;
					alTime = RegExp.$3+":"+RegExp.$4;

					var Ausdruck = /Ende (.*):(.*) Uhr.*$/;
					Ausdruck.exec(description);
					stTime = RegExp.$1+":"+RegExp.$2;
					timer_list_addRow(timer_body, i, alDate, alTime, stDate, stTime, sender, progName, "Klack")
					if(document.f.debug.checked == true)
						sLog_addRow(sLog_body, "green", "- Timer "+i+": "+alDate+" "+alTime+" "+sender+" "+progName, "analyzed");
				}
				if(recProg_NodeList.length>0)
					sLog_addRow(sLog_body, "green", "Analyze "+recProg_NodeList.length+" Timers", "finished");
				else
					sLog_addRow(sLog_body, "yellow", "No Timers found", "finished");
			}
		}
	}
}
function do_analyze_tvinfo()
{
	loadXMLDoc("/tmp/tvinfo.xml", processReqChange);
}
function do_analyze_klack()
{
	loadXMLDoc("/tmp/klack.xml", processReqChange);
}
function do_get_tvinfo()
{
	sLog_addRow(sLog_body, "ok", "TVinfo: Sync Timer", "started");
	var _url =  "/control/exec?Y_Tools&timer_get_tvinfo&"+_username+"&"+_password;

	if(_username != "")
		if(_password != "")
		{
			var res = loadSyncURL(_url);
			if(res.search(/Connecting/)!=-1)
			{
				sLog_addRow(sLog_body, "green", "TVinfo: connecting "+res, "ok");
				if(res.search(/empty/)!=-1)
					sLog_addRow(sLog_body, "red", "TVinfo: Username and/or Password wrong. "+wiki_url("Neutrino:yWeb#Timer_Sync"), "failed");
				else
					sLog_addRow(sLog_body, "green", "TVinfo: Username / Password ok", "ok");
			}
			else
				sLog_addRow(sLog_body, "red", "TVinfo: connecting. "+wiki_url("Neutrino:yWeb#Timer_Sync"), "failed");
		}
		else
			sLog_addRow(sLog_body, "red", "TVinfo: no password given. "+wiki_url("Neutrino:yWeb#Timer_Sync"), "failed");
	else
		sLog_addRow(sLog_body, "red", "TVinfo: no username given. "+wiki_url("Neutrino:yWeb#Timer_Sync"), "failed");

}
function do_get_klack()
{
	sLog_addRow(sLog_body, "ok", "Klack: Sync Timer", "started");
	var _url = "/control/exec?Y_Tools&timer_get_klack";
	var res = loadSyncURL(_url);
alert(res);
	if(res.search(/Connecting/)!=-1)
		sLog_addRow(sLog_body, "green", "Klack: connecting: "+res, "ok");
	else
		sLog_addRow(sLog_body, "red", "Klack: connecting. "+wiki_url("Neutrino:yWeb#Timer_Sync"), "failed");
}
function do_clear_all()
{
	sLog_clear();
	timer_list_clear();
}
function do_get_selected()
{
	document.getElementById("wait").style.visibility="visible";
	if (document.f.planer[0].checked == true)
	{
		do_get_tvinfo();
		do_analyze_tvinfo();
	}
	else if (document.f.planer[1].checked == true)
	{
		do_get_klack();
		do_analyze_klack();
	}
	document.getElementById("wait").style.visibility="hidden";
}
function doToogleCheckboxes()
{
	var state = document.timer.checkall.checked;
	var _rows = timer_body.getElementsByTagName("tr");
	for(var i=0; i< _rows.length; i++)
	{
		var rowNode = _rows.item(i);
			rowNode.firstChild.firstChild.checked = state;
	}
}
/*set timer*/
function do_set_timer()
{
	sLog_addRow(sLog_body, "green", "Sync Timer to box", "started");
	var channel_replace = loadSyncURL("/control/exec?Y_Tools&get_synctimer_channels&" + Math.random());
	if(channel_replace.length >  0)
		sLog_addRow(sLog_body, "green", "Channel-Replace-List loaded "+channel_replace.length+" bytes", "ok");
	else
		sLog_addRow(sLog_body, "yellow", "Channel-Replace-List is empty", "notify");
	var channels=0;

	if(typ != "tv")
	{
		loadSyncURL("/control/setmode?tv");
		sLog_addRow(sLog_body, "green", "Switsch to TV-Mode", "ok");
	}

	var _rows = timer_body.getElementsByTagName("tr");
	for(var i=0; i< _rows.length; i++)
	{
		var rowNode = _rows.item(i);
		if(rowNode.firstChild.firstChild.checked == true)
		{
			var channel_name = rowNode.childNodes[5].firstChild.nodeValue;
			if(rowNode.childNodes[7].firstChild.nodeValue == "TVInfo")
			 	var Ausdruck = new RegExp("(.*);"+rowNode.childNodes[5].firstChild.nodeValue+";.*","i");
			else if(rowNode.childNodes[7].firstChild.nodeValue == "Klack")
			 	var Ausdruck = new RegExp("(.*);.*;"+rowNode.childNodes[5].firstChild.nodeValue+".*","i");
			else
				var Ausdruck ="";
			if(Ausdruck != "")
			{
				Ergebnis=Ausdruck.exec(channel_replace);
				if(Ergebnis)
					channel_name = RegExp.$1;
			}
			var _urlt = "/control/timer?action=new&alDate="+rowNode.childNodes[1].firstChild.nodeValue
				+"&alTime="+rowNode.childNodes[2].firstChild.nodeValue
				+"&stDate="+rowNode.childNodes[3].firstChild.nodeValue
				+"&stTime="+rowNode.childNodes[4].firstChild.nodeValue
				+"&channel_name="+channel_name
				+"&rec_dir="+document.f.rec_dir.value
				+"&rs=1"
				+"&update=1";
			_url = _urlt.replace(/:/gi,".");
			/*_url = encodeURI(_url);*/
			loadSyncURL(_url);
			channels++;
			if(document.f.debug.checked)
				/*sLog_addRow(sLog_body, "green", "Sync Timer to box: "+channel_name+" "+rowNode.childNodes[6].firstChild.nodeValue, "added");*/
				sLog_addRow(sLog_body, "green", "Sync Timer to box url: "+_url, "added");
		}
	}
	if(channels >  0)
		sLog_addRow(sLog_body, "green", "Sync Timer to box: "+channels+" Timers added", "finished");
	else
		sLog_addRow(sLog_body, "yellow", "Sync Timer to box: No Timers to add", "finished");
}