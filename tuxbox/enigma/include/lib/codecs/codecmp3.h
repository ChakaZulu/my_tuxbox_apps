#ifndef __lib_codecs_codecmp3_h
#define __lib_codecs_codecmp3_h

#include <mad.h>
#include <lib/codecs/codec.h>

class eAudioDecoderMP3: public eAudioDecoder
{
	enum { INPUT_BUFFER_SIZE=8192 };
	unsigned char input_buffer[INPUT_BUFFER_SIZE];
	int avgbr; // average bitrate
	
	int framecnt;

	mad_stream stream;
	mad_frame frame;
	mad_synth synth;
	mad_timer_t timer;
	eIOBuffer &input, &output;
public:
	eAudioDecoderMP3(eIOBuffer &input, eIOBuffer &output);
	~eAudioDecoderMP3();
	
	void resync();
	int getMinimumFramelength();
	int decodeMore(int last, int maxsamples);
	int getAverageBitrate();
};

#endif
