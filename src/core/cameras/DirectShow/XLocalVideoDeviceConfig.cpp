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

// Types of properties values
#define TYPE_INT  (0)
#define TYPE_BOOL (1)

// Kinds of video source properties
#define PROP_KIND_VIDEO  (0)
#define PROP_KIND_CAMERA (1)

typedef struct
{
    uint16_t        PropertyKind;
    int             Property;
    uint16_t        ValueType;
    uint16_t        Order;
    const char*     Name;
}
PropertyInformation;

const static map<string, PropertyInformation> SupportedProperties =
{
    { "brightness", { PROP_KIND_VIDEO,  (int) XVideoProperty::Brightness,            TYPE_INT,  0, "Brightness"              } },
    { "contrast",   { PROP_KIND_VIDEO,  (int) XVideoProperty::Contrast,              TYPE_INT,  1, "Contrast"                } },
    { "saturation", { PROP_KIND_VIDEO,  (int) XVideoProperty::Saturation,            TYPE_INT,  2, "Saturation"              } },
    { "hue",        { PROP_KIND_VIDEO,  (int) XVideoProperty::Hue,                   TYPE_INT,  3, "Hue"                     } },
    { "sharpness",  { PROP_KIND_VIDEO,  (int) XVideoProperty::Sharpness,             TYPE_INT,  4, "Sharpness"               } },
    { "gamma",      { PROP_KIND_VIDEO,  (int) XVideoProperty::Gamma,                 TYPE_INT,  5, "Gamma"                   } },
    { "exposure",   { PROP_KIND_CAMERA, (int) XCameraProperty::Exposure,             TYPE_INT,  6, "Exposure"                } },
    { "autoexp",    { PROP_KIND_CAMERA, (int) XCameraProperty::Exposure,             TYPE_BOOL, 7, "Automatic Exposure"      } },
    { "color",      { PROP_KIND_VIDEO,  (int) XVideoProperty::ColorEnable,           TYPE_BOOL, 8, "Color Image"             } },
    { "blc",        { PROP_KIND_VIDEO,  (int) XVideoProperty::BacklightCompensation, TYPE_BOOL, 9, "Back Light Compensation" } }
};

// ------------------------------------------------------------------------------------------

XLocalVideoDeviceConfig::XLocalVideoDeviceConfig( const shared_ptr<XLocalVideoDevice>& camera ) :
    mCamera( camera )
{
}

// Set the specified property of a DirectShow video device
XError XLocalVideoDeviceConfig::SetProperty( const string& propertyName, const string& value )
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
            if ( itSupportedProperty->second.PropertyKind == PROP_KIND_VIDEO )
            {
                ret = mCamera->SetVideoProperty( static_cast<XVideoProperty>( itSupportedProperty->second.Property ), propValue, false );
            }
            else
            {
                XCameraProperty cameraProperty = static_cast<XCameraProperty>( itSupportedProperty->second.Property );
                int32_t         currentPropValue;
                bool            isAuto;

                // get current property value including automatic control
                ret = mCamera->GetCameraProperty( cameraProperty, &currentPropValue, &isAuto );

                if ( ret )
                {
                    if ( itSupportedProperty->second.ValueType == TYPE_INT )
                    {
                        ret = mCamera->SetCameraProperty( cameraProperty, propValue, isAuto );
                    }
                    else
                    {
                        ret = mCamera->SetCameraProperty( cameraProperty, currentPropValue, ( propValue != 0 ) );
                    }
                }
            }
        }
    }

    return ret;
}

// Get the specified property of a DirectShow video device
XError XLocalVideoDeviceConfig::GetProperty( const string& propertyName, string& value ) const
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
        if ( itSupportedProperty->second.PropertyKind == PROP_KIND_VIDEO )
        {
            ret = mCamera->GetVideoProperty( static_cast<XVideoProperty>( itSupportedProperty->second.Property ), &propValue );
        }
        else
        {
            int32_t currentPropValue;
            bool    isAuto;

            ret = mCamera->GetCameraProperty( static_cast<XCameraProperty>( itSupportedProperty->second.Property ), &currentPropValue, &isAuto );

            if ( ret )
            {
                if ( itSupportedProperty->second.ValueType == TYPE_INT )
                {
                    propValue = currentPropValue;
                }
                else
                {
                    propValue = ( isAuto ) ? 1 : 0;
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
    }

    return properties;
}

// ------------------------------------------------------------------------------------------

XLocalVideoDevicePropsInfo::XLocalVideoDevicePropsInfo( const shared_ptr<XLocalVideoDevice>& camera ) :
    mCamera( camera )
{
}

XError XLocalVideoDevicePropsInfo::GetProperty( const std::string& propertyName, std::string& value ) const
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
        int32_t min = 0, max = 0, step = 0, default = 0;
        bool    isAutoSupported;

        // get property features - min/max/default/etc
        if ( itSupportedProperty->second.PropertyKind == PROP_KIND_VIDEO )
        {
            ret = mCamera->GetVideoPropertyRange( static_cast<XVideoProperty>( itSupportedProperty->second.Property ), &min, &max, &step, &default, &isAutoSupported );
        }
        else
        {
            if ( itSupportedProperty->second.ValueType == TYPE_INT )
            {
                ret = mCamera->GetCameraPropertyRange( static_cast< XCameraProperty >( itSupportedProperty->second.Property ), &min, &max, &step, &default, &isAutoSupported );
            }
            else
            {
                default = true;
            }
        }

        if ( ret )
        {
            if ( itSupportedProperty->second.ValueType == TYPE_INT )
            {
                sprintf( buffer, "{\"min\":%d,\"max\":%d,\"def\":%d,\"type\":\"int\",\"order\":%d,\"name\":\"%s\"}",
                         min, max, default, itSupportedProperty->second.Order, itSupportedProperty->second.Name );
            }
            else
            {
                sprintf( buffer, "{\"def\":%d,\"type\":\"bool\",\"order\":%d,\"name\":\"%s\"}",
                         default, itSupportedProperty->second.Order, itSupportedProperty->second.Name );
            }

            value = buffer;
        }
    }

    return ret;
}

// Get information for all supported properties of a DirectShow video device
map<string, string> XLocalVideoDevicePropsInfo::GetAllProperties( ) const
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
