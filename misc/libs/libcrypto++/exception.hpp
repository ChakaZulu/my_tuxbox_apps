/*
 * exception.hpp
 *
 * Copyright (C) 2002 Bastian Blank <waldi@tuxbox.org>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: exception.hpp,v 1.2 2002/03/12 19:37:03 waldi Exp $
 */

#ifndef __LIBCRYPTO__EXCEPTION_HPP
#define __LIBCRYPTO__EXCEPTION_HPP 1

#include <stdexcept>

#include <libcrypto++/lib.hpp>

namespace Crypto
{
  namespace exception
  {
    class no_item : public std::domain_error
    {
      public:
        no_item ( const std::string & what )
        : domain_error ( what )
        { }
    };

    namespace evp
    {
      class bad_decrypt : public std::runtime_error
      {
        public:
          bad_decrypt ( const std::string & what )
          : runtime_error ( what )
          { }
      };
    };

    class undefined_libcrypto_error : public std::runtime_error
    {
      public:
        undefined_libcrypto_error ()
        : runtime_error ( Crypto::lib::get_error () )
        { Crypto::lib::clear_error (); }
    };

  };
};

#endif
