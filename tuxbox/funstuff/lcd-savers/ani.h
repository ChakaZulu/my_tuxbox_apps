#ifndef __ani__
#define __ani__

struct ani_header {
	unsigned char magic[4];		// = "LCDA"
	unsigned short format;		// Format
	unsigned short width;		// Breite
	unsigned short height;		// H�he
	unsigned short count;		// Anzahl Einzelbilder
	unsigned long delay;		// �s zwischen Einzelbildern
}__attribute((packed));


#endif // __ani__
