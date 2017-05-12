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
