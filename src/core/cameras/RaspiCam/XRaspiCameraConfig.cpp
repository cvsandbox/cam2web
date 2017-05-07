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
#include "XRaspiCameraConfig.hpp"

using namespace std;

const static string  PROP_HFLIP               = "hflip";
const static string  PROP_VFLIP               = "vflip";
const static string  PROP_VIDEO_STABILISATION = "videostabilisation";
const static string  PROP_SHARPNESS           = "sharpness";
const static string  PROP_CONTRAST            = "contrast";
const static string  PROP_BRIGHTNESS          = "brightness";
const static string  PROP_SATURATION          = "saturation";
const static string  PROP_AWBMODE             = "awb";
const static string  PROP_EXPMODE             = "expmode";
const static string  PROP_EXPMETERINGMODE     = "expmeteringmode";
const static string  PROP_EFFECT              = "effect";

const static string* SupportedProperties[] =
{
    &PROP_HFLIP, &PROP_VFLIP, &PROP_VIDEO_STABILISATION,
    &PROP_SHARPNESS, &PROP_CONTRAST, &PROP_BRIGHTNESS, &PROP_SATURATION,
    &PROP_AWBMODE
};

const static map<string, AwbMode> SupportedAwbModes =
{
    { "Off",          AwbMode::Off          },
    { "Auto",         AwbMode::Auto         },
    { "Sunlight",     AwbMode::Sunlight     },
    { "Cloudy",       AwbMode::Cloudy       },
    { "Shade",        AwbMode::Shade        },
    { "Tungsten",     AwbMode::Tungsten     },
    { "Fluorescent",  AwbMode::Fluorescent  },
    { "Incandescent", AwbMode::Incandescent },
    { "Flash",        AwbMode::Flash        },
    { "Horizon",      AwbMode::Horizon      }
};

const static map<string, ExposureMode> SupportedExposureModes =
{
    { "Off",          ExposureMode::Off          },
    { "Auto",         ExposureMode::Auto         },
    { "Night",        ExposureMode::Night        },
    { "NightPreview", ExposureMode::NightPreview },
    { "Backlight",    ExposureMode::Backlight    },
    { "Spotlight",    ExposureMode::Spotlight    },
    { "Sports",       ExposureMode::Sports       },
    { "Snow",         ExposureMode::Snow         },
    { "Beach",        ExposureMode::Beach        },
    { "VeryLong",     ExposureMode::VeryLong     },
    { "FixedFps",     ExposureMode::FixedFps     },
    { "AntiShake",    ExposureMode::AntiShake    },
    { "FireWorks",    ExposureMode::FireWorks    }
};

const static map<string, ExposureMeteringMode> SupportedExposureMeteringModes =
{
    { "Average", ExposureMeteringMode::Average },
    { "Spot",    ExposureMeteringMode::Spot    },
    { "Backlit", ExposureMeteringMode::Backlit },
    { "Matrix",  ExposureMeteringMode::Matrix  }
};

const static map<string, ImageEffect> SupportedImageEffects =
{
    { "None",         ImageEffect::None         },
    { "Negative",     ImageEffect::Negative     },
    { "Solarize",     ImageEffect::Solarize     },
    { "Sketch",       ImageEffect::Sketch       },
    { "Denoise",      ImageEffect::Denoise      },
    { "Emboss",       ImageEffect::Emboss       },
    { "OilPaint",     ImageEffect::OilPaint     },
    { "Hatch",        ImageEffect::Hatch        },
    { "Gpen",         ImageEffect::Gpen         },
    { "Pastel",       ImageEffect::Pastel       },
    { "WaterColor",   ImageEffect::WaterColor   },
    { "Film",         ImageEffect::Film         },
    { "Blur",         ImageEffect::Blur         },
    { "Saturation",   ImageEffect::Saturation   },
    { "ColorSwap",    ImageEffect::ColorSwap    },
    { "WashedOut",    ImageEffect::WashedOut    },
    { "Posterise",    ImageEffect::Posterise    },
    { "ColorPoint",   ImageEffect::ColorPoint   },
    { "ColorBalance", ImageEffect::ColorBalance },
    { "Cartoon",      ImageEffect::Cartoon      }
};

XRaspiCameraConfig::XRaspiCameraConfig( const shared_ptr<XRaspiCamera>& camera ) :
    mCamera( camera )
{

}

// Set the specified property of a PI camera
XError XRaspiCameraConfig::SetProperty( const string& propertyName, const string& value )
{
    XError ret = XError::Success;
    int    numericValue;
    bool   propStatus = true;

    // many configuration setting are actually numeric, so scan it once
    int scannedCount = sscanf( value.c_str( ), "%d", &numericValue );

    if ( propertyName == PROP_HFLIP )
    {
        bool hflip = ( ( value == "1" ) || ( value == "true" ) );
        bool vflip = mCamera->GetVerticalFlip( );

        propStatus = mCamera->SetCameraFlip( hflip, vflip );
    }
    else if ( propertyName == PROP_VFLIP )
    {
        bool vflip = ( ( value == "1" ) || ( value == "true" ) );
        bool hflip = mCamera->GetHorizontalFlip( );

        propStatus = mCamera->SetCameraFlip( hflip, vflip );
    }
    else if ( propertyName == PROP_VIDEO_STABILISATION )
    {
        propStatus = mCamera->SetVideoStabilisation( ( value == "1" ) || ( value == "true" ) );
    }
    else if ( propertyName == PROP_SHARPNESS )
    {
        if ( scannedCount == 1 )
        {
            propStatus = mCamera->SetSharpness( numericValue );
        }
        else
        {
            ret = XError::InvalidPropertyValue;
        }
    }
    else if ( propertyName == PROP_CONTRAST )
    {
        if ( scannedCount == 1 )
        {
            propStatus = mCamera->SetContrast( numericValue );
        }
        else
        {
            ret = XError::InvalidPropertyValue;
        }
    }
    else if ( propertyName == PROP_BRIGHTNESS )
    {
        if ( scannedCount == 1 )
        {
            propStatus = mCamera->SetBrightness( numericValue );
        }
        else
        {
            ret = XError::InvalidPropertyValue;
        }    
    }
    else if ( propertyName == PROP_SATURATION )
    {
        if ( scannedCount == 1 )
        {
            propStatus = mCamera->SetSaturation( numericValue );
        }
        else
        {
            ret = XError::InvalidPropertyValue;
        }
    }
    else if ( propertyName == PROP_AWBMODE )
    {
        map<string, AwbMode>::const_iterator itMode = SupportedAwbModes.find( value );
        
        if ( itMode == SupportedAwbModes.end( ) )
        {
            ret = XError::InvalidPropertyValue;
        }
        else
        {
            propStatus = mCamera->SetWhiteBalanceMode( itMode->second );
        }
    }
    else if ( propertyName == PROP_EXPMODE )
    {
        map<string, ExposureMode>::const_iterator itMode = SupportedExposureModes.find( value );
        
        if ( itMode == SupportedExposureModes.end( ) )
        {
            ret = XError::InvalidPropertyValue;
        }
        else
        {
            propStatus = mCamera->SetExposureMode( itMode->second );
        }
    }
    else if ( propertyName == PROP_EXPMETERINGMODE )
    {
        map<string, ExposureMeteringMode>::const_iterator itMode = SupportedExposureMeteringModes.find( value );
        
        if ( itMode == SupportedExposureMeteringModes.end( ) )
        {
            ret = XError::InvalidPropertyValue;
        }
        else
        {
            propStatus = mCamera->SetExposureMeteringMode( itMode->second );
        }            
    }
    else if ( propertyName == PROP_EFFECT )
    {
        map<string, ImageEffect>::const_iterator itMode = SupportedImageEffects.find( value );
        
        if ( itMode == SupportedImageEffects.end( ) )
        {
            ret = XError::InvalidPropertyValue;
        }
        else
        {
            propStatus = mCamera->SetImageEffect( itMode->second );
        }            
    }
    else
    {
        ret = XError::UnknownProperty;
    }

    if ( ( ret == XError::Success ) && ( !propStatus ) )
    {
        ret = XError::Failed;
    }

    return ret;
}

// Get the specified property of a PI camera
XError XRaspiCameraConfig::GetProperty( const string& propertyName, string& value ) const
{
    XError ret = XError::Success;
    bool   valueIsANumber = false;
    int    numericValue;

    if ( propertyName == PROP_HFLIP )
    {
        value = ( mCamera->GetHorizontalFlip( ) ) ? "1" : "0";
    }
    else if ( propertyName == PROP_VFLIP )
    {
        value = ( mCamera->GetVerticalFlip( ) ) ? "1" : "0";
    }
    else if ( propertyName == PROP_VIDEO_STABILISATION )
    {
        value = ( mCamera->GetVideoStabilisation( ) ) ? "1" : "0";
    }
    else if ( propertyName == PROP_SHARPNESS )
    {
        numericValue   = mCamera->GetSharpness( );
        valueIsANumber = true;
    }
    else if ( propertyName == PROP_CONTRAST )
    {
        numericValue   = mCamera->GetContrast( );
        valueIsANumber = true;
    }
    else if ( propertyName == PROP_BRIGHTNESS )
    {
        numericValue   = mCamera->GetBrightness( );
        valueIsANumber = true;
    }
    else if ( propertyName == PROP_SATURATION )
    {
        numericValue   = mCamera->GetSaturation( );
        valueIsANumber = true;
    }
    else
    {
        ret = XError::UnknownProperty;
    }

    if ( valueIsANumber )
    {
        char buffer[32];

        sprintf( buffer, "%d", numericValue );
        value = buffer;
    }

    return ret;
}

// Get all supported properties of a PI camera
map<string, string> XRaspiCameraConfig::GetAllProperties( ) const
{
    map<string, string> properties;

    for ( int i = 0; i < sizeof( SupportedProperties ) / sizeof( SupportedProperties[0] ); i++ )
    {
        string value;

        if ( GetProperty( *SupportedProperties[i], value ) == XError::Success )
        {
            properties.insert( pair<string, string>( *SupportedProperties[i], value ) );
        }
    }

    return properties;
}

