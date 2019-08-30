/*
    cam2web - streaming camera to web

    Copyright (C) 2017-2019, cvsandbox, cvsandbox@gmail.com

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

#include <mutex>
#include <thread>

#include <bcm_host.h>
#include <interface/vcos/vcos.h>
#include <interface/mmal/mmal.h>
#include <interface/mmal/mmal_logging.h>
#include <interface/mmal/mmal_buffer.h>
#include <interface/mmal/util/mmal_util.h>
#include <interface/mmal/util/mmal_util_params.h>
#include <interface/mmal/util/mmal_default_components.h>
#include <interface/mmal/util/mmal_connection.h>

#include "XRaspiCamera.hpp"
#include "XManualResetEvent.hpp"

using namespace std;

namespace Private
{
    #define CAMERA_VIDEO_PORT   (1)
    #define BUFFER_COUNT        (2)

    // Private details of the implementation
    class XRaspiCameraData
    {
    private:
        recursive_mutex         Sync;
        recursive_mutex         ConfigSync;
        thread                  ControlThread;
        XManualResetEvent       NeedToStop;
        IVideoSourceListener*   Listener;
        bool                    Running;

        MMAL_COMPONENT_T*       Camera;
        MMAL_COMPONENT_T*       JpegEncoder;
        MMAL_CONNECTION_T*      JpegEncoderConnection;
        MMAL_PORT_T*            VideoPort;
        MMAL_PORT_T*            VideoBufferPort;
        MMAL_POOL_T*            VideoBufferPool;

        static bool             HostInitDone;

    public:
        uint32_t                FramesReceived;
        uint32_t                FrameWidth;
        uint32_t                FrameHeight;
        uint32_t                FrameRate;
        uint32_t                JpegQuality;
        bool                    JpegEncoding;
        bool                    HorizontalFlip;
        bool                    VerticalFlip;
        bool                    VideoStabilisation;
        uint32_t                ImageRotation;
        int32_t                 Sharpness;
        int32_t                 Contrast;
        int32_t                 Brightness;
        int32_t                 Saturation;
        AwbMode                 WhiteBalanceMode;
        ExposureMode            CameraExposureMode;
        ExposureMeteringMode    CameraExposureMeteringMode;
        ImageEffect             CameraImageEffect;
        string                  TextAnnotation;
        bool                    TextBlackBackground;

    public:
        XRaspiCameraData( ) :
            Sync( ), ConfigSync( ), ControlThread( ), NeedToStop( ), Listener( nullptr ), Running( false ),
            Camera( nullptr ), JpegEncoder( nullptr ), JpegEncoderConnection( nullptr ),
            VideoPort( nullptr ), VideoBufferPort( nullptr ), VideoBufferPool( nullptr ),
            FramesReceived( 0 ),
            FrameWidth( 640 ), FrameHeight( 480 ), FrameRate( 30 ), JpegQuality( 10 ), JpegEncoding( true ),
            HorizontalFlip( false ), VerticalFlip( false ), VideoStabilisation( false ), ImageRotation( 0 ),
            Sharpness( 0 ), Contrast( 0 ), Brightness( 50 ), Saturation( 0 ),
            WhiteBalanceMode( AwbMode::Auto ), CameraExposureMode( ExposureMode::Auto ),
            CameraExposureMeteringMode( ExposureMeteringMode::Average ),
            CameraImageEffect( ImageEffect::None ),
            TextAnnotation( ), TextBlackBackground( true )
        {
        }

        bool Start( );
        void SignalToStop( );
        void WaitForStop( );
        bool IsRunning( );
        IVideoSourceListener* SetListener( IVideoSourceListener* listener );

        void NotifyNewImage( const std::shared_ptr<const XImage>& image );
        void NotifyError( const string& errorMessage, bool fatal = false );

        bool Init( );
        void Cleanup( );

        void SetVideoSize( uint32_t width, uint32_t height );
        void SetFrameRate( uint32_t frameRate );
        void EnableJpegEncoding( bool enable );
        void SetJpegQuality( uint32_t jpegQuality );

        bool SetCameraFlip( bool horizontal, bool vertical );
        bool SetImageRotation( uint32_t rotation );
        bool SetVideoStabilisation( bool enabled );
        bool SetSharpness( int32_t sharpness );
        bool SetContrast( int32_t contrast );
        bool SetBrightness( int32_t brightness );
        bool SetSaturation( int32_t saturation );
        bool SetWhiteBalanceMode( AwbMode mode );
        bool SetExposureMode( ExposureMode mode );
        bool SetExposureMeteringMode( ExposureMeteringMode mode );
        bool SetImageEffect( ImageEffect effect );
        bool SetTextTextAnnotation( const string& text, bool blackBackground );

        static void ControlThreadHanlder( XRaspiCameraData* me );
        static void CameraControlCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer );
        static void VideoBufferCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer );
    };

    bool XRaspiCameraData::HostInitDone = false;
}

const shared_ptr<XRaspiCamera> XRaspiCamera::Create( )
{
    return shared_ptr<XRaspiCamera>( new XRaspiCamera );
}

XRaspiCamera::XRaspiCamera( ) :
    mData( new Private::XRaspiCameraData( ) )
{
}

XRaspiCamera::~XRaspiCamera( )
{
    delete mData;
}

// Start the video source
bool XRaspiCamera::Start( )
{
    return mData->Start( );
}

// Signal video source to stop
void XRaspiCamera::SignalToStop( )
{
    mData->SignalToStop( );
}

// Wait till video source stops
void XRaspiCamera::WaitForStop( )
{
    mData->WaitForStop( );
}

// Check if video source is still running
bool XRaspiCamera::IsRunning( )
{
    return mData->IsRunning( );
}

// Get number of frames received since the start of the video source
uint32_t XRaspiCamera::FramesReceived( )
{
    return mData->FramesReceived;
}

// Set video source listener
IVideoSourceListener* XRaspiCamera::SetListener( IVideoSourceListener* listener )
{
    return mData->SetListener( listener );
}

// Get/Set video size
uint32_t XRaspiCamera::Width( ) const
{
    return mData->FrameWidth;
}
uint32_t XRaspiCamera::Height( ) const
{
    return mData->FrameHeight;
}
void XRaspiCamera::SetVideoSize( uint32_t width, uint32_t height )
{
    mData->SetVideoSize( width, height );
}

// Get/Set frame rate
uint32_t XRaspiCamera::FrameRate( ) const
{
    return mData->FrameRate;
}
void XRaspiCamera::SetFrameRate( uint32_t frameRate )
{
    mData->SetFrameRate( frameRate );
}

// Enable/Disable JPEG encoding
bool XRaspiCamera::IsJpegEncodingEnabled( ) const
{
    return mData->JpegEncoding;
}
void XRaspiCamera::EnableJpegEncoding( bool enable )
{
    mData->EnableJpegEncoding( enable );
}

// Get/Set JPEG quality
uint32_t XRaspiCamera::JpegQuality( ) const
{
    return mData->JpegQuality;
}
void XRaspiCamera::SetJpegQuality( uint32_t jpegQuality )
{
    mData->SetJpegQuality( jpegQuality );
}

// Get/Set camera's horizontal/vertical flip
bool XRaspiCamera::GetHorizontalFlip( ) const
{
    return mData->HorizontalFlip;
}
bool XRaspiCamera::GetVerticalFlip( ) const
{
    return mData->VerticalFlip;
}
bool XRaspiCamera::SetCameraFlip( bool horizontal, bool vertical )
{
    return mData->SetCameraFlip( horizontal, vertical );
}

// Get/Set camera's image rotation, (0, 90, 180, 270)
uint32_t XRaspiCamera::GetImageRotation( ) const
{
    return mData->ImageRotation;
}
bool XRaspiCamera::SetImageRotation( uint32_t rotation )
{
    return mData->SetImageRotation( rotation );
}

// Get/Set video stabilisation flag
bool XRaspiCamera::GetVideoStabilisation( ) const
{
    return mData->VideoStabilisation;
}
bool XRaspiCamera::SetVideoStabilisation( bool enabled )
{
    return mData->SetVideoStabilisation( enabled );
}

// Get/Set camera's sharpness value, [-100, 100]
int32_t XRaspiCamera::GetSharpness( ) const
{
    return mData->Sharpness;
}
bool XRaspiCamera::SetSharpness( int32_t sharpness )
{
    return mData->SetSharpness( sharpness );
}

// Get/Set camera's contrast value, [-100, 100]
int32_t XRaspiCamera::GetContrast( ) const
{
    return mData->Contrast;
}
bool XRaspiCamera::SetContrast( int32_t contrast )
{
    return mData->SetContrast( contrast );
}

// Get/Set camera's brightness value, [0, 100]
int32_t XRaspiCamera::GetBrightness( ) const
{
    return mData->Brightness;
}
bool XRaspiCamera::SetBrightness( int32_t brightness )
{
    return mData->SetBrightness( brightness );
}

// Get/Set camera's saturation value, [-100, 100]
int32_t XRaspiCamera::GetSaturation( ) const
{
    return mData->Saturation;
}
bool XRaspiCamera::SetSaturation( int32_t saturation )
{
    return mData->SetSaturation( saturation );
}

// Get/Set Automatic White Balance mode
AwbMode XRaspiCamera::GetWhiteBalanceMode( ) const
{
    return mData->WhiteBalanceMode;
}
bool XRaspiCamera::SetWhiteBalanceMode( AwbMode mode )
{
    return mData->SetWhiteBalanceMode( mode );
}

// Get/Set exposure mode
ExposureMode XRaspiCamera::GetExposureMode( ) const
{
    return mData->CameraExposureMode;
}
bool XRaspiCamera::SetExposureMode( ExposureMode mode )
{
    return mData->SetExposureMode( mode );
}

// Get/Set camera's exposure metering mode
ExposureMeteringMode XRaspiCamera::GetExposureMeteringMode( ) const
{
    return mData->CameraExposureMeteringMode;
}
bool XRaspiCamera::SetExposureMeteringMode( ExposureMeteringMode mode )
{
    return mData->SetExposureMeteringMode( mode );
}

// Get/Set camera's image effect
ImageEffect XRaspiCamera::GetImageEffect( ) const
{
    return mData->CameraImageEffect;
}
bool XRaspiCamera::SetImageEffect( ImageEffect effect )
{
    return mData->SetImageEffect( effect );
}

// Get/Set text annotation
string XRaspiCamera::TextAnnotation( ) const
{
    return mData->TextAnnotation;
}
bool XRaspiCamera::SetTextTextAnnotation( const string& text, bool blackBackground )
{
    return mData->SetTextTextAnnotation( text, blackBackground );
}
bool XRaspiCamera::ClearTextTextAnnotation( )
{
    return mData->SetTextTextAnnotation( string( ), true );
}

namespace Private
{

// Start video source so it initializes and begins providing video frames
bool XRaspiCameraData::Start( )
{
    lock_guard<recursive_mutex> lock( Sync );

    if ( !IsRunning( ) )
    {
        NeedToStop.Reset( );
        Running = true;
        FramesReceived = 0;

        ControlThread = thread( ControlThreadHanlder, this );
    }

    return true;
}

// Signal video to stop, so it could finalize and clean-up
void XRaspiCameraData::SignalToStop( )
{
    lock_guard<recursive_mutex> lock( Sync );

    if ( IsRunning( ) )
    {
        NeedToStop.Signal( );
    }
}

// Wait till video source (its thread) stops
void XRaspiCameraData::WaitForStop( )
{
    SignalToStop( );

    if ( ( IsRunning( ) ) || ( ControlThread.joinable( ) ) )
    {
        ControlThread.join( );
    }
}

// Check if video source is still running
bool XRaspiCameraData::IsRunning( )
{
    lock_guard<recursive_mutex> lock( Sync );

    if ( ( !Running ) && ( ControlThread.joinable( ) ) )
    {
        ControlThread.join( );
    }

    return Running;
}

// Set video source listener
IVideoSourceListener* XRaspiCameraData::SetListener( IVideoSourceListener* listener )
{
    lock_guard<recursive_mutex> lock( Sync );
    IVideoSourceListener* oldListener = listener;

    Listener = listener;

    return oldListener;
}

// Notify listener with a new image
void XRaspiCameraData::NotifyNewImage( const std::shared_ptr<const XImage>& image )
{
    IVideoSourceListener* myListener;

    {
        lock_guard<recursive_mutex> lock( Sync );
        myListener = Listener;
    }

    if ( myListener != nullptr )
    {
        myListener->OnNewImage( image );
    }
}

// Notify listener about error
void XRaspiCameraData::NotifyError( const string& errorMessage, bool fatal )
{
    IVideoSourceListener* myListener;

    {
        lock_guard<recursive_mutex> lock( Sync );
        myListener = Listener;
    }

    if ( myListener != nullptr )
    {
        myListener->OnError( errorMessage, fatal );
    }
}

// Initialize camera and start capturing
bool XRaspiCameraData::Init( )
{
    lock_guard<recursive_mutex> lock( ConfigSync );
    MMAL_STATUS_T               status;

    if ( !HostInitDone )
    {
        bcm_host_init( );
        HostInitDone = true;
    }

    status = mmal_component_create( MMAL_COMPONENT_DEFAULT_CAMERA, &Camera );
    if ( status != MMAL_SUCCESS )
    {
        NotifyError( "Failed creating camera component", true );
    }
    else
    {
        VideoPort = Camera->output[CAMERA_VIDEO_PORT];

        status = mmal_port_enable( Camera->control, CameraControlCallback );
        if ( status != MMAL_SUCCESS )
        {
            NotifyError( "Failed enabling camera's control port", true );
        }
    }

    if ( status == MMAL_SUCCESS )
    {
        //  set-up the camera configuration
        MMAL_PARAMETER_CAMERA_CONFIG_T cam_config;

        cam_config.hdr.id                                = MMAL_PARAMETER_CAMERA_CONFIG;
        cam_config.hdr.size                              = sizeof( cam_config );
        cam_config.max_stills_w                          = FrameWidth;
        cam_config.max_stills_h                          = FrameHeight;
        cam_config.stills_yuv422                         = 0;
        cam_config.one_shot_stills                       = 0;
        cam_config.max_preview_video_w                   = FrameWidth;
        cam_config.max_preview_video_h                   = FrameHeight;
        cam_config.num_preview_video_frames              = 2;
        cam_config.stills_capture_circular_buffer_height = 0;
        cam_config.fast_preview_resume                   = 0;
        cam_config.use_stc_timestamp                     = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC;

        status = mmal_port_parameter_set( Camera->control, &cam_config.hdr );
        if ( status != MMAL_SUCCESS )
        {
            NotifyError( "Failed setting camera's configuration", true );
        }
    }

    if ( status == MMAL_SUCCESS )
    {
        // set-up video port format
        MMAL_ES_FORMAT_T* format = VideoPort->format;

        format->encoding                 = ( JpegEncoding ) ? MMAL_ENCODING_I420 : MMAL_ENCODING_RGB24;
        format->encoding_variant         = format->encoding;
        format->es->video.width          = FrameWidth;
        format->es->video.height         = FrameHeight;
        format->es->video.crop.x         = 0;
        format->es->video.crop.y         = 0;
        format->es->video.crop.width     = FrameWidth;
        format->es->video.crop.height    = FrameHeight;
        format->es->video.frame_rate.num = FrameRate;
        format->es->video.frame_rate.den = 1;

        status = mmal_port_format_commit( VideoPort );
        if ( status != MMAL_SUCCESS )
        {
            NotifyError( "Failed configuring video port", true );
        }
    }

    if ( status == MMAL_SUCCESS )
    {
        // enable the camera
        status = mmal_component_enable( Camera );
        if ( status != MMAL_SUCCESS )
        {
            NotifyError( "Failed enabling camera", true );
        }
    }

    // configure JPEG encoder in case user prefers getting JPEGs instead of RGB pixel data
    if ( JpegEncoding )
    {
        if ( status == MMAL_SUCCESS )
        {
            // create JPEG encoder component
            status = mmal_component_create( MMAL_COMPONENT_DEFAULT_IMAGE_ENCODER, &JpegEncoder );
            if ( status != MMAL_SUCCESS )
            {
                NotifyError( "Failed creating JPEG encoder", true );
            }
        }
        if ( status == MMAL_SUCCESS )
        {
            // set JPEG encoder's input/output ports' format
            MMAL_PORT_T* inputPort  = JpegEncoder->input[0];
            MMAL_PORT_T* outputPort = JpegEncoder->output[0];

            mmal_format_copy( inputPort->format, VideoPort->format );

            status = mmal_port_format_commit( inputPort );
            if ( status != MMAL_SUCCESS )
            {
                NotifyError( "Failed setting JPEG encoder's input port format", true );
            }
            else
            {
                mmal_format_copy( outputPort->format, inputPort->format );

                outputPort->format->encoding = MMAL_ENCODING_JPEG;

                status = mmal_port_format_commit( outputPort );
                if ( status != MMAL_SUCCESS )
                {
                    NotifyError( "Failed setting JPEG encoder's output port format", true );
                }
                else
                {
                    outputPort->buffer_size = outputPort->buffer_size_recommended;
                    outputPort->buffer_num  = outputPort->buffer_num_recommended;

                    if ( outputPort->buffer_size < outputPort->buffer_size_min )
                    {
                        outputPort->buffer_size = outputPort->buffer_size_min;
                    }
                    if ( outputPort->buffer_num < outputPort->buffer_num_min )
                    {
                        outputPort->buffer_num = outputPort->buffer_num_min;
                    }

                    // set JPEG quality
                    status = mmal_port_parameter_set_uint32( outputPort, MMAL_PARAMETER_JPEG_Q_FACTOR, JpegQuality );

                    if ( status != MMAL_SUCCESS )
                    {
                        NotifyError( "Failed setting JPEG quality" );
                    }
                }
            }
        }

        if ( status == MMAL_SUCCESS )
        {
            // connect video port to JPEG encoder
            status = mmal_connection_create( &JpegEncoderConnection, VideoPort, JpegEncoder->input[0],
                                             MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT );
            if ( status != MMAL_SUCCESS )
            {
                NotifyError( "Failed connecting video port to JPEG encoder", true );
            }
        }

        if ( status == MMAL_SUCCESS )
        {
            // enable the JPEG encoder connection
            status = mmal_connection_enable( JpegEncoderConnection );
            if ( status != MMAL_SUCCESS )
            {
                NotifyError( "Failed enabling JPEG encoder connection", true );
            }
        }
    }

    if ( status == MMAL_SUCCESS )
    {
        // create video buffer pool
        VideoBufferPort = ( JpegEncoding ) ? JpegEncoder->output[0] : VideoPort;
        VideoBufferPort->userdata = reinterpret_cast<MMAL_PORT_USERDATA_T*>( this );

        if ( VideoBufferPort->buffer_num < BUFFER_COUNT )
        {
            VideoBufferPort->buffer_num  = BUFFER_COUNT;
        }

        VideoBufferPool = mmal_port_pool_create( VideoBufferPort, VideoBufferPort->buffer_num, VideoBufferPort->buffer_size );
        if ( VideoBufferPool == nullptr )
        {
            NotifyError( "Failed creating video buffer pool", true );
            status = MMAL_EFAULT;
        }
    }

    if ( status == MMAL_SUCCESS )
    {
        // enable the video buffer port
        status = mmal_port_enable( VideoBufferPort, VideoBufferCallback );
        if ( status != MMAL_SUCCESS )
        {
            NotifyError( "Failed enabling video buffer port", true );
        }
    }

    if ( status == MMAL_SUCCESS )
    {
        // send all buffers to the video buffer port
        int queueLength = mmal_queue_length( VideoBufferPool->queue );

        for ( int i = 0; ( i < queueLength ) && ( status == MMAL_SUCCESS ); i++ )
        {
            MMAL_BUFFER_HEADER_T* buffer = mmal_queue_get( VideoBufferPool->queue );

            if ( buffer == nullptr )
            {
                status = MMAL_EFAULT;
            }
            else
            {
                status = mmal_port_send_buffer( VideoBufferPort, buffer );
            }
        }

        if ( status != MMAL_SUCCESS )
        {
            NotifyError( "Failed configuring video buffer pool", true );
        }
    }

    if ( status == MMAL_SUCCESS )
    {
        // apply all configuration
        if ( ( !SetCameraFlip( HorizontalFlip, VerticalFlip ) ) ||
             ( !SetImageRotation( ImageRotation ) ) ||
             ( !SetVideoStabilisation( VideoStabilisation ) ) ||
             ( !SetSharpness( Sharpness ) ) ||
             ( !SetContrast( Contrast ) ) ||
             ( !SetBrightness( Brightness ) ) ||
             ( !SetSaturation( Saturation ) ) ||
             ( !SetWhiteBalanceMode( WhiteBalanceMode ) ) ||
             ( !SetExposureMode( CameraExposureMode ) ) ||
             ( !SetExposureMeteringMode( CameraExposureMeteringMode ) ) ||
             ( !SetImageEffect( CameraImageEffect ) ) ||
             ( !SetTextTextAnnotation( TextAnnotation, TextBlackBackground ) ) )
        {
            NotifyError( "Failed applying camera configuration" );
        }
    }

    if ( status == MMAL_SUCCESS )
    {
        // begin capture
        if ( mmal_port_parameter_set_boolean( VideoPort, MMAL_PARAMETER_CAPTURE, 1 ) != MMAL_SUCCESS )
        {
            NotifyError( "Failed starting video capture", true );
        }
    }

    return ( status == MMAL_SUCCESS );
}

// Stop camera capture and clean-up
void XRaspiCameraData::Cleanup( )
{
    lock_guard<recursive_mutex> lock( ConfigSync );

    if ( VideoPort != nullptr )
    {
        mmal_port_parameter_set_boolean( VideoPort, MMAL_PARAMETER_CAPTURE, 0 );
    }

    if ( VideoBufferPort != nullptr )
    {
        mmal_port_disable( VideoBufferPort );
    }

    if ( JpegEncoderConnection != nullptr )
    {
        mmal_connection_disable( JpegEncoderConnection );
        mmal_connection_destroy( JpegEncoderConnection );
        JpegEncoderConnection = nullptr;
    }

    if ( VideoBufferPool != nullptr )
    {
        mmal_port_pool_destroy( VideoPort, VideoBufferPool );
        VideoBufferPool = nullptr;
    }

    if ( JpegEncoder != nullptr )
    {
        mmal_component_destroy( JpegEncoder );
        JpegEncoder = nullptr;
    }

    if ( Camera != nullptr )
    {
        mmal_component_destroy( Camera );
        Camera = nullptr;
    }

    VideoPort = nullptr;
    VideoBufferPort = nullptr;
}

// Set size of video frames to be provided
void XRaspiCameraData::SetVideoSize( uint32_t width, uint32_t height )
{
    lock_guard<recursive_mutex> lock( ConfigSync );

    if ( !IsRunning( ) )
    {
        FrameWidth  = width;
        FrameHeight = height;
    }
}

// Set frame rate to be set when starting camera
void XRaspiCameraData::SetFrameRate( uint32_t frameRate )
{
    lock_guard<recursive_mutex> lock( ConfigSync );

    if ( !IsRunning( ) )
    {
        FrameRate = frameRate;
    }
}

// Enable/disable JPEG encoding
void XRaspiCameraData::EnableJpegEncoding( bool enable )
{
    lock_guard<recursive_mutex> lock( ConfigSync );

    if ( !IsRunning( ) )
    {
        JpegEncoding = enable;
    }
}

// Set quality of provided JPEG images
void XRaspiCameraData::SetJpegQuality( uint32_t jpegQuality )
{
    lock_guard<recursive_mutex> lock( ConfigSync );

    if ( !IsRunning( ) )
    {
        JpegQuality = jpegQuality;
    }
}

// Set camera's horizontal/vertical flip
bool XRaspiCameraData::SetCameraFlip( bool horizontal, bool vertical )
{
    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    HorizontalFlip = horizontal;
    VerticalFlip   = vertical;

    if ( VideoPort != nullptr )
    {
        MMAL_PARAMETER_MIRROR_T mirror = { { MMAL_PARAMETER_MIRROR, sizeof( MMAL_PARAMETER_MIRROR_T ) }, MMAL_PARAM_MIRROR_NONE };

        if ( ( horizontal ) && ( vertical ) )
        {
           mirror.value = MMAL_PARAM_MIRROR_BOTH;
        }
        else if ( horizontal )
        {
           mirror.value = MMAL_PARAM_MIRROR_HORIZONTAL;
        }
        else if ( vertical )
        {
           mirror.value = MMAL_PARAM_MIRROR_VERTICAL;
        }

        ret = ( mmal_port_parameter_set( VideoPort, &mirror.hdr ) == MMAL_SUCCESS );
    }

    return ret;
}

// Set camera's image rotation
bool XRaspiCameraData::SetImageRotation( uint32_t rotation )
{
    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    ImageRotation = ( ( rotation % 360 ) / 90 ) * 90;

    if ( VideoPort != nullptr )
    {
        ret = ( mmal_port_parameter_set_int32( VideoPort, MMAL_PARAMETER_ROTATION,
                static_cast<int32_t>( ImageRotation ) ) == MMAL_SUCCESS );
    }

    return ret;
}

// Set video stabilisation flag
bool XRaspiCameraData::SetVideoStabilisation( bool enabled )
{
    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    VideoStabilisation = enabled;

    if ( Camera != nullptr )
    {
        ret = ( mmal_port_parameter_set_boolean( Camera->control, MMAL_PARAMETER_VIDEO_STABILISATION, VideoStabilisation ) == MMAL_SUCCESS );
    }

    return ret;
}

// Set camera's sharpness value
bool XRaspiCameraData::SetSharpness( int32_t sharpness )
{
    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    Sharpness = sharpness;

    if ( Sharpness < -100 ) { Sharpness = -100; }
    if ( Sharpness >  100 ) { Sharpness =  100; }

    if ( Camera != nullptr )
    {
        MMAL_RATIONAL_T value = { Sharpness, 100 };

        ret = ( mmal_port_parameter_set_rational( Camera->control, MMAL_PARAMETER_SHARPNESS, value ) == MMAL_SUCCESS );
    }

    return ret;
}

// Set camera's contrast value
bool XRaspiCameraData::SetContrast( int32_t contrast )
{
    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    Contrast = contrast;

    if ( Contrast < -100 ) { Contrast = -100; }
    if ( Contrast >  100 ) { Contrast =  100; }

    if ( Camera != nullptr )
    {
        MMAL_RATIONAL_T value = { Contrast, 100 };

        ret = ( mmal_port_parameter_set_rational( Camera->control, MMAL_PARAMETER_CONTRAST, value ) == MMAL_SUCCESS );
    }

    return ret;
}

// Set camera's brightness value
bool XRaspiCameraData::SetBrightness( int32_t brightness )
{
    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    Brightness = brightness;

    if ( Brightness <   0 ) { Brightness =   0; }
    if ( Brightness > 100 ) { Brightness = 100; }

    if ( Camera != nullptr )
    {
        MMAL_RATIONAL_T value = { Brightness, 100 };

        ret = ( mmal_port_parameter_set_rational( Camera->control, MMAL_PARAMETER_BRIGHTNESS, value ) == MMAL_SUCCESS );
    }

    return ret;
}

// Set camera's saturation value
bool XRaspiCameraData::SetSaturation( int32_t saturation )
{
    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    Saturation = saturation;

    if ( Saturation < -100 ) { Saturation = -100; }
    if ( Saturation >  100 ) { Saturation =  100; }

    if ( Camera != nullptr )
    {
        MMAL_RATIONAL_T value = { Saturation, 100 };

        ret = ( mmal_port_parameter_set_rational( Camera->control, MMAL_PARAMETER_SATURATION, value ) == MMAL_SUCCESS );
    }

    return ret;
}

// Set camera's Automatic White Balance mode
bool XRaspiCameraData::SetWhiteBalanceMode( AwbMode mode )
{
    static const MMAL_PARAM_AWBMODE_T modes[] =
    {
        MMAL_PARAM_AWBMODE_OFF,
        MMAL_PARAM_AWBMODE_AUTO,
        MMAL_PARAM_AWBMODE_SUNLIGHT,
        MMAL_PARAM_AWBMODE_CLOUDY,
        MMAL_PARAM_AWBMODE_SHADE,
        MMAL_PARAM_AWBMODE_TUNGSTEN,
        MMAL_PARAM_AWBMODE_FLUORESCENT,
        MMAL_PARAM_AWBMODE_INCANDESCENT,
        MMAL_PARAM_AWBMODE_FLASH,
        MMAL_PARAM_AWBMODE_HORIZON
    };

    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    WhiteBalanceMode = mode;

    if ( Camera != nullptr )
    {
        int imode = static_cast<int>( mode );
        MMAL_PARAM_AWBMODE_T nativeMode = ( ( imode < 0 ) || ( imode >= sizeof( modes ) / sizeof( modes[0] ) ) ) ?
                                            MMAL_PARAM_AWBMODE_AUTO : modes[imode];

        MMAL_PARAMETER_AWBMODE_T param = { { MMAL_PARAMETER_AWB_MODE, sizeof( param ) }, nativeMode };

        ret = ( mmal_port_parameter_set( Camera->control, &param.hdr ) == MMAL_SUCCESS );
    }

    return ret;
}

// Set camera's exposure mode
bool XRaspiCameraData::SetExposureMode( ExposureMode mode )
{
    static const MMAL_PARAM_EXPOSUREMODE_T modes[] =
    {
        MMAL_PARAM_EXPOSUREMODE_OFF,
        MMAL_PARAM_EXPOSUREMODE_AUTO,
        MMAL_PARAM_EXPOSUREMODE_NIGHT,
        MMAL_PARAM_EXPOSUREMODE_NIGHTPREVIEW,
        MMAL_PARAM_EXPOSUREMODE_BACKLIGHT,
        MMAL_PARAM_EXPOSUREMODE_SPOTLIGHT,
        MMAL_PARAM_EXPOSUREMODE_SPORTS,
        MMAL_PARAM_EXPOSUREMODE_SNOW,
        MMAL_PARAM_EXPOSUREMODE_BEACH,
        MMAL_PARAM_EXPOSUREMODE_VERYLONG,
        MMAL_PARAM_EXPOSUREMODE_FIXEDFPS,
        MMAL_PARAM_EXPOSUREMODE_ANTISHAKE,
        MMAL_PARAM_EXPOSUREMODE_FIREWORKS
    };

    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    CameraExposureMode = mode;

    if ( Camera != nullptr )
    {
        int imode = static_cast<int>( mode );
        MMAL_PARAM_EXPOSUREMODE_T nativeMode = ( ( imode < 0 ) || ( imode >= sizeof( modes ) / sizeof( modes[0] ) ) ) ?
                                                MMAL_PARAM_EXPOSUREMODE_AUTO : modes[imode];

        MMAL_PARAMETER_EXPOSUREMODE_T param = { { MMAL_PARAMETER_EXPOSURE_MODE, sizeof( param ) }, nativeMode };

        ret = ( mmal_port_parameter_set( Camera->control, &param.hdr ) == MMAL_SUCCESS );
    }

    return ret;
}

// Set camera's exposure metering mode
bool XRaspiCameraData::SetExposureMeteringMode( ExposureMeteringMode mode )
{
    static const MMAL_PARAM_EXPOSUREMETERINGMODE_T modes[] =
    {
        MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE,
        MMAL_PARAM_EXPOSUREMETERINGMODE_SPOT,
        MMAL_PARAM_EXPOSUREMETERINGMODE_BACKLIT,
        MMAL_PARAM_EXPOSUREMETERINGMODE_MATRIX
    };

    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    CameraExposureMeteringMode = mode;

    if ( Camera != nullptr )
    {
        int imode = static_cast<int>( mode );
        MMAL_PARAM_EXPOSUREMETERINGMODE_T nativeMode = ( ( imode < 0 ) || ( imode >= sizeof( modes ) / sizeof( modes[0] ) ) ) ?
                                                        MMAL_PARAM_EXPOSUREMETERINGMODE_AVERAGE : modes[imode];

        MMAL_PARAMETER_EXPOSUREMETERINGMODE_T param = { { MMAL_PARAMETER_EXP_METERING_MODE, sizeof( param ) }, nativeMode };

        ret = ( mmal_port_parameter_set( Camera->control, &param.hdr ) == MMAL_SUCCESS );
    }

    return ret;
}

// Set camera's image effect
bool XRaspiCameraData::SetImageEffect( ImageEffect effect )
{
    static const MMAL_PARAM_IMAGEFX_T effects[] =
    {
        MMAL_PARAM_IMAGEFX_NONE,
        MMAL_PARAM_IMAGEFX_NEGATIVE,
        MMAL_PARAM_IMAGEFX_SOLARIZE,
        MMAL_PARAM_IMAGEFX_POSTERIZE,
        MMAL_PARAM_IMAGEFX_WHITEBOARD,
        MMAL_PARAM_IMAGEFX_BLACKBOARD,
        MMAL_PARAM_IMAGEFX_SKETCH,
        MMAL_PARAM_IMAGEFX_DENOISE,
        MMAL_PARAM_IMAGEFX_EMBOSS,
        MMAL_PARAM_IMAGEFX_OILPAINT,
        MMAL_PARAM_IMAGEFX_HATCH,
        MMAL_PARAM_IMAGEFX_GPEN,
        MMAL_PARAM_IMAGEFX_PASTEL,
        MMAL_PARAM_IMAGEFX_WATERCOLOUR,
        MMAL_PARAM_IMAGEFX_FILM,
        MMAL_PARAM_IMAGEFX_BLUR,
        MMAL_PARAM_IMAGEFX_SATURATION,
        MMAL_PARAM_IMAGEFX_COLOURSWAP,
        MMAL_PARAM_IMAGEFX_WASHEDOUT,
        MMAL_PARAM_IMAGEFX_POSTERISE,
        MMAL_PARAM_IMAGEFX_COLOURPOINT,
        MMAL_PARAM_IMAGEFX_COLOURBALANCE,
        MMAL_PARAM_IMAGEFX_CARTOON
    };

    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    CameraImageEffect = effect;

    if ( Camera != nullptr )
    {
        int ieffect = static_cast<int>( effect );
        MMAL_PARAM_IMAGEFX_T nativeEffect = ( ( ieffect < 0 ) || ( ieffect >= sizeof( effects ) / sizeof( effects[0] ) ) ) ?
                                                MMAL_PARAM_IMAGEFX_NONE : effects[ieffect];

        MMAL_PARAMETER_IMAGEFX_T param = { { MMAL_PARAMETER_IMAGE_EFFECT, sizeof( param ) }, nativeEffect };

        ret = ( mmal_port_parameter_set( Camera->control, &param.hdr ) == MMAL_SUCCESS );
    }

    return ret;
}

// Add text annotation to camera's images
bool XRaspiCameraData::SetTextTextAnnotation( const string& text, bool blackBackground )
{
    lock_guard<recursive_mutex> lock( ConfigSync );
    bool                        ret = true;

    TextAnnotation      = text;
    TextBlackBackground = blackBackground;

    if ( Camera != nullptr )
    {
        MMAL_PARAMETER_CAMERA_ANNOTATE_V2_T param = { { MMAL_PARAMETER_ANNOTATE, sizeof( param ) } };

        param.enable                = ( text.empty( ) ) ? 0 : 1;
        param.show_shutter          = 0;
        param.show_analog_gain      = 0;
        param.show_lens             = 0;
        param.show_caf              = 0;
        param.show_motion           = 0;
        param.show_frame_num        = 0;
        param.black_text_background = ( blackBackground ) ? 1 : 0;

        strncpy( param.text, text.c_str( ), MMAL_CAMERA_ANNOTATE_MAX_TEXT_LEN_V2 - 1 );
        param.text[MMAL_CAMERA_ANNOTATE_MAX_TEXT_LEN_V2 - 1] = 0;

        ret = ( mmal_port_parameter_set( Camera->control, &param.hdr ) == MMAL_SUCCESS );
    }

    return ret;
}

// Background control thread - does not do much other than init, clean-up and wait in between
void XRaspiCameraData::ControlThreadHanlder( XRaspiCameraData* me )
{
    if ( me->Init( ) )
    {
        while ( !me->NeedToStop.Wait( 1000 ) )
        {
        }
    }

    me->Cleanup( );

    {
        lock_guard<recursive_mutex> lock( me->Sync );
        me->Running = false;
    }
}

// Callback for camera's control port
void XRaspiCameraData::CameraControlCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer )
{
}

// Callback signalling availability of a new video frame
void XRaspiCameraData::VideoBufferCallback( MMAL_PORT_T* port, MMAL_BUFFER_HEADER_T* buffer )
{
    XRaspiCameraData* me = reinterpret_cast<XRaspiCameraData*>( port->userdata );

    if ( buffer->length != 0 )
    {
        mmal_buffer_header_mem_lock( buffer );

        {
            shared_ptr<XImage> image = ( !me->JpegEncoding ) ?
                XImage::Create( buffer->data + buffer->offset, me->FrameWidth, me->FrameHeight, me->FrameWidth * 3, XPixelFormat::RGB24 ) :
                XImage::Create( buffer->data + buffer->offset, buffer->length, 1, buffer->length, XPixelFormat::JPEG );

            me->FramesReceived++;

            if ( image )
            {
                me->NotifyNewImage( image );
            }
            else
            {
                me->NotifyError( "Failed allocating an image" );
            }
        }

        mmal_buffer_header_mem_unlock( buffer );
    }

    mmal_buffer_header_release( buffer );

    if ( port->is_enabled )
    {
        MMAL_BUFFER_HEADER_T* newBuffer;
        MMAL_STATUS_T         status;

        newBuffer = mmal_queue_get( me->VideoBufferPool->queue );
        if ( newBuffer )
        {
            status = mmal_port_send_buffer( port, newBuffer );
        }

        if ( ( !newBuffer ) || ( status != MMAL_SUCCESS ) )
        {
            me->NotifyError( "Unable to return buffer to video port" );
        }
    }
}

} // namespace Private

