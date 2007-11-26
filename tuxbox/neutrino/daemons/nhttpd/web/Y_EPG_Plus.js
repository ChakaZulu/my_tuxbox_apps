var g_width_px=650;
var g_width_min=135;
var g_delta_min=5;
var g_bar_delta_min=15;
var _delta = Math.round(g_width_px * g_delta_min / g_width_min);
var epg_data;
var epg_data_index=0;

function epg_zapto()
{
	dbox_zapto(document.getElementById("d_channel_id").text);
}
function epg_set_timer()
{
	dbox_set_timer(document.getElementById("d_channel_id").text, document.getElementById("d_start").text, document.getElementById("d_stop").text);
}
function build_epg_clear()
{
	var ep = document.getElementById("epg_plus");
	obj_clear_all_childs(ep);
}
function build_epg_setbox(_item, _starttime, _stoptime, _start, _stop)
{
	var d_start = Math.max(_start, _starttime);
	var d_stop = Math.min(_stop, _stoptime);
	var d_left = 103+ Math.round((d_start-_starttime) * _delta / 60 / g_delta_min);
	var d_width = Math.max(0,Math.round((d_stop-d_start) * _delta / 60 / g_delta_min)-3);
	_item.style.cssText = "position:absolute; top:0px; left:"+d_left+"px; width:"+d_width+"px;";
}
function show_epg_item(_index)
{
	show_obj("epg_info",true);
	document.getElementById("d_desc").innerHTML = epg_data[_index][4]+" "+epg_data[_index][0];
	document.getElementById("d_info1").innerHTML = epg_data[_index][1];
	document.getElementById("d_info2").innerHTML = epg_data[_index][2];
	document.getElementById("d_start").text = epg_data[_index][3];
	document.getElementById("d_stop").text = epg_data[_index][5];
	document.getElementById("d_channel_id").text = epg_data[_index][6];
}
function build_epg_bouquet(__bdiv, __channel_id, _starttime, _stoptime)
{
	var xml = loadSyncURLxml("/control/epg?xml=true&channelid="+__channel_id+"}&details=true&stoptime="+_stoptime);
	if(xml){
		var prog_list = xml.getElementsByTagName('prog');
		for(var i=0;i<prog_list.length;i++){
			var prog = prog_list[i];

			var _stop = getXMLNodeItemValue(prog, "stop_sec");
			_stop = parseInt(_stop);
			if(_stop > _starttime){
				var _start_t	= getXMLNodeItemValue(prog, "start_t");
				var _start	= getXMLNodeItemValue(prog, "start_sec");
				_start = parseInt(_start);
				var _stop_t	= getXMLNodeItemValue(prog, "stop_t");
				var _desc	= epg_de_qout(getXMLNodeItemValue(prog, "description"));
				var _info1	= epg_de_qout(getXMLNodeItemValue(prog, "info1"));
				var _info2	= epg_de_qout(getXMLNodeItemValue(prog, "info2"));
				var __item = obj_createAt(__bdiv, "div", "ep_bouquet_item");

				var epg_obj= new Array(_desc, _info1, _info2, _start, _start_t, _stop.toString(), __channel_id);
				epg_data.push(epg_obj);
				__item.innerHTML = "<span onclick=\"show_epg_item('"+epg_data_index+"');\" title=\""+_start_t+" "+_desc+" (click for details)\">"+_desc+"</span>";
				build_epg_setbox(__item, _starttime, _stoptime, _start, _stop);
				epg_data_index++;
			}
		}
	}
}
function build_epg_time_bar(_tdiv, _starttime, _stoptime)
{
	var __w_step = Math.round(g_width_px * g_bar_delta_min / g_width_min);
	var __steps = Math.round(g_width_px  / __w_step)+1;
	var _start = _starttime;
	for(var i=0;i<__steps;i++){
		var __item = obj_createAt(_tdiv, "div", "ep_time_bar_item");
		__item.innerHTML = format_time(new Date(_start*1000));
		var _stop = _start + (g_bar_delta_min * 60);
		build_epg_setbox(__item, _starttime, _stoptime, _start, _stop);
		_start = _stop;
	}
}
var g_i = 0;
var g_bouquet_list;
function build_epg_plus(_bouquet, _starttime)
{
	build_epg_clear();
	epg_data = new Array();
	epg_data_index=0;
	var _bouquets_xml = loadSyncURLxml("/control/getbouquet?bouquet="+_bouquet+"&xml=true");
	if(_bouquets_xml){
		g_bouquet_list = _bouquets_xml.getElementsByTagName("channel");
		var ep = document.getElementById("epg_plus");
		var _stoptime = _starttime + g_width_min * 60;
		var __tdiv = obj_createAt(ep, "div", "ep_time_bar");
		var __tname_div = obj_createAt(__tdiv, "div", "ep_time_bar_item");
		__tname_div.innerHTML = "Uhrzeit";
		build_epg_time_bar(__tdiv, _starttime, _stoptime);
		g_i=0;
		window.setTimeout("build_epg_plus_loop("+_starttime+","+_stoptime+")",100);
	}
}
function build_epg_plus_loop(_starttime, _stoptime)
{
	if(g_i<g_bouquet_list.length){
		var _bouquet = g_bouquet_list[g_i];
		var __channel_name = getXMLNodeItemValue(_bouquet, "name");
		var __channel_id = getXMLNodeItemValue(_bouquet, "id");
		var ep = document.getElementById("epg_plus");
		var __bdiv = obj_createAt(ep, "div", "ep_bouquet");
		var __bname_div = obj_createAt(__bdiv, "div", "ep_bouquet_name");

		__bname_div.innerHTML = __channel_name;
		build_epg_bouquet(__bdiv, __channel_id, _starttime, _stoptime);
		window.setTimeout("build_epg_plus_loop("+_starttime+","+_stoptime+")",100);
		g_i++;
	}
	else{
		show_waitbox(false);
		obj_disable("btGet", false);
	}
}
function build_epg_plus_main()
{
	show_obj("epg_info",false);
	show_waitbox(true);
	obj_disable("btGet", true);
	var sel=document.e.bouquets.selectedIndex;
	if(sel != -1)
		bou = document.e.bouquets[sel].value;
	else
		bou = 1;
	_secs=document.e.epg_time.value;
	_secs=parseInt(_secs);
	build_epg_plus(bou, _secs);
	/*document.getElementById("epg_plus").width = g_width_px;*/
}
function build_epg_plus_delta(_delta)
{
	if(document.e.epg_time.selectedIndex + _delta < document.e.epg_time.length && document.e.epg_time.selectedIndex + _delta >= 0)
		document.e.epg_time.selectedIndex += _delta;
	build_epg_plus_main();
}
function build_time_list(_delta)
{
	var now = new Date();
	now.setMinutes(0);
	now.setSeconds(0);
	now.setMilliseconds(0);
	now = new Date(now.getTime()+_delta*60*60*1000);
	var _secs = now/1000;
	var _hour = now.getHours();
	var et = document.getElementById("epg_time");
	for(i=0;i<24;i++){
		var _time = (_hour + i) % 24;
		if(_time < 10)
			_time = "0"+_time;
		_time += ":00";
		var _time_t = _secs + i * 3600;
		var __item = obj_createAt(et, "option", "ep_bouquet_item");
		__item.text = _time;
		__item.value = _time_t;
	}
}
