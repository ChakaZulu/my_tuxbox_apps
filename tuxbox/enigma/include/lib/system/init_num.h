#ifndef __lib_system_init_num_
#define __lib_system_init_num_

namespace eAutoInitNumbers
{
	enum { increment=5 };
	enum
	{
		configuration=0,
		lowlevel=configuration+increment,
		graphic=lowlevel+increment,
		skin=graphic+increment,
		rc=skin+increment,
		guiobject=rc+increment,
		actions=guiobject+increment,
		sysinfo=actions+increment,
		dvb=actions+increment,
		service=dvb+increment,
		osd=service+increment,
		wizard=osd+increment,
		main=osd+increment*5,
	};
};

#endif
