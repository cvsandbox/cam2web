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

