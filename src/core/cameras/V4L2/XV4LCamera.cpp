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

#include <mutex>
#include <thread>
#include <chrono>

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#include "XV4LCamera.hpp"
#include "XManualResetEvent.hpp"

using namespace std;
using namespace std::chrono;

namespace Private
{
    #define BUFFER_COUNT        (4)

    // Private details of the implementation
    class XV4LCameraData
    {
    private:
        recursive_mutex         Sync;
        recursive_mutex         ConfigSync;
        thread                  ControlThread;
        XManualResetEvent       NeedToStop;
        IVideoSourceListener*   Listener;
        bool                    Running;

        int                     VideoFd;
        bool                    VideoStreamingActive;
        uint8_t*                MappedBuffers[BUFFER_COUNT];
        uint32_t                MappedBufferLength[BUFFER_COUNT];

    public:
        uint32_t                VideoDevice;
        uint32_t                FramesReceived;
        uint32_t                FrameWidth;
        uint32_t                FrameHeight;
        uint32_t                FrameRate;
        bool                    JpegEncoding;

    public:
        XV4LCameraData( ) :
            Sync( ), ConfigSync( ), ControlThread( ), NeedToStop( ), Listener( nullptr ), Running( false ),
            VideoFd( -1 ), VideoStreamingActive( false ), MappedBuffers( ), MappedBufferLength( ),
            VideoDevice( 0 ),
            FramesReceived( 0 ), FrameWidth( 640 ), FrameHeight( 480 ), FrameRate( 30 ), JpegEncoding( true )
        {
        }

        void SetVideoDevice( uint32_t videoDevice );

        bool Start( );
        void SignalToStop( );
        void WaitForStop( );
        bool IsRunning( );
        IVideoSourceListener* SetListener( IVideoSourceListener* listener );

        void NotifyNewImage( const std::shared_ptr<const XImage>& image );
        void NotifyError( const string& errorMessage, bool fatal = false );

        static void ControlThreadHanlder( XV4LCameraData* me );

    private:
        bool Init( );
        void VideoCaptureLoop( );
        void Cleanup( );

    };
}

const shared_ptr<XV4LCamera> XV4LCamera::Create( )
{
    return shared_ptr<XV4LCamera>( new XV4LCamera );
}

XV4LCamera::XV4LCamera( ) :
    mData( new Private::XV4LCameraData( ) )
{
}

XV4LCamera::~XV4LCamera( )
{
    delete mData;
}

// Set/get video device
uint32_t XV4LCamera::VideoDevice( ) const
{
    return mData->VideoDevice;
}
void XV4LCamera::SetVideoDevice( uint32_t videoDevice )
{
    mData->SetVideoDevice( videoDevice );
}

// Start the video source
bool XV4LCamera::Start( )
{
    return mData->Start( );
}

// Signal video source to stop
void XV4LCamera::SignalToStop( )
{
    mData->SignalToStop( );
}

// Wait till video source stops
void XV4LCamera::WaitForStop( )
{
    mData->WaitForStop( );
}

// Check if video source is still running
bool XV4LCamera::IsRunning( )
{
    return mData->IsRunning( );
}

// Get number of frames received since the start of the video source
uint32_t XV4LCamera::FramesReceived( )
{
    return mData->FramesReceived;
}

// Set video source listener
IVideoSourceListener* XV4LCamera::SetListener( IVideoSourceListener* listener )
{
    return mData->SetListener( listener );
}

namespace Private
{

// Set vide device number to use
void XV4LCameraData::SetVideoDevice( uint32_t videoDevice )
{
    lock_guard<recursive_mutex> lock( Sync );

    if ( !IsRunning( ) )
    {
        VideoDevice = videoDevice;
    }
}

// Start video source so it initializes and begins providing video frames
bool XV4LCameraData::Start( )
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
void XV4LCameraData::SignalToStop( )
{
    lock_guard<recursive_mutex> lock( Sync );

    if ( IsRunning( ) )
    {
        NeedToStop.Signal( );
    }
}

// Wait till video source (its thread) stops
void XV4LCameraData::WaitForStop( )
{
    SignalToStop( );

    if ( ( IsRunning( ) ) || ( ControlThread.joinable( ) ) )
    {
        ControlThread.join( );
    }
}

// Check if video source is still running
bool XV4LCameraData::IsRunning( )
{
    lock_guard<recursive_mutex> lock( Sync );
    return Running;
}

// Set video source listener
IVideoSourceListener* XV4LCameraData::SetListener( IVideoSourceListener* listener )
{
    lock_guard<recursive_mutex> lock( Sync );
    IVideoSourceListener* oldListener = listener;

    Listener = listener;

    return oldListener;
}

// Notify listener with a new image
void XV4LCameraData::NotifyNewImage( const std::shared_ptr<const XImage>& image )
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
void XV4LCameraData::NotifyError( const string& errorMessage, bool fatal )
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
bool XV4LCameraData::Init( )
{
    lock_guard<recursive_mutex> lock( ConfigSync );
    char                        strVideoDevice[32];
    bool                        ret = true;
    int                         ecode;

    sprintf( strVideoDevice, "/dev/video%d", VideoDevice );

    // open video device
    VideoFd = open( strVideoDevice, O_RDWR );
    if ( VideoFd == -1 )
    {
        NotifyError( "Failed opening video device", true );
        ret = false;
    }
    else
    {
        v4l2_capability videoCapability = { 0 };

        // get video capabilities of the device
        ecode = ioctl( VideoFd, VIDIOC_QUERYCAP, &videoCapability );
        if ( ecode < 0 )
        {
            NotifyError( "Failed getting video capabilities of the device", true );
            ret = false;
        }
        // make sure device supports video capture
        else if ( ( videoCapability.capabilities & V4L2_CAP_VIDEO_CAPTURE ) == 0 )
        {
            NotifyError( "Device does not support video capture", true );
            ret = false;
        }
        else if ( ( videoCapability.capabilities & V4L2_CAP_STREAMING ) == 0 )
        {
            NotifyError( "Device does not support streaming", true );
            ret = false;
        }
    }

    // configure video format
    if ( ret )
    {
        v4l2_format videoFormat = { 0 };
        uint32_t    pixelFormat = ( JpegEncoding ) ? V4L2_PIX_FMT_MJPEG : V4L2_PIX_FMT_YUYV;

        videoFormat.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        videoFormat.fmt.pix.width       = FrameWidth;
        videoFormat.fmt.pix.height      = FrameHeight;
        videoFormat.fmt.pix.pixelformat = pixelFormat;
        videoFormat.fmt.pix.field       = V4L2_FIELD_ANY;
    
        ecode = ioctl( VideoFd, VIDIOC_S_FMT, &videoFormat );
        if ( ecode < 0 )
        {
            NotifyError( "Failed setting video format", true );
            ret = false;
        }
        else if ( videoFormat.fmt.pix.pixelformat != pixelFormat )
        {
            NotifyError( string( "The camera does not support requested format: " ) + ( ( JpegEncoding ) ? "MJPEG" : "YUYV" ), true );
            ret = false;
        }
        else
        {
            // update width/height in case camera does not support what was requested
            FrameWidth  = videoFormat.fmt.pix.width;
            FrameHeight = videoFormat.fmt.pix.height;
        }
    }

    // request capture buffers
    if ( ret )
    {
        v4l2_requestbuffers requestBuffers = { 0 };

        requestBuffers.count  = BUFFER_COUNT;
        requestBuffers.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        requestBuffers.memory = V4L2_MEMORY_MMAP;

        ecode = ioctl( VideoFd, VIDIOC_REQBUFS, &requestBuffers );
        if ( ecode < 0 )
        {
            NotifyError( "Unable to allocate capture buffers", true );
            ret = false;
        }
        else if ( requestBuffers.count < BUFFER_COUNT )
        {
            NotifyError( "Not enough memory to allocate capture buffers", true );
            ret = false;
        }
    }

    // map capture buffers
    if ( ret )
    {
        v4l2_buffer videoBuffer;

        for ( int i = 0; i < BUFFER_COUNT; i++ )
        {
            memset( &videoBuffer, 0, sizeof( videoBuffer ) );

            videoBuffer.index  = i;
            videoBuffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            videoBuffer.memory = V4L2_MEMORY_MMAP;

            ecode = ioctl( VideoFd, VIDIOC_QUERYBUF, &videoBuffer );
            if ( ecode < 0 )
            {
                NotifyError( "Unable to query capture buffer", true );
                ret = false;
                break;
            }

            MappedBuffers[i]      = (uint8_t*) mmap( 0, videoBuffer.length, PROT_READ, MAP_SHARED, VideoFd, videoBuffer.m.offset );
            MappedBufferLength[i] = videoBuffer.length;

            if ( MappedBuffers[i] == nullptr )
            {
                NotifyError( "Unable to map capture buffer", true );
                ret = false;
                break;
            }
        }
    }

    // enqueue capture buffers
    if ( ret )
    {
        v4l2_buffer videoBuffer;

        for ( int i = 0; i < BUFFER_COUNT; i++ )
        {
            memset( &videoBuffer, 0, sizeof( videoBuffer ) );
        
            videoBuffer.index  = i;
            videoBuffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            videoBuffer.memory = V4L2_MEMORY_MMAP;
        
            ecode = ioctl( VideoFd, VIDIOC_QBUF, &videoBuffer );
        
            if ( ecode < 0 )
            {
                NotifyError( "Unable to enqueue capture buffer", true );
                ret = false;
            }
        }
    }

    // enable video streaming
    if ( ret )
    {
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    
        ecode = ioctl( VideoFd, VIDIOC_STREAMON, &type );
        if ( ecode < 0 )
        {
            NotifyError( "Failed starting video streaming", true );
            ret = false;
        }
        else
        {
            VideoStreamingActive = true;
        }
    }

    return ret;
}

// Stop camera capture and clean-up
void XV4LCameraData::Cleanup( )
{
    lock_guard<recursive_mutex> lock( ConfigSync );

    // disable vide streaming
    if ( VideoStreamingActive )
    {
        int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        ioctl( VideoFd, VIDIOC_STREAMOFF, &type );
        VideoStreamingActive = false;
    }

    // unmap capture buffers
    for ( int i = 0; i < BUFFER_COUNT; i++ )
    {
        if ( MappedBuffers[i] != nullptr )
        {
            munmap( MappedBuffers[i], MappedBufferLength[i] );
            MappedBuffers[i]      = nullptr;
            MappedBufferLength[i] = 0;
        }
    }

    // close the video device
    if ( VideoFd != -1 )
    {
        close( VideoFd );
        VideoFd = -1;
    }
}

// Helper function to decode YUYV data into RGB
static void DecodeYuyvToRgb( const uint8_t* yuyvPtr, uint8_t* rgbPtr, int32_t width, int32_t height, int32_t rgbStride )
{
    /* 
        The code below does YUYV to RGB conversion using the next coefficients.
        However those are multiplied by 256 to get integer calculations.
     
        r = y + (1.4065 * (cr - 128));
        g = y - (0.3455 * (cb - 128)) - (0.7169 * (cr - 128));
        b = y + (1.7790 * (cb - 128));
    */

    int r, g, b;
    int y, u, v;
    int z = 0;

    for ( int32_t iy = 0; iy < height; iy++ )
    {
        uint8_t* rgbRow = rgbPtr + iy * rgbStride;

        for ( int32_t ix = 0; ix < width; ix++ )
        {
            y = ( ( z == 0 ) ? yuyvPtr[0] : yuyvPtr[2] ) << 8;
            u = yuyvPtr[1] - 128;
            v = yuyvPtr[3] - 128;

            r = ( y + ( 360 * v ) ) >> 8;
            g = ( y - ( 88  * u ) - ( 184 * v ) ) >> 8;
            b = ( y + ( 455 * u ) ) >> 8;

            rgbRow[RedIndex]   = (uint8_t) ( r > 255 ) ? 255 : ( ( r < 0 ) ? 0 : r );
            rgbRow[GreenIndex] = (uint8_t) ( g > 255 ) ? 255 : ( ( g < 0 ) ? 0 : g );
            rgbRow[BlueIndex]  = (uint8_t) ( b > 255 ) ? 255 : ( ( b < 0 ) ? 0 : b );

            if ( z++ )
            {
                z = 0;
                yuyvPtr += 4;
            }

            rgbRow += 3;
        }
    }
}

// Do video capture in an end-less loop until signalled to stop
void XV4LCameraData::VideoCaptureLoop( )
{
    v4l2_buffer videoBuffer;
    uint32_t    sleepTime = 0;
    uint32_t    frameTime = 1000 / FrameRate;
    uint32_t    handlingTime ;
    int         ecode;

    // If JPEG encoding is used, client is notified with an image wrapping a mapped buffer.
    // If not used howver, we decode YUYV data into RGB.
    shared_ptr<XImage> rgbImage;
    
    if ( !JpegEncoding )
    {
        rgbImage = XImage::Allocate( FrameWidth, FrameHeight, XPixelFormat::RGB24 );

        if ( !rgbImage )
        {
            NotifyError( "Failed allocating an image", true );
            return;
        }
    }

    // acquire images untill we've been told to stop
    while ( !NeedToStop.Wait( sleepTime ) )
    {
        steady_clock::time_point startTime = steady_clock::now( );

        // dequeue buffer
        memset( &videoBuffer, 0, sizeof( videoBuffer ) );

        videoBuffer.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        videoBuffer.memory = V4L2_MEMORY_MMAP;

        ecode = ioctl( VideoFd, VIDIOC_DQBUF, &videoBuffer );
        if ( ecode < 0 )
        {
            NotifyError( "Failed to dequeue capture buffer" );
        }
        else
        {
            shared_ptr<XImage> image;

            if ( JpegEncoding )
            {
                image = XImage::Create( MappedBuffers[videoBuffer.index], videoBuffer.bytesused, 1, videoBuffer.bytesused, XPixelFormat::JPEG );
            }
            else
            {
                DecodeYuyvToRgb( MappedBuffers[videoBuffer.index], rgbImage->Data( ), FrameWidth, FrameHeight, rgbImage->Stride( ) );
                image = rgbImage;
            }

            if ( image )
            {
                NotifyNewImage( image );
            }
            else
            {
                NotifyError( "Failed allocating an image" );
            }

            // put the buffer back into the queue
            ecode = ioctl( VideoFd, VIDIOC_QBUF, &videoBuffer );
            if ( ecode < 0 )
            {
                NotifyError( "Failed to requeue capture buffer" );
            }
        }

        handlingTime = static_cast<uint32_t>( duration_cast<milliseconds>( steady_clock::now( ) - startTime ).count( ) );
        sleepTime    = ( handlingTime > frameTime ) ? 0 : ( frameTime - handlingTime );
    }
}

// Background control thread - performs camera init/clean-up and runs video loop
void XV4LCameraData::ControlThreadHanlder( XV4LCameraData* me )
{    
    if ( me->Init( ) )
    {
        me->VideoCaptureLoop( );
    }
    
    me->Cleanup( );
    
    {
        lock_guard<recursive_mutex> lock( me->Sync );
        me->Running = false;
    }
}

} // namespace Private

