/*
    cam2web - streaming camera to web

    BSD 2-Clause License

    Copyright (c) 2017, cvsandbox, cvsandbox@gmail.com
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
