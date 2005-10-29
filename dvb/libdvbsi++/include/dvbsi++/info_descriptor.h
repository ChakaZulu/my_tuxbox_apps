/*
 * $Id: info_descriptor.h,v 1.2 2005/10/29 00:10:08 obi Exp $
 *
 * Copyright (C) 2004-2005 St�phane Est�-Gracias <sestegra@free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation.
 *
 * See the file 'COPYING' in the top level directory for details.
 */

#ifndef __info_descriptor_h__
#define __info_descriptor_h__

#include "descriptor.h"

class InfoDescriptor : public Descriptor
{
	protected:
		std::string iso639LanguageCode;
		std::string info;

	public:
		InfoDescriptor(const uint8_t * const buffer);

		const std::string &getIso639LanguageCode(void) const;
		const std::string &getInfo(void) const;
};

#endif /* __info_descriptor_h__ */
