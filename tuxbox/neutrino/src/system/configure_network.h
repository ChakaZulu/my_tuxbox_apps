#ifndef __configure_network_h__
#define __configure_network_h__

/*
 * $Header: /cvs/tuxbox/apps/tuxbox/neutrino/src/system/configure_network.h,v 1.1 2003/03/05 02:20:48 thegoodguy Exp $
 *
 * (C) 2003 by thegoodguy <thegoodguy@berlios.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <string>

class CNetworkConfig
{
 private:
	std::string orig_address;
	std::string orig_netmask;
	std::string orig_broadcast;
	std::string orig_gateway;
	std::string orig_nameserver;
	bool        orig_inet_static;

	void copy_to_orig(void);
	bool modified_from_orig(void);

 public:
	std::string address;
	std::string netmask;
	std::string broadcast;
	std::string gateway;
	std::string nameserver;
	bool        inet_static;

	CNetworkConfig(void);

	void commitConfig(void);
};

#endif /* __configure_network_h__ */
