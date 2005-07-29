function channelChange()
{
	currentChannel = document.channelselector.channel.selectedIndex;
	if (currentChannel >= 0)
	{
		var channel = document.channelselector.channel.options[currentChannel].value;
		vlcStop();
		switchChannel(channel, currentBouquet, currentChannel);
		setTimeout("vlcStart()", 1000);
	}
}
function zapChannelForward()
{
	currentChannel = currentChannel + 1;
	if (currentChannel >= channels[currentBouquet].length)
		currentChannel = 0;
	document.channelselector.channel.selectedIndex = currentChannel;
	var channel = document.channelselector.channel.options[currentChannel].value;
	switchChannel(channel, currentBouquet, currentChannel);
}
function bouquetForward()
{
	currentBouquet = currentBouquet + 1;
	if (currentBouquet >= bouquets.length)
		currentBouquet = 0;
	loadChannels(currentBouquet, 0);
	document.channelselector.bouquet.selectedIndex = currentBouquet;
	document.channelselector.channel.selectedIndex = 0;
}
function zapChannelBackward()
{
	currentChannel = currentChannel - 1;
	if (currentChannel < 0)
		currentChannel = channels[currentBouquet].length - 1;
	document.channelselector.channel.selectedIndex = currentChannel;
	var channel = document.channelselector.channel.options[currentChannel].value;
	switchChannel(channel, currentBouquet, currentChannel);
}
function bouquetBackward()
{
	currentBouquet = currentBouquet - 1;
	if (currentBouquet < 0)
		currentBouquet = bouquets.length - 1;
	loadChannels(currentBouquet, currentChannel);
	document.channelselector.bouquet.selectedIndex = currentBouquet;
	document.channelselector.channel.selectedIndex = channels[currentBouquet].length - 1;
}
function bouquetChange()
{
	var channel = -1;
	var bouquet = document.channelselector.bouquet.selectedIndex;
	if (bouquet == currentBouquet)
		channel = currentChannel;
	loadChannels(bouquet, channel);
}
function epg()
{
	var selChannel = document.channelselector.channel.selectedIndex;
	if (selChannel >= 0)
		openEPG(document.channelselector.channel.options[selChannel].value);
	else
		alert("No Channel selected");
}
function mepg()
{
	openMultiEPG(document.channelselector.bouquet.options[currentBouquet].value);
}
function loadChannels(bouquet, channel)
{
	deleteChannelOptions();
	addChannelOptions(bouquet);
	document.channelselector.channel.selectedIndex = channel;
	document.channelselector.channel.focus();
	currentBouquet = bouquet;
}
function addChannelOptions(bouquet)
{
	for (var i = 0; i < channels[bouquet].length; i++)
	{
		newOption = new Option(channels[bouquet][i], channelRefs[bouquet][i], false, true);
		document.channelselector.channel.options[document.channelselector.channel.length] = newOption;
	}
}
function deleteChannelOptions()
{
	var j = document.channelselector.channel.options.length;
	for (var i = j - 1; i >= 0; i--)
		document.channelselector.channel.options[i] = null;
}
function loadBouquets(bouquet)
{
	for (var i = 0; i < bouquets.length; i++)
	{
		newOption = new Option(bouquets[i], bouquetRefs[i], false, true);
		document.channelselector.bouquet.options[i] = newOption;
	}
	document.channelselector.bouquet.selectedIndex = bouquet;
}
function vlcStop()
{
	vlccmd.location = "http://127.0.0.1:8080/?control=stop";
	vlccmd.location = "http://127.0.0.1:8080/?control=empty";
}
function vlcStart()
{
	if (parent.data.vlcparms)
	{
		if (parent.data.vlcparms.indexOf("ffffffff") == -1)
		{
			vlccmd.location.href = "http://127.0.0.1:8080/?control=add&mrl=" + parent.data.vlcparms;
			setTimeout("vlcStartItem()", 200);
		}
		else
		{
			parent.data.location.reload();
			setTimeout("vlcStart()", 500);
		}
	}
	else
		setTimeout("vlcStart()", 200);
}
function vlcStartItem()
{
	vlccmd.location.href = "http://127.0.0.1:8080/?control=play&item=0";
	setTimeout("setStreamingServiceRef()", 200);
}
function setStreamingServiceRef()
{
	if (parent.data.serviceReference)
		document.location = "/cgi-bin/setStreamingServiceRef?sref=" + parent.data.serviceReference;
	else
		setTimeout("setStreamingServiceRef()", 200);
}

