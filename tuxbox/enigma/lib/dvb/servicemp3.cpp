#ifndef DISABLE_FILE

#include <lib/dvb/servicemp3.h>
#include <config.h>
#include <lib/dvb/servicefile.h>
#include <lib/system/init.h>
#include <lib/system/init_num.h>
#include <lib/base/i18n.h>
#include <lib/codecs/codecmp3.h>
#include <lib/codecs/codecmpg.h>
#include <lib/dvb/decoder.h>

#include <unistd.h>
#include <fcntl.h>
#include <id3tag.h>
#include <linux/soundcard.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

#if HAVE_DVB_API_VERSION < 3
#include <ost/audio.h>
#else
#include <linux/dvb/audio.h>
#endif

#define VIDEO_FLUSH_BUFFER    0

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
	metaDataUpdated((const char*)metadata);
//	eDebug("processing metadata! %s", metadata);
	
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

				// otherwise there's really data.
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

eMP3Decoder::eMP3Decoder(int type, const char *filename, eServiceHandlerMP3 *handler)
: handler(handler), input(8*1024), output(256*1024),
	output2(256*1024), type(type), outputbr(0), messages(this, 1)
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
				http_status=_("Connecting...");
				filelength=-1;
				handler->messages.send(eServiceHandlerMP3::eMP3DecoderMessage(eServiceHandlerMP3::eMP3DecoderMessage::status));
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

	switch (type)
	{
	case codecMP3:
		audiodecoder=new eAudioDecoderMP3(input, output);
		break;
	case codecMPG:
		audiodecoder=new eMPEGDemux(input, output, output2);
		break;
	}
	
	if (type != codecMPG)
	{
		pcmsettings.reconfigure=1;
		dspfd[1]=-1;
	
		dspfd[0]=::open("/dev/sound/dsp", O_WRONLY|O_NONBLOCK);
		if (dspfd[0]<0)
		{
			eDebug("output failed! (%m)");
			error=errno;
			state=stateError;
		}
	
		if (dspfd[0] >= 0)
		{
			outputsn[0]=new eSocketNotifier(this, dspfd[0], eSocketNotifier::Write, 0);
			CONNECT(outputsn[0]->activated, eMP3Decoder::outputReady);
		} else
			outputsn[0]=0;
		outputsn[1]=0;
		Decoder::displayIFrameFromFile(DATADIR "/iframe");
	} else
	{
		Decoder::parms.vpid=0x1ffe;
		Decoder::parms.apid=0x1ffe;
		Decoder::parms.pcrpid=-1;
		Decoder::parms.audio_type=DECODE_AUDIO_MPEG;
		Decoder::Set();
		
		dspfd[0]=::open("/dev/video", O_WRONLY|O_NONBLOCK);
		dspfd[1]=::open("/dev/sound/dsp1", O_WRONLY|O_NONBLOCK);
		
		if ((dspfd[0]<0) || (dspfd[1]<0))
		{
			if (dspfd[0]>=0)
				::close(dspfd[0]);
			if (dspfd[1]>=0)
				::close(dspfd[1]);
			eDebug("output failed! (%m)");
			error=errno;
			state=stateError;
			outputsn[0]=0;
			outputsn[1]=0;
		} else
		{
			outputsn[0]=new eSocketNotifier(this, dspfd[0], eSocketNotifier::Write, 0);
			CONNECT(outputsn[0]->activated, eMP3Decoder::outputReady);
			outputsn[1]=new eSocketNotifier(this, dspfd[1], eSocketNotifier::Write, 0);
			CONNECT(outputsn[1]->activated, eMP3Decoder::outputReady2);
		}
	}
	
	if (sourcefd >= 0)
	{
		inputsn=new eSocketNotifier(this, sourcefd, eSocketNotifier::Read, 0);
		CONNECT(inputsn->activated, eMP3Decoder::decodeMore);
	} else
		inputsn=0;
	
	CONNECT(messages.recv_msg, eMP3Decoder::gotMessage);
	
	if (type == codecMPG)
	{
		maxOutputBufferSize=1024*1024*4;
		minOutputBufferSize=64*1024;
	} else
	{
		maxOutputBufferSize=256*1024;
		minOutputBufferSize=8*1024;
	}
	
	run();
}

		// we got (http) metadata.
void eMP3Decoder::metaDataUpdated(eString meta)
{
	{
		eLocker locker(poslock);
		eString streamTitle, streamUrl;
		if (meta.left(6) == "Stream")
			while (!meta.empty())
			{
				unsigned int eq=meta.find('=');
				if (eq == eString::npos)
						break;
				eString left=meta.left(eq);
				meta=meta.mid(eq+1); // skip until =
				eq=meta.find(';');
				if (eq == eString::npos)
					break;
				eString right=meta.left(eq);
				meta=meta.mid(eq+1);
				if (left=="StreamTitle")
					streamTitle=right;
				else if (left == "StreamUrl")
					streamUrl=right;
				else
					eDebug("unknown tag: %s = %s", left.c_str(), right.c_str());			
			}
		else
			streamTitle=meta;

		metadata[0]=streamTitle;
		metadata[1]=streamUrl;
	}

	handler->messages.send(eServiceHandlerMP3::eMP3DecoderMessage(eServiceHandlerMP3::eMP3DecoderMessage::infoUpdated));
}

int eMP3Decoder::getOutputDelay(int i)
{
	int delay = 0;
	(void)i;
	if (::ioctl(dspfd[type==codecMPG], SNDCTL_DSP_GETODELAY, &delay) < 0)
		eDebug("SNDCTL_DSP_GETODELAY failed (%m)");
	return delay;
}

int eMP3Decoder::getOutputRate(int i)
{
	(void)i;
	return pcmsettings.samplerate*pcmsettings.channels*2;
}

void eMP3Decoder::streamingDone(int err)
{
	if (err || !http || http->code != 200)
	{
		eLocker locker(poslock);
		if (err)
		{
			switch (err)
			{
			case -2:
				http_status="Can't resolve hostname!";
				break;
			case -3:
				http_status="Can't connect!";
				break;
			default:
				http_status.sprintf("unknown error %d", err);
			}
		} else if (http && (http->code!=200))
			http_status.sprintf("error: %d (%s)", http->code, http->code_descr.c_str());
		else	
			http_status="unknown";
		handler->messages.send(eServiceHandlerMP3::eMP3DecoderMessage(eServiceHandlerMP3::eMP3DecoderMessage::status));
	} else
	{
		state=stateFileEnd;
		outputsn[0]->start();
		if (outputsn[1])
			outputsn[1]->start();
		eDebug("streaming vorbei!");
	}
	http=0;
}

eHTTPDataSource *eMP3Decoder::createStreamSink(eHTTPConnection *conn)
{
	stream=new eHTTPStream(conn, input);
	CONNECT(stream->dataAvailable, eMP3Decoder::decodeMoreHTTP);
	CONNECT(stream->metaDataUpdated, eMP3Decoder::metaDataUpdated);
	http_status=_("playing...");
	handler->messages.send(eServiceHandlerMP3::eMP3DecoderMessage(eServiceHandlerMP3::eMP3DecoderMessage::status));
	return stream;
}

void eMP3Decoder::thread()
{
	messages.start();
	exec();
}

void eMP3Decoder::outputReady(int what)
{
	(void)what;
	if (type != codecMPG)
	{
		if ( ( pcmsettings.reconfigure 
				|| (pcmsettings.samplerate != audiodecoder->pcmsettings.samplerate) 
				|| (pcmsettings.channels != audiodecoder->pcmsettings.channels)))
		{
			pcmsettings=audiodecoder->pcmsettings;
			
			outputbr=pcmsettings.samplerate*pcmsettings.channels*16;
			if (::ioctl(dspfd[0], SNDCTL_DSP_SPEED, &pcmsettings.samplerate) < 0)
				eDebug("SNDCTL_DSP_SPEED failed (%m)");
			if (::ioctl(dspfd[0], SNDCTL_DSP_CHANNELS, &pcmsettings.channels) < 0)
				eDebug("SNDCTL_DSP_CHANNELS failed (%m)");
			if (::ioctl(dspfd[0], SNDCTL_DSP_SETFMT, &pcmsettings.format) < 0)
				eDebug("SNDCTL_DSP_SETFMT failed (%m)");
//			eDebug("reconfigured audio interface...");
		}
	}

	output.tofile(dspfd[0], 65536);
	checkFlow(0);
}

void eMP3Decoder::outputReady2(int what)
{
	(void)what;
	output2.tofile(dspfd[1], 65536);
	checkFlow(0);
}

void eMP3Decoder::checkFlow(int last)
{
// 	eDebug("I: %d O: %d S: %d", input.size(), output.size(), state);

	if (state == statePause)
		return;

	int i=input.size(), o[2];
	o[0]=output.size();
	if (outputsn[1])
		o[1]=output2.size();
	else
		o[1]=0;
		
//	eDebug("input: %d  video %d   audio %d", input.size(), output.size(), output2.size());
	
	// states:
	// buffering  -> output is stopped (since queue is empty)
	// playing    -> input queue (almost) empty, output queue filled
	// bufferFull -> input queue full, reading disabled, output enabled
	
	if (!o[0] || (outputsn[1] && !o[1]) )
	{
		if (state == stateFileEnd)
		{
			outputsn[0]->stop();
			if (outputsn[1])
				outputsn[1]->stop();
			eDebug("ok, everything played..");
			handler->messages.send(eServiceHandlerMP3::eMP3DecoderMessage(eServiceHandlerMP3::eMP3DecoderMessage::done));
			return;
		} else if (state != stateBuffering)
		{
//			eDebug("stateBuffering");
			state=stateBuffering;
			outputsn[0]->stop();
		}
	}

	if (outputsn[1])
	{
		if (!o[1])
			outputsn[1]->stop();
		else if (o[1] > 16384)
			outputsn[1]->start();
	}

	if ((o[0] > maxOutputBufferSize) || (o[1] > maxOutputBufferSize))
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
	
	if ((o[0] < maxOutputBufferSize) && ((!outputsn[1]) || (o[1] < maxOutputBufferSize)))
	{
		int samples=0;
		if (last || (i >= audiodecoder->getMinimumFramelength()))
			samples=audiodecoder->decodeMore(last, 16384);
		if (samples < 0)
		{
			state=stateFileEnd;
			eDebug("datei TOTAL kaputt");
		}
		
		if ((o[0] + samples) < maxOutputBufferSize)
		{
			if (state == stateBufferFull)
			{
//				eDebug("stateBufferFull -> statePlaying");
				state=statePlaying;
			}
			if (inputsn)
				inputsn->start();
			else if (http)
				http->enableRead();
		}
	}

	if ((state == stateBuffering) && (o[0] > minOutputBufferSize))
	{
//		eDebug("statePlaying...");
		state=statePlaying;
		outputsn[0]->start();
		if (outputsn[1])
			outputsn[1]->start();
	}
	
}

void eMP3Decoder::recalcPosition()
{
	eLocker l(poslock);
	if (audiodecoder->getAverageBitrate() > 0)
	{
		if (filelength != -1)
			length=filelength/(audiodecoder->getAverageBitrate()>>3);
		else
			length=-1;
		if (sourcefd > 0)
		{
			position=::lseek(sourcefd, 0, SEEK_CUR);
			position+=input.size();
			position/=(audiodecoder->getAverageBitrate()>>3);
			if (type != codecMPG)
				position += output.size() / pcmsettings.samplerate / pcmsettings.channels / 2;
			else
				position += (output.size() + output2.size()) / audiodecoder->getAverageBitrate();
		} else
			position=-1;
	} else
		length=position=-1;
}

void eMP3Decoder::dspSync()
{
	if (::ioctl(dspfd[type==codecMPG], SNDCTL_DSP_RESET) < 0)
		eDebug("SNDCTL_DSP_RESET failed (%m)");

	if (type == codecMPG)
	{
		if (::ioctl(dspfd[0], VIDEO_FLUSH_BUFFER) < 0)
			eDebug("VIDEO_FLUSH_BUFFER failed (%m)");
	}
	Decoder::flushBuffer();
}

void eMP3Decoder::decodeMoreHTTP()
{
	checkFlow(0);
}

void eMP3Decoder::decodeMore(int what)
{
	int flushbuffer=0;
	(void)what;
	
	if (state == stateFileEnd)
	{
		inputsn->stop();
		return;
	}

	while ( input.size() < audiodecoder->getMinimumFramelength() )
	{
		if (input.fromfile(sourcefd, audiodecoder->getMinimumFramelength()) < audiodecoder->getMinimumFramelength())
		{
			flushbuffer=1;
			break;
		}
	}
	
	checkFlow(flushbuffer);

	if (flushbuffer)
	{
		eDebug("end of file...");
		state=stateFileEnd;
		inputsn->stop();
		outputsn[0]->start();
		if (outputsn[1])
			outputsn[1]->start();
	}
}

eMP3Decoder::~eMP3Decoder()
{
	kill(); // wait for thread exit.

	if (inputsn)
		delete inputsn;
	if (outputsn[0])
		delete outputsn[0];
	if (outputsn[1])
		delete outputsn[1];
	if (dspfd[0] >= 0)
		close(dspfd[0]);
	if (dspfd[1] >= 0)
		close(dspfd[1]);
	if (sourcefd >= 0)
		close(sourcefd);
	if (http)
		delete http;
	delete audiodecoder;
	Decoder::SetStreamType(TYPE_PES);
	Decoder::parms.vpid=-1;
	Decoder::parms.apid=-1;
	Decoder::parms.pcrpid=-1;
	Decoder::parms.audio_type=DECODE_AUDIO_MPEG;
	Decoder::Set();
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
		dspSync();
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
			if ((state==stateBuffering) ||
				(state==stateBufferFull) ||
				(statePlaying))
			{
				inputsn->stop();
				outputsn[0]->stop();
				if (outputsn[1])
					outputsn[1]->stop();
				state=statePause;
				dspSync();
			}
		} else if (state == statePause)
		{
			inputsn->start();
			outputsn[0]->start();
			if (outputsn[1])
				outputsn[1]->start();
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
		eDebug("seek/seekreal/skip, %d", message.parm);
		int offset=0;
		if (message.type != eMP3DecoderMessage::seekreal)
		{
			int br=audiodecoder->getAverageBitrate();
			if ( br <= 0 )
				break;
			if ( type == codecMPG )
				br/=128;
			else
				br/=32;
			br*=message.parm;
			offset=input.size();
			if ( type == codecMPG )
				offset+=br/125;
			else
				offset+=br/1000;
			eDebug("skipping %d bytes (br: %d)..", offset, br);
			if (message.type == eMP3DecoderMessage::skip)
				offset+=::lseek(sourcefd, 0, SEEK_CUR);
			if (offset<0)
				offset=0;
			eDebug("so final offset is %d", offset);
		}
		else
			offset=message.parm;

		input.clear();
		if ( ::lseek(sourcefd, offset, SEEK_SET) < 0 )
			eDebug("seek error (%m)");
		dspSync();
		output.clear();
		audiodecoder->resync();

		decodeMore(0);

		break;
	}
	}
}

eString eMP3Decoder::getInfo(int id)
{
	eLocker l(poslock);
	switch (id)
	{
	case 0:
		return http_status;
	case 1:
		return metadata[0];
	case 2:
		return metadata[1];
	}
	return "";
}

int eMP3Decoder::getPosition(int real)
{
	if (sourcefd < 0)
		return -1;
	recalcPosition();
	eLocker l(poslock);
	if (real)
	{
			// our file pos
		size_t real = ::lseek(sourcefd, 0, SEEK_CUR);
			// minus bytes still in input buffer
		real -= input.size();
			// minus not yet played data in a.) output buffers and b.) dsp buffer
		long long int nyp = output.size();
		int obr = getOutputRate(0);
		
		if (obr > 0)
		{
			nyp += getOutputDelay(0);
			eDebug("ODelay: %d bytes", (int)nyp);
				// convert to input bitrate
			eDebug("%d, %d", audiodecoder->getAverageBitrate()/8, obr);
			nyp *= (long long int)audiodecoder->getAverageBitrate()/8;
			nyp /= (long long int)obr;
			eDebug("odelay: %d bytes", (int)nyp);
		} else
			nyp = 0;
		
		return real - nyp;
	}
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
	else if (message.type == eMP3DecoderMessage::status)
		serviceEvent(eServiceEvent(eServiceEvent::evtStatus));
	else if (message.type == eMP3DecoderMessage::infoUpdated)
		serviceEvent(eServiceEvent(eServiceEvent::evtInfoUpdated));
}

eService *eServiceHandlerMP3::createService(const eServiceReference &service)
{
	if ( service.descr )
		return new eServiceMP3(service.path.c_str(), service.descr.c_str() );
	return new eServiceMP3(service.path.c_str());
}

int eServiceHandlerMP3::play(const eServiceReference &service)
{
	if ( service.path )
	{
		FILE *f = fopen( service.path.c_str(), "r" );
		if (!f)
		{
			if ( service.path.find("://") == eString::npos )
			{
				eDebug("file %s not exist.. don't play", service.path.c_str() );
				return -1;
			}
		}
		else
			fclose(f);
	}
	else
		return -1;

	decoder=new eMP3Decoder(service.data[0], service.path.c_str(), this);
	decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::start));
	
	if (!decoder->getError())
		state=statePlaying;
	else
		state=stateError;

	flags=flagIsSeekable|flagSupportPosition;

	if ( service.data[0] != eMP3Decoder::codecMPG )
		flags|=flagIsTrack;

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
	case eServiceCommand::cmdSeekBegin:
	case eServiceCommand::cmdSeekEnd:
		if (ioctl(Decoder::getAudioDevice(), AUDIO_SET_MUTE, cmd.type == eServiceCommand::cmdSeekBegin) < 0)
			eDebug("AUDIO_SET_MUTE error (%m)");
		break;
	case eServiceCommand::cmdSkip:
		decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::skip, cmd.parm));
		break;
	case eServiceCommand::cmdSeekAbsolute:
		decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::seek, cmd.parm));
		break;
	case eServiceCommand::cmdSeekReal:
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
		eServiceReference ref(id, 0, filename);
		ref.data[0]=eMP3Decoder::codecMP3;
		eServiceFileHandler::getInstance()->addReference(node, eServiceReference(id, 0, filename));
	} else if ((filename.right(5).upper()==".MPEG")
		|| (filename.right(4).upper()==".MPG")
		|| (filename.right(4).upper()==".VOB")
		|| (filename.right(4).upper()==".BIN")
		|| (filename.right(4).upper()==".VDR"))
	{
		struct stat s;
		if (::stat(filename.c_str(), &s))
			return;
		eServiceReference ref(id, 0, filename);
		ref.data[0]=eMP3Decoder::codecMPG;
		eServiceFileHandler::getInstance()->addReference(node, ref);
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

int eServiceHandlerMP3::getState()
{
	return state;
}

int eServiceHandlerMP3::stop()
{
	if ( decoder )
	{
		decoder->messages.send(eMP3Decoder::eMP3DecoderMessage(eMP3Decoder::eMP3DecoderMessage::exit));
		delete decoder;
		decoder=0;
	}
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

eString eServiceHandlerMP3::getInfo(int id)
{
	if (!decoder)
		return "";
	return decoder->getInfo(id);
}

std::map<eString,eString> &eServiceID3::getID3Tags()
{
	if ( state == NOTPARSED )
	{
		id3_file *file;

		file=::id3_file_open(filename.c_str(), ID3_FILE_MODE_READONLY);
		if (!file)
			return tags;

		id3_tag *tag=id3_file_tag(file);
		if ( !tag )
		{
			state=NOTEXIST;
			id3_file_close(file);
			return tags;
		}

		struct id3_frame const *frame;
		id3_ucs4_t const *ucs4;
		id3_utf8_t *utf8;

		for (unsigned int i=0; i<tag->nframes; ++i)
		{
			frame=tag->frames[i];
			if ( !frame->nfields )
				continue;
			for ( unsigned int fr=0; fr < frame->nfields; fr++ )
			{
				union id3_field const *field;
				field    = &frame->fields[fr];
				if ( field->type != ID3_FIELD_TYPE_STRINGLIST )
					continue;

				unsigned int nstrings = id3_field_getnstrings(field);

				for (unsigned int j = 0; j < nstrings; ++j)
				{
					ucs4 = id3_field_getstrings(field, j);
					ASSERT(ucs4);

					if (strcmp(frame->id, ID3_FRAME_GENRE) == 0)
						ucs4 = id3_genre_name(ucs4);

					utf8 = id3_ucs4_utf8duplicate(ucs4);
					if (utf8 == 0)
						break;

					tags.insert(std::pair<eString,eString>(frame->id, eString((char*)utf8)));
					free(utf8);
				}
			}
		}
		id3_file_close(file);
		state=PARSED;
	}
	return tags;
}

eServiceID3::eServiceID3( const eServiceID3 &ref )
	:tags(ref.tags), filename(ref.filename), state(ref.state)
{
}

eServiceMP3::eServiceMP3(const char *filename, const char *descr)
: eService(""), id3tags(filename)
{
//	eDebug("*************** servicemp3.cpp FILENAME: %s", filename);
	if (descr)
	{
		if (!isUTF8(descr))
			service_name=convertLatin1UTF8(descr);
		else
			service_name=descr;
	}

	if (!strncmp(filename, "http://", 7))
	{
		if (!descr)
		{
			if (!isUTF8(filename))
				service_name=convertLatin1UTF8(filename);
			else
				service_name=filename;
		}
		return;
	}

	if ( !descr )
	{
		eString f=filename;
		eString l=f.mid(f.rfind('/')+1);
		if (!isUTF8(l))
			l=convertLatin1UTF8(l);
		service_name=l;
	}

	id3 = &id3tags;
}

eServiceMP3::eServiceMP3(const eServiceMP3 &c)
:eService(c), id3tags( c.id3tags )
{
	id3=&id3tags;
}

eAutoInitP0<eServiceHandlerMP3> i_eServiceHandlerMP3(eAutoInitNumbers::service+2, "eServiceHandlerMP3");

#endif //DISABLE_FILE
