#include <lib/dvb/servicemp3.h>
#include <lib/dvb/servicefile.h>
#include <lib/system/init.h>
#include <lib/base/i18n.h>
#include <lib/codecs/codecmp3.h>

#include <unistd.h>
#include <fcntl.h>
#include <id3tag.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>

/*
	note: mp3 decoding is done in ONE seperate thread with multiplexed input/
	decoding and output. The only problem arises when the ::read-call,
	encapsulated in eIOBuffer::fromfile, blocks. althought we only call
	it when data is "ready" to read (according to ::poll), this doesn't help
	since linux/posix/unix/whatever doesn't support proper read-ahead with
	::poll-notification. bad luck for us.
	
	the only way to address this problem (except using ::aio_*) is to
	use another thread. i don't like threads so if you really have a slow
	network/harddisk, it's your problem. sorry.
*/

eHTTPStream::eHTTPStream(eHTTPConnection *c, eIOBuffer &buffer): eHTTPDataSource(c), buffer(buffer)
{
#if 0
	if (c->remote_header.count("Content-Length"))
		total=atoi(c->remote_header["Content-Length"].c_str());
	else
		total=-1;
	received=0;
#endif
	eDebug("HTTP stream sink created!");
	metadatainterval=metadataleft=bytes=0;
	if (c->remote_header.count("icy-metaint"))
		metadatainterval=atoi(c->remote_header["icy-metaint"].c_str());
	eDebug("metadata interval: %d", metadatainterval);
}

eHTTPStream::~eHTTPStream()
{
	eDebug("HTTP stream sink deleted!");
}

void eHTTPStream::processMetaData()
{
	metadata[metadatapointer]=0;
	eDebug("processing metadata! %s", metadata);
	
	metadatapointer=0;
}

void eHTTPStream::haveData(void *vdata, int len)
{
	__u8 *data=(__u8*)vdata;
	
	while (len)
	{
		int valid=len;
		if (!metadataleft)
		{
				// not in metadata mode.. process mp3 data (stream to input buffer)

				// are we just at the beginning of metadata? (pointer)
			if (metadatainterval && (metadatainterval == bytes))
			{
						// enable metadata mode
				metadataleft=*data++*16;
				metadatapointer=0;
				len--;
				bytes=0;
				continue;
			} else if (metadatainterval && (metadatainterval < bytes))
				eFatal("metadatainterval < bytes");

				// otherwis there's really data.
			if (metadatainterval)
			{
					// is metadata in our buffer?
				if ((valid + bytes) > metadatainterval)
					valid=metadatainterval-bytes;
			}
			buffer.write(data, valid);
			data+=valid;
			len-=valid;
			bytes+=valid;
		} else
		{
				// metadata ... process it.
			int meta=len;
			if (meta > metadataleft)
				meta=metadataleft;
			
			memcpy(metadata+metadatapointer, data, meta);
			metadatapointer+=meta;
			data+=meta;
			len-=meta;
			metadataleft-=meta;
			
			if (!metadataleft)
				processMetaData();
		}
	}
	dataAvailable();
}

eMP3Decoder::eMP3Decoder(const char *filename, eServiceHandlerMP3 *handler): handler(handler), input(8*1024), output(256*1024), messages(this, 1)
{
	state=stateInit;
	
	http=0;
	
//	filename="http://205.188.209.193:80/stream/1003";

//	filename="http://sik1.oulu.fi:8002/";
//	filename="http://64.236.34.141:80/stream/1022";
//	filename="http://ios.h07.org:8006/";
//	filename="http://10.0.0.112/join.mp3";
	
	if (strstr(filename, "://")) // assume streaming
	{
		if (!strncmp(filename, "file://", 7))
			filename+=7;
		else
		{
			eDebug("I AM STREAMING...");
			
			http=eHTTPConnection::doRequest(filename, this, &error);
			if (!http)
			{
				streamingDone(error);
			} else
			{
				CONNECT(http->transferDone, eMP3Decoder::streamingDone);
				CONNECT(http->createDataSource, eMP3Decoder::createStreamSink);
				http->local_header["User-Agent"]="enigma-mp3/1.0.0";
				http->local_header["Icy-MetaData"]="1"; // enable ICY metadata
				http->start();
				eDebug("starting http streaming.");
			}
			filename=0;
		}
	}
	
	if (filename) // not streaming
	{
		sourcefd=::open(filename, O_RDONLY);
		if (sourcefd<0)
		{
			error=errno;
			eDebug("error opening %s", filename);
			state=stateError;
		} else
		{
			filelength=::lseek(sourcefd, 0, SEEK_END);
			lseek(sourcefd, 0, SEEK_SET);
		}
	} else
		sourcefd=-1;
	
	length=-1;

	audiodecoder=new eAudioDecoderMP3(input, output);
	pcmsettings.reconfigure=1;
	
	dspfd=::open("/dev/sound/dsp", O_WRONLY|O_NONBLOCK);
	if (dspfd<0)
	{
		eDebug("output failed! (%m)");
		error=errno;
		state=stateError;
	}
	
	if (dspfd >= 0)
	{
		outputsn=new eSocketNotifier(this, dspfd, eSocketNotifier::Write, 0);
		CONNECT(outputsn->activated, eMP3Decoder::outputReady);
	} else
		outputsn=0;
	
	if (sourcefd >= 0)
	{
		inputsn=new eSocketNotifier(this, sourcefd, eSocketNotifier::Read, 0);
		CONNECT(inputsn->activated, eMP3Decoder::decodeMore);
	} else
		inputsn=0;
	
	CONNECT(messages.recv_msg, eMP3Decoder::gotMessage);
	
	maxOutputBufferSize=256*1024;
	
	run();
}

void eMP3Decoder::streamingDone(int err)
{
	if (err || !http || http->code != 200)
	{
		eDebug("error !!!");
	} else
	{
		state=stateFileEnd;
		outputsn->start();
		eDebug("streaming vorbei!");
	}
	http=0;
}

eHTTPDataSource *eMP3Decoder::createStreamSink(eHTTPConnection *conn)
{
	stream=new eHTTPStream(conn, input);
	CONNECT(stream->dataAvailable, eMP3Decoder::decodeMoreHTTP);
	return stream;
}

void eMP3Decoder::thread()
{
	messages.start();
	exec();
}

void eMP3Decoder::outputReady(int what)
{
	if ( ( pcmsettings.reconfigure 
			|| (pcmsettings.samplerate != audiodecoder->pcmsettings.samplerate) 
			|| (pcmsettings.channels != audiodecoder->pcmsettings.channels)))
	{
		pcmsettings=audiodecoder->pcmsettings;
		
		outputbr=pcmsettings.samplerate*pcmsettings.channels*16;
		::ioctl(dspfd, SNDCTL_DSP_SPEED, &pcmsettings.samplerate);
		::ioctl(dspfd, SNDCTL_DSP_CHANNELS, &pcmsettings.channels);
		::ioctl(dspfd, SNDCTL_DSP_SETFMT, &pcmsettings.format);
		eDebug("reconfigured audio interface...");
	}
	
	output.tofile(dspfd, 65536);
	
	checkFlow(0);
}

void eMP3Decoder::checkFlow(int last)
{
//	eDebug("I: %d O: %d S: %d", input.size(), output.size(), state);
	int i=input.size(), o=output.size();
	
	// states:
	// buffering  -> output is stopped (since queue is empty)
	// playing    -> input queue (almost) empty, output queue filled
	// bufferFull -> input queue full, reading disabled, output enabled
	
	if (!o)
	{
		if (state == stateFileEnd)
		{
			outputsn->stop();
			eDebug("ok, everything played..");
			handler->messages.send(eServiceHandlerMP3::eMP3DecoderMessage(eServiceHandlerMP3::eMP3DecoderMessage::done));
			return;
		} else if (state != stateBuffering)
		{
//			eDebug("stateBuffering");
			state=stateBuffering;
			outputsn->stop();
		}
	}
	
	if ((state == stateBuffering) && o > 64*1024)
	{
//		eDebug("statePlaying...");
		state=statePlaying;
		outputsn->start();
	}
	
	if (o > maxOutputBufferSize)
	{
		if (state != stateBufferFull)
		{
//			eDebug("stateBufferFull");
			if (inputsn)
				inputsn->stop();
			else if (http)
				http->disableRead();
			state=stateBufferFull;
		}
	}
	
	if (o < maxOutputBufferSize)
	{
	
		int samples=audiodecoder->decodeMore(last, 16384);
		if (samples < 0)
		{
			state=stateFileEnd;
			eDebug("datei TOTAL kaputt");
		}
		
		if (!samples)
		{
			if (state == stateBufferFull)
			{
//				eDebug("stateBufferFull -> statePlaying");
				state=statePlaying;
			}
//			eDebug("anyway, we need input.");
			if (inputsn)
				inputsn->start();
			else if (http)
				http->enableRead();
		}
	}
}

void eMP3Decoder::dspSync()
{
	if (dspfd >= 0)
		::ioctl(dspfd, SNDCTL_DSP_RESET);
}

void eMP3Decoder::decodeMoreHTTP()
{
	checkFlow(0);

#if 0
	if (audiodecoder->getAverageBitrate() > 0)
	{
		length=filelength/(audiodecoder->getAverageBitrate()>>3);
		eLocker l(poslock);
		position=::lseek(sourcefd, 0, SEEK_CUR);
		if (position > 0 )
			position/=(audiodecoder->getAverageBitrate()>>3);
		else
			position=-1;
	} else
#endif
		length=position=-1;
}

void eMP3Decoder::decodeMore(int what)
{
	int flushbuffer=0;

	if (input.size() < audiodecoder->getMinimumFramelength())
	{
		if (input.fromfile(sourcefd, audiodecoder->getMinimumFramelength()) < audiodecoder->getMinimumFramelength())
			flushbuffer=1;
	}
	
	checkFlow(flushbuffer);

	if (audiodecoder->getAverageBitrate() > 0)
	{
		length=filelength/(audiodecoder->getAverageBitrate()>>3);
		eLocker l(poslock);
		position=::lseek(sourcefd, 0, SEEK_CUR);
		if (position > 0 )
			position/=(audiodecoder->getAverageBitrate()>>3);
		else
			position=-1;
	} else
		length=position=-1;

	if (flushbuffer)
	{
		eDebug("end of file...");
		state=stateFileEnd;
		inputsn->stop();
		outputsn->start();
	}
}

eMP3Decoder::~eMP3Decoder()
{
	kill(); // wait for thread exit.

	if (inputsn)
		delete inputsn;
	if (outputsn)
		delete outputsn;
	if (dspfd >= 0)
		close(dspfd);
	if (sourcefd >= 0)
		close(sourcefd);
	if (http)
		delete http;
	delete audiodecoder;
}

void eMP3Decoder::gotMessage(const eMP3DecoderMessage &message)
{
	switch (message.type)
	{
	case eMP3DecoderMessage::start:
		if (state == stateError)
			break;
		if (state == stateInit)
		{
			state=stateBuffering;
			if (inputsn)
				inputsn->start();
			else
				eDebug("handle streaming init");
		}
		break;
	case eMP3DecoderMessage::exit:
		eDebug("got quit message..");
		quit();
		break;
	case eMP3DecoderMessage::setSpeed:
		if (state == stateError)
			break;
		if (!inputsn)
			break;
		// speed=message.parm;
		if (message.parm == 0)
		{
			if ((state==stateBuffering) || (state==stateBufferFull) || (statePlaying))
			{
				inputsn->stop();
				outputsn->stop();
				state=statePause;
				dspSync();
			}
		} else if (state == statePause)
		{
			inputsn->start();
			outputsn->start();
//			speed=message.parm;
			state=stateBuffering;
		} else
		{
			output.clear();
			dspSync();
		}
		break;
	case eMP3DecoderMessage::seek:
	case eMP3DecoderMessage::seekreal:
	case eMP3DecoderMessage::skip:
	{
		if (state == stateError)
			break;
		if (!inputsn)
			break;
		int offset=0;
		
		if (message.type != eMP3DecoderMessage::seekreal)
		{
			int br=audiodecoder->getAverageBitrate();
			if (br <= 0)
				br=192000;
			br/=8;
		
			br*=message.parm;
			offset=input.size();
			input.clear();
			offset+=br/1000;
			eDebug("skipping %d bytes (br: %d)..", offset, br);
			if (message.type == eMP3DecoderMessage::skip)
				offset+=::lseek(sourcefd, 0, SEEK_CUR);
			if (offset<0)
				offset=0;
		} else
		{
			input.clear();
			offset=message.parm;
		}
		
		eDebug("seeking to %d", offset);
		::lseek(sourcefd, offset, SEEK_SET);
		dspSync();
		output.clear();
		audiodecoder->resync();
		
		if (state == statePlaying)
		{
			inputsn->start();
			state=stateBuffering;
		}
		
		break;
	}
	}
}

int eMP3Decoder::getPosition(int real)
{
	if (sourcefd < 0)
		return -1;
	eLocker l(poslock);
	if (real)
		return ::lseek(sourcefd, 0, SEEK_CUR)-input.size();
	return position;
}

int eMP3Decoder::getLength(int real)
{
	if (sourcefd < 0)
		return -1;
	eLocker l(poslock);
	if (real)
		return filelength;
	return length+output.size()/(outputbr/8);
}

void eServiceHandlerMP3::gotMessage(const eMP3DecoderMessage &message)
{
	if (message.type == eMP3DecoderMessage::done)
	{
		state=stateStopped;
		serviceEvent(eServiceEvent(eServiceEvent::evtEnd));
	}
}

eService *eServiceHandlerMP3::createService(const eServiceReference &service)
{
	return new eServiceMP3(service.path.c_str());
}

int eServiceHandlerMP3::play(const eServiceReference &service)
{
	decoder=new eMP3Decoder(service.path.c_str(), this);
	decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::start));
	
	if (!decoder->getError())
		state=statePlaying;
	else
		state=stateError;
	
	serviceEvent(eServiceEvent(eServiceEvent::evtStart));
	serviceEvent(eServiceEvent(eServiceEvent::evtFlagsChanged) );

	return 0;
}

int eServiceHandlerMP3::getErrorInfo()
{
	return decoder ? decoder->getError() : -1;
}

int eServiceHandlerMP3::serviceCommand(const eServiceCommand &cmd)
{
	if (!decoder)
	{
		eDebug("no decoder");
		return 0;
	}
	switch (cmd.type)
	{
	case eServiceCommand::cmdSetSpeed:
		if ((state == statePlaying) || (state == statePause) || (state == stateSkipping))
		{
			if (cmd.parm < 0)
				return -1;
			decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::setSpeed, cmd.parm));
			if (cmd.parm == 0)
				state=statePause;
			else if (cmd.parm == 1)
				state=statePlaying;
			else
				state=stateSkipping;
		} else
			return -2;
		break;
	case eServiceCommand::cmdSkip:
		decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::skip, cmd.parm));
		break;
	case eServiceCommand::cmdSeekAbsolute:
		decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::seek, cmd.parm));
		break;
	case eServiceCommand::cmdSeekReal:
		eDebug("seekreal");
		decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::seekreal, cmd.parm));
		break;
	default:
		return -1;
	}
	return 0;
}

eServiceHandlerMP3::eServiceHandlerMP3(): eServiceHandler(0x1000), messages(eApp, 0)
{
	if (eServiceInterface::getInstance()->registerHandler(id, this)<0)
		eFatal("couldn't register serviceHandler %d", id);
	CONNECT(eServiceFileHandler::getInstance()->fileHandlers, eServiceHandlerMP3::addFile);
	CONNECT(messages.recv_msg, eServiceHandlerMP3::gotMessage);
	decoder=0;
}

eServiceHandlerMP3::~eServiceHandlerMP3()
{
	eServiceInterface::getInstance()->unregisterHandler(id);
}

void eServiceHandlerMP3::addFile(void *node, const eString &filename)
{
	if (filename.left(7) == "http://")
		eServiceFileHandler::getInstance()->addReference(node, eServiceReference(id, 0, filename));
	else if (filename.right(4).upper()==".MP3")
	{
		struct stat s;
		if (::stat(filename.c_str(), &s))
			return;
		eServiceFileHandler::getInstance()->addReference(node, eServiceReference(id, 0, filename));
	}
}

eService *eServiceHandlerMP3::addRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->addRef(service);
}

void eServiceHandlerMP3::removeRef(const eServiceReference &service)
{
	return eServiceFileHandler::getInstance()->removeRef(service);
}

int eServiceHandlerMP3::getFlags()
{
	return flagIsSeekable|flagSupportPosition;
}

int eServiceHandlerMP3::getState()
{
	return state;
}

int eServiceHandlerMP3::stop()
{
	decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::exit));
	delete decoder;
	decoder=0;
	serviceEvent(eServiceEvent(eServiceEvent::evtStop));
	return 0;
}

int eServiceHandlerMP3::getPosition(int what)
{
	if (!decoder)
		return -1;
	switch (what)
	{
	case posQueryLength:
		return decoder->getLength(0);
	case posQueryCurrent:
		return decoder->getPosition(0);
	case posQueryRealLength:
		return decoder->getLength(1);
	case posQueryRealCurrent:
		return decoder->getPosition(1);
	default:
		return -1;
	}
}

eServiceMP3::eServiceMP3(const char *filename): eService("")
{
	id3_file *file;
	
	if (!strncmp(filename, "http://", 7))
	{
		service_name=filename;
		return;
	}
	
	eString f=filename;
	eString l=f.mid(f.rfind('/')+1);
	service_name=l;

	file=::id3_file_open(filename, ID3_FILE_MODE_READONLY);
	if (!file)
		return;
		
	id3=&id3tags;
	
	id3_tag *tag=id3_file_tag(file);
	if (!tag)
	{
		id3_file_close(file);
		return;
	}

	eString description="";

  struct id3_frame const *frame;
  id3_ucs4_t const *ucs4;
  id3_utf8_t *utf8;

#if 0
	struct
	{
		char const *id;
		char c;
	} const info[] = {
		{ ID3_FRAME_TITLE,  '2'},
		{ "TIT3",           's'}, 
		{ "TCOP",           'd'},
		{ "TPRO",           'p'},
		{ "TCOM",           'b'},
		{ ID3_FRAME_ARTIST, '1'},
		{ "TPE2",           'f'},
		{ "TPE3",           'c'},
 		{ "TEXT",           'l'},
		{ ID3_FRAME_ALBUM,  '3'},
		{ ID3_FRAME_YEAR,   '4'},
		{ ID3_FRAME_TRACK,  'a'},
		{ "TPUB",           'P'},
		{ ID3_FRAME_GENRE,  '6'},
 		{ "TRSN",           'S'},
		{ "TENC",           'e'}
	};

	const char *naming="[%a] [%1 - %3] %2";
	
	for (const char *c=naming; *c; ++c)
	{
		if ((*c != '%') || (*++c=='%') || !*c)
		{
			description+=*c;
			continue;
		}
		
		unsigned int i;
		
		for (i=0; i<sizeof(info)/sizeof(*info); ++i)
			if (info[i].c == *c)
				break;
		if (i == sizeof(info)/sizeof(*info))
			continue;

		union id3_field const *field;
		unsigned int nstrings, j;

		frame = id3_tag_findframe(tag, info[i].id, 0);
		if (frame == 0)
			continue;
		
		field    = &frame->fields[1];
		nstrings = id3_field_getnstrings(field);
	
		for (j = 0; j < nstrings; ++j) 
		{
			ucs4 = id3_field_getstrings(field, j);
			assert(ucs4);

			if (strcmp(info[i].id, ID3_FRAME_GENRE) == 0)
				ucs4 = id3_genre_name(ucs4);


			utf8 = id3_ucs4_utf8duplicate(ucs4);
			description+=eString((const char*)utf8);
			if (utf8 == 0)
				break;
			id3tags.tags.insert(std::pair<eString,eString>(info[i].id, eString((char*)utf8)));
			free(utf8);
		}
	}
	service_name=description;
#endif

	for (int i=0; i<tag->nframes; ++i)
	{
		union id3_field const *field;
		unsigned int nstrings, j;
		frame=tag->frames[i];
		
		field    = &frame->fields[1];
		nstrings = id3_field_getnstrings(field);
	
		for (j = 0; j < nstrings; ++j) 
		{
			ucs4 = id3_field_getstrings(field, j);
			assert(ucs4);

			if (strcmp(frame->id, ID3_FRAME_GENRE) == 0)
				ucs4 = id3_genre_name(ucs4);

			utf8 = id3_ucs4_utf8duplicate(ucs4);
			description+=eString((const char*)utf8);
			if (utf8 == 0)
				break;
			id3tags.tags.insert(std::pair<eString,eString>(frame->id, eString((char*)utf8)));
			free(utf8);
		}
	}
	
	id3_file_close(file);
}

eServiceMP3::eServiceMP3(const eServiceMP3 &c): eService(c)
{
	id3tags=c.id3tags;
	id3=&id3tags;
}

eAutoInitP0<eServiceHandlerMP3> i_eServiceHandlerMP3(7, "eServiceHandlerMP3");
