function NewWindow(mypage, myname, w, h, scroll)
{
	var win1 = (screen.width - w) / 2;
	var wint = (screen.height - h) / 2;
	winprops = 'height='+h+', width='+w+', top='+wint+', left='+win1+', scrollbars='+scroll+', resizable'
	win = window.open(mypage, myname, winprops)
	if (parseInt(navigator.appVersion) >= 4)
	{
		win.window.focus();
	}
}

function record(xy)
{
	NewWindow('/addTimerEvent?'+xy, 'record', '200', '200', 'no');
}

function EPGDetails(xy)
{
	NewWindow('/EPGDetails?'+xy, 'EPGDetails', '600', '700', 'no');
}

function switchChannel(xy)
{
	win=window.open("?path="+xy, "switchStatus","width=1,height=1,left=0,top=0");
	win.focus();
	parent.setTimeout("reload()", 3000);
}
