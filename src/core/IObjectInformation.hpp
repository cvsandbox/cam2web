/*
    cam2web - streaming camera to web

    Copyright (C) 2017, cvsandbox, cvsandbox@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef IOBJECT_INFORMATION_HPP
#define IOBJECT_INFORMATION_HPP

#include <string>
#include <map>

#include "XError.hpp"

typedef std::map<std::string, std::string> PropertyMap;

class IObjectInformation
{
public:
    virtual ~IObjectInformation( ) { }

    virtual XError GetProperty( const std::string& propertyName, std::string& value ) const = 0;

    virtual PropertyMap GetAllProperties( ) const = 0;
};

class XObjectInformationMap : public IObjectInformation
{
public:
    XObjectInformationMap( const PropertyMap& infoMap ) : InfoMap( infoMap ) { }

    virtual XError GetProperty( const std::string& propertyName, std::string& value ) const
    {
        XError ret = XError::UnknownProperty;

        PropertyMap::const_iterator itProperty = InfoMap.find( propertyName );

        if ( itProperty != InfoMap.end( ) )
        {
            value = itProperty->second;
            ret   = XError::Success;
        }

        return ret;
    }

    virtual PropertyMap GetAllProperties( ) const
    {
        return InfoMap;
    }

private:
    PropertyMap InfoMap;
};

#endif // IOBJECT_INFORMATION_HPP
