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

#include "XV4LCameraConfig.hpp"

using namespace std;

#define TYPE_INT  (0)
#define TYPE_BOOL (1)

typedef struct
{
    XVideoProperty  VideoProperty;
    uint16_t        Type;
    uint16_t        Order;
    const char*     Name;
}
PropertyInformation;

const static map<string, PropertyInformation> SupportedProperties =
{
    { "brightness",  { XVideoProperty::Brightness,            TYPE_INT,   0, "Brightness"              } },
    { "contrast",    { XVideoProperty::Contrast,              TYPE_INT,   1, "Contrast"                } },
    { "saturation",  { XVideoProperty::Saturation,            TYPE_INT,   2, "Saturation"              } },
    { "hue",         { XVideoProperty::Hue,                   TYPE_INT,   3, "Hue"                     } },
    { "sharpness",   { XVideoProperty::Sharpness,             TYPE_INT,   4, "Sharpness"               } },
    { "gain",        { XVideoProperty::Gain,                  TYPE_INT,   5, "Gain"                    } },
    { "blc",         { XVideoProperty::BacklightCompensation, TYPE_INT,   6, "Back Light Compensation" } },
    { "redBalance",  { XVideoProperty::RedBalance,            TYPE_INT,   7, "Red Balance"             } },
    { "blueBalance", { XVideoProperty::BlueBalance,           TYPE_INT,   8, "Blue Balance"            } },
    { "awb",         { XVideoProperty::AutoWhiteBalance,      TYPE_BOOL,  9, "Automatic White Balance" } },
    { "hflip",       { XVideoProperty::HorizontalFlip,        TYPE_BOOL, 10, "Horizontal Flip"         } },
    { "vflip",       { XVideoProperty::VerticalFlip,          TYPE_BOOL, 11, "Vertical Flip"           } }
};

// ------------------------------------------------------------------------------------------

XV4LCameraConfig::XV4LCameraConfig( const shared_ptr<XV4LCamera>& camera ) :
    mCamera( camera )
{

}

// Set the specified property of a DirectShow video device
XError XV4LCameraConfig::SetProperty( const string& propertyName, const string& value )
{
    XError  ret       = XError::Success;
    int32_t propValue = 0;

    // assume all configuration values are numeric
    int scannedCount = sscanf( value.c_str( ), "%d", &propValue );

    if ( scannedCount != 1 )
    {
        ret = XError::InvalidPropertyValue;
    }
    else
    {
        map<string, PropertyInformation>::const_iterator itSupportedProperty = SupportedProperties.find( propertyName );

        if ( itSupportedProperty == SupportedProperties.end( ) )
        {
            ret = XError::UnknownProperty;
        }
        else
        {
            ret = mCamera->SetVideoProperty( itSupportedProperty->second.VideoProperty, propValue );
        }
    }

    return ret;
}

// Get the specified property of a DirectShow video device
XError XV4LCameraConfig::GetProperty( const string& propertyName, string& value ) const
{
    XError  ret       = XError::Success;
    int32_t propValue = 0;
    char    buffer[32];

    // find the property in the list of supported
    map<string, PropertyInformation>::const_iterator itSupportedProperty = SupportedProperties.find( propertyName );

    if ( itSupportedProperty == SupportedProperties.end( ) )
    {
        ret = XError::UnknownProperty;
    }
    else
    {
        // get the property value itself
        ret = mCamera->GetVideoProperty( itSupportedProperty->second.VideoProperty, &propValue );
    }

    if ( ret )
    {
        sprintf( buffer, "%d", propValue );
        value = buffer;
    }

    return ret;
}

// Get all supported properties of a DirectShow video device
map<string, string> XV4LCameraConfig::GetAllProperties( ) const
{
    map<string, string> properties;
    string              value;

    for ( auto property : SupportedProperties )
    {
        if ( GetProperty( property.first, value ) )
        {
            properties.insert( pair<string, string>( property.first, value ) );
        }
    }

    return properties;
}

// ------------------------------------------------------------------------------------------

XV4LCameraPropsInfo::XV4LCameraPropsInfo( const shared_ptr<XV4LCamera>& camera ) :
    mCamera( camera )
{
}

XError XV4LCameraPropsInfo::GetProperty( const std::string& propertyName, std::string& value ) const
{
    XError  ret = XError::Success;
    char    buffer[128];

    // find the property in the list of supported
    map<string, PropertyInformation>::const_iterator itSupportedProperty = SupportedProperties.find( propertyName );

    if ( itSupportedProperty == SupportedProperties.end( ) )
    {
        ret = XError::UnknownProperty;
    }
    else
    {
        int32_t min, max, step, def;

        // get property features - min/max/default/etc
        ret = mCamera->GetVideoPropertyRange( itSupportedProperty->second.VideoProperty, &min, &max, &step, &def );

        if ( ret )
        {
            if ( itSupportedProperty->second.Type == TYPE_INT )
            {
                sprintf( buffer, "{\"min\":%d,\"max\":%d,\"def\":%d,\"type\":\"int\",\"order\":%d,\"name\":\"%s\"}",
                         min, max, def, itSupportedProperty->second.Order, itSupportedProperty->second.Name );
            }
            else
            {
                sprintf( buffer, "{\"def\":%d,\"type\":\"bool\",\"order\":%d,\"name\":\"%s\"}",
                         def, itSupportedProperty->second.Order, itSupportedProperty->second.Name );
            }

            value = buffer;
        }
    }

    return ret;
}

// Get information for all supported properties of a DirectShow video device
map<string, string> XV4LCameraPropsInfo::GetAllProperties( ) const
{
    map<string, string> properties;
    string              value;

    for ( auto property : SupportedProperties )
    {
        if ( GetProperty( property.first, value ) )
        {
            properties.insert( pair<string, string>( property.first, value ) );
        }
    }

    return properties;
}

