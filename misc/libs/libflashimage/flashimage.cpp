/*
 * flashimage.cpp
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
 * $Id: flashimage.cpp,v 1.1 2002/05/28 18:51:27 waldi Exp $
 */

#include <flashimage.hpp>

#include <sstream>
#include <stdexcept>

#include <libcrypto++/bio.hpp>
#include <libcrypto++/x509.hpp>

FlashImage::FlashImage::FlashImage ( FlashImageFS & fs )
: fs ( fs )
{
  std::stringstream stream;
  fs.file ( "control", stream );
  control = parse_control ( stream );
  stream.clear ();

  if ( control["Format"] == "" )
    throw std::runtime_error ( std::string ( "Format: field empty" ) );
  if ( control["Format"] < "1.0" || control["Format"] > "1.0" )
    throw std::runtime_error ( std::string ( "Format: unknown: " ) + control["Format"] );

  if ( control["Date"] == "" )
    throw std::runtime_error ( std::string ( "Date: field empty" ) );

  if ( control["Version"] == "" )
    throw std::runtime_error ( std::string ( "Version: field empty" ) );

  if ( control["Status"] == "" )
    throw std::runtime_error ( std::string ( "Status: field empty" ) );

  if ( ! ( control["Status"] == "Unofficial" || control["Status"] == "Official" ) )
    throw std::runtime_error ( std::string ( "Status: unknown:" ) + control["Status"] );

  if ( control["Maintainer"] == "" )
    throw std::runtime_error ( std::string ( "Maintainer: field empty" ) );

  if ( control["Digest"] == "" )
    throw std::runtime_error ( std::string ( "Digest: field empty" ) );

  if ( ! ( control["Digest"] == "MD5" || control["Digest"] == "SHA1" || control["Digest"] == "RIPEMD160" ) )
    throw std::runtime_error ( std::string ( "Digest: unknown:" ) + control["Digest"] );
}

int FlashImage::FlashImage::verify_image ()
{
  std::stringstream stream;
  std::stringstream stream_out;
  Crypto::x509::cert cert;

  try
  {
    fs.file ( "signcert", stream );
    cert.read ( stream );
  }

  catch ( std::runtime_error & )
  {
    throw std::runtime_error ( "verification failed: no certificate" );
  }

  Crypto::evp::key::key key = cert.get_publickey ();

  int verify;

  if ( control["Digest"] == "MD5" )
    verify = fs.verify ( key, Crypto::evp::md::md5 () );
  else if ( control["Digest"] == "SHA1" )
    verify = fs.verify ( key, Crypto::evp::md::sha1 () );
  else if ( control["Digest"] == "RIPEMD160" )
    verify = fs.verify ( key, Crypto::evp::md::ripemd160 () );
  else
    throw std::runtime_error ( std::string ( "Digest: unknown: " ) + control["Digest"] );

  return verify;
}

std::map < std::string, std::string > FlashImage::FlashImage::parse_control ( std::istream & stream )
{
  std::map < std::string, std::string > fields;
  std::string field;
  std::string data;

  while ( ! stream.fail () )
  {
    std::getline ( stream, field, ':' );

    if ( field == "" )
      break;

    std::getline ( stream, data, ' ' );
    std::getline ( stream, data );

    if ( data == "" )
      break;

    fields.insert ( std::pair < std::string, std::string > ( field, data ) );
  }

  return fields;
}

#ifndef INLINE
#include <flashimage.ipp>
#endif

