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

#include <map>
#include <list>
#include <algorithm>

#include "XLocalVideoDeviceConfig.hpp"

using namespace std;

// map of supported property names
const static map<string, XVideoProperty> SupportedProperties =
{
    { "brightness", XVideoProperty::Brightness            },
    { "contrast",   XVideoProperty::Contrast              },
    { "hue",        XVideoProperty::Hue                   },
    { "saturation", XVideoProperty::Saturation            },
    { "sharpness",  XVideoProperty::Sharpness             },
    { "gamma",      XVideoProperty::Gamma                 },
    { "color",      XVideoProperty::ColorEnable           },
    { "blc",        XVideoProperty::BacklightCompensation }
};

// available subproperties
const static char* STR_MIN = "min";
const static char* STR_MAX = "max";
const static char* STR_DEF = "def";

const static list<string> SupportedSubproperties = { STR_MIN, STR_MAX, STR_DEF };

// ------------------------------------------------------------------------------------------

XLocalVideoDeviceConfig::XLocalVideoDeviceConfig( const shared_ptr<XLocalVideoDevice>& camera ) :
    mCamera( camera )
{

}

// Set the specified property of a DirectShow video device
XError XLocalVideoDeviceConfig::SetProperty( const string& propertyName, const string& value )
{
    XError  ret              = XError::Success;
    string  basePropertyName = propertyName;
    string  subPropertyName;
    int32_t propValue        = 0;

    // assume all configuration values are numeric
    int scannedCount = sscanf( value.c_str( ), "%d", &propValue );

    if ( scannedCount != 1 )
    {
        ret = XError::InvalidPropertyValue;
    }
    else
    {
        string::size_type delimiterPos = basePropertyName.find( ':' );

        if ( delimiterPos != string::npos )
        {
            subPropertyName = basePropertyName.substr( delimiterPos + 1 );
            basePropertyName = basePropertyName.substr( 0, delimiterPos );
        }

        map<string, XVideoProperty>::const_iterator itSupportedProperty = SupportedProperties.find( basePropertyName );

        if ( itSupportedProperty == SupportedProperties.end( ) )
        {
            ret = XError::UnknownProperty;
        }
        else
        {
            if ( subPropertyName.empty( ) )
            {
                ret = mCamera->SetVideoProperty( itSupportedProperty->second, propValue, false );
            }
            else
            {
                if ( std::find( SupportedSubproperties.begin( ), SupportedSubproperties.end( ), subPropertyName ) == SupportedSubproperties.end( ) )
                {
                    ret = XError::UnknownProperty;
                }
                else
                {
                    ret = XError::ReadOnlyProperty;
                }
            }
        }
    }

    return ret;
}

// Get the specified property of a DirectShow video device
XError XLocalVideoDeviceConfig::GetProperty( const string& propertyName, string& value ) const
{
    XError  ret              = XError::Success;
    string  basePropertyName = propertyName;
    string  subPropertyName;
    int32_t propValue        = 0;
    char    buffer[32];

    string::size_type delimiterPos = basePropertyName.find( ':' );

    if ( delimiterPos != string::npos )
    {
        subPropertyName = basePropertyName.substr( delimiterPos + 1 );
        basePropertyName = basePropertyName.substr( 0, delimiterPos );
    }

    // find the property in the list of supported
    map<string, XVideoProperty>::const_iterator itSupportedProperty = SupportedProperties.find( basePropertyName );

    if ( itSupportedProperty == SupportedProperties.end( ) )
    {
        ret = XError::UnknownProperty;
    }
    else
    {
        if ( subPropertyName.empty( ) )
        {
            // get the property value itself
            ret = mCamera->GetVideoProperty( itSupportedProperty->second, &propValue );
        }
        else
        {
            int32_t min, max, step, default;
            bool    isAutoSupported;

            // get property features - min/max/default/etc
            ret = mCamera->GetVideoPropertyRange( itSupportedProperty->second, &min, &max, &step, &default, &isAutoSupported );

            if ( ret )
            {
                if ( subPropertyName == STR_MIN )
                {
                    propValue = min;
                }
                else if ( subPropertyName == STR_MAX )
                {
                    propValue = max;
                }
                else if ( subPropertyName == STR_DEF )
                {
                    propValue = default;
                }
                else
                {
                    ret = XError::UnknownProperty;
                }
            }
        }
    }

    if ( ret )
    {
        sprintf( buffer, "%d", propValue );
        value = buffer;
    }

    return ret;
}

// Get all supported properties of a DirectShow video device
map<string, string> XLocalVideoDeviceConfig::GetAllProperties( ) const
{
    map<string, string> properties;
    string              value;

    for ( auto property : SupportedProperties )
    {
        if ( GetProperty( property.first, value ) )
        {
            properties.insert( pair<string, string>( property.first, value ) );
        }

        // get its subproperties as well
        for ( auto subproperty : SupportedSubproperties )
        {
            string subpropertyName = property.first + ":" + subproperty;

            if ( GetProperty( subpropertyName, value ) )
            {
                properties.insert( pair<string, string>( subpropertyName, value ) );
            }
        }
    }

    return properties;
}
