function fertig()
{
	for (i=channels.achannels.length-1; i >= 0 ; i--) {
		if (channels.achannels.options[i].selected==true) {
			channels.achannels.options[i] = null;
		}
	}
	channels.submit();
}

function movechannels(source, destination)
{
	for (i=0; i < source.length; i++) {
		if (source.options[i].selected==true) {
			destination.options[destination.length] = new Option(source.options[i].text, source.options[i].value);
		}
	}
	for (i=source.length-1; i >= 0 ; i--) {
		if (source.options[i].selected==true) {
			source.options[i] = null;
		}
	}
}

function poschannel(box, direction)
{
	if (direction==0) {
		for (i=1; i < box.length  ; i++) {
			if (box.options[i].selected==true) {
				buffer = new Option(box.options[i].text, box.options[i].value);
				box.options[i].selected==false
				box.options[i]= new Option(box.options[i-1].text, box.options[i-1].value);
				box.options[i-1]=buffer;
				box.options[i-1].selected=true;
			}
		}
	} else {
		for (i=box.length-2; i >= 0  ; i--) {
			if (box.options[i].selected==true) {
				buffer = new Option(box.options[i].text, box.options[i].value);
				box.options[i].selected==false
				box.options[i]=new Option(box.options[i+1].text, box.options[i+1].value);
				box.options[i+1]=buffer;
				box.options[i+1].selected=true;
			}
		}
	}
}
