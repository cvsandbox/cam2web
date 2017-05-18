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

#ifndef XRASPI_CAMERA_HPP
#define XRASPI_CAMERA_HPP

#include <memory>

#include "IVideoSource.hpp"
#include "XInterfaces.hpp"

namespace Private
{
    class XRaspiCameraData;
}

enum class AwbMode
{
    Off = 0,
    Auto,
    Sunlight,
    Cloudy,
    Shade,
    Tungsten,
    Fluorescent,
    Incandescent,
    Flash,
    Horizon
};

enum class ExposureMode
{
    Off = 0,
    Auto,
    Night,
    NightPreview,
    Backlight,
    Spotlight,
    Sports,
    Snow,
    Beach,
    VeryLong,
    FixedFps,
    AntiShake,
    FireWorks
};

enum class ExposureMeteringMode
{
    Average = 0,
    Spot,
    Backlit,
    Matrix
};

enum class ImageEffect
{
    None = 0,
    Negative,
    Solarize,
    Posterize,
    WhiteBoard,
    BlackBoard,
    Sketch,
    Denoise,
    Emboss,
    OilPaint,
    Hatch,
    Gpen,
    Pastel,
    WaterColor,
    Film,
    Blur,
    Saturation,
    ColorSwap,
    WashedOut,
    Posterise,
    ColorPoint,
    ColorBalance,
    Cartoon
};
    
class XRaspiCamera : public IVideoSource, private Uncopyable
{
protected:
    XRaspiCamera( );

public:
    ~XRaspiCamera( );

    static const std::shared_ptr<XRaspiCamera> Create( );

    // Start video source so it initializes and begins providing video frames
    bool Start( );
    // Signal source video to stop, so it could finalize and clean-up
    void SignalToStop( );
    // Wait till video source (its thread) stops
    void WaitForStop( );
    // Check if video source is still running
    bool IsRunning( );

    // Get number of frames received since the start of the video source
    uint32_t FramesReceived( );

    // Set video source listener returning the old one
    IVideoSourceListener* SetListener( IVideoSourceListener* listener );

public: // Different settings of the video source

    // Get/Set camera's horizontal/vertical flip
    bool GetHorizontalFlip( ) const;
    bool GetVerticalFlip( ) const;
    bool SetCameraFlip( bool horizontal, bool vertical );

    // Get/Set video stabilisation flag
    bool GetVideoStabilisation( ) const;
    bool SetVideoStabilisation( bool enabled );

    // Get/Set camera's sharpness value, [-100, 100]
    int GetSharpness( ) const;
    bool SetSharpness( int sharpness );

    // Get/Set camera's contrast value, [-100, 100]
    int GetContrast( ) const;
    bool SetContrast( int contrast );

    // Get/Set camera's brightness value, [0, 100]
    int GetBrightness( ) const;
    bool SetBrightness( int brightness );

    // Get/Set camera's saturation value, [-100, 100]
    int GetSaturation( ) const;
    bool SetSaturation( int saturation );
    
    // Get/Set camera's Automatic White Balance mode
    AwbMode GetWhiteBalanceMode( ) const;
    bool SetWhiteBalanceMode( AwbMode mode );
    
    // Get/Set camera's exposure mode
    ExposureMode GetExposureMode( ) const;
    bool SetExposureMode( ExposureMode mode );
    
    // Get/Set camera's exposure metering mode
    ExposureMeteringMode GetExposureMeteringMode( ) const;
    bool SetExposureMeteringMode( ExposureMeteringMode mode );
    
    // Get/Set camera's image effect
    ImageEffect GetImageEffect( ) const;
    bool SetImageEffect( ImageEffect effect );

private:
    Private::XRaspiCameraData* mData;
};

#endif // XRASPI_CAMERA_HPP

