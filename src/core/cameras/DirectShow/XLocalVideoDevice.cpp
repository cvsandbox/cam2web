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

#include <assert.h>
#include <string>
#include <map>
#include <algorithm>
#include <mutex>
#include <thread>
#include <intrin.h>

// Include DirectShow relate headers
#include <dshow.h>
#include <Dvdmedia.h>
// Include qedit.h to get ISampleGrabberCB declaration
#include <qedit.h>

#include "XLocalVideoDevice.hpp"
#include "XManualResetEvent.hpp"

using namespace std;

#ifdef _MSC_VER
#pragma comment(lib, "strmiids")
#endif

// Namespace with some private stuff to hide
namespace Private
{
    // Set video input of the provided crossbar's interface
    static void SetCurrentCrossbarInput( IAMCrossbar* pCrossbar, const XDevicePinInfo& videoInput );
    // Get current video input of the provided crossbar's interface
    static XDevicePinInfo GetCurrentCrossbarInput( IAMCrossbar* pCrossbar );
    // Collect available video input pins of the crossbar
    static vector<XDevicePinInfo> ColletCrossbarVideoInputs( IAMCrossbar* pCrossbar );
    // Enumerate specified category/type filter's capabilities and set its media type to one of those capabilities
    static vector<XDeviceCapabilities> GetPinCapabilitiesAndConfigureIt( ICaptureGraphBuilder2* pCaptureGraphBuilder,
                                                                         IBaseFilter* pBaseFilter, const GUID* pCategory,
                                                                         const XDeviceCapabilities& capToSet,
                                                                         uint32_t requestedFps = 0 );
    // Check if filter provides output pin supporting MEDIASUBTYPE_MJPG media subtype
    static bool IsJpegEncodingAvailable( IBaseFilter* pBaseFilter );

    // Create filter described by the specified moniker string
    static IBaseFilter* CreateFilter( const string& moniker );

    // Convert specfied wide character string to UTF8
    static string Utf16to8( LPCWSTR utf16string );
    // Free media type - release memory taken by some of its members
    static void FreeMediaType( AM_MEDIA_TYPE& mt );
    // Delete media type - free "inner" member and then delete structure itself
    static void DeleteMediaType( AM_MEDIA_TYPE* pmt );

    #define RELEASE_COM( pObject ) if ( pObject != nullptr ) { pObject->Release( );  pObject = nullptr; }

    class SampleGrabber;

    // Private details of the implementation
    class XLocalVideoDeviceData
    {
    public:
        recursive_mutex             Sync;
        string                      DeviceMoniker;
        XDeviceCapabilities         Resolution;
        uint32_t                    RequestedFps;
        XDevicePinInfo              VideoInput;
        volatile bool               NeedToSetVideoInput;

        vector<XDeviceCapabilities> Capabilities;
        vector<XDevicePinInfo>      VideoPins;
        bool                        IsCrossbarAvailable;
        XManualResetEvent           InfoCollectedEvent;

        bool                        JpegEncodingPreferred;
        bool                        JpegEncodingEnabled;

    private:
        IVideoSourceListener*       Listener;

        XManualResetEvent           ExitEvent;
        thread                      BackgroundThread;
        bool                        Running;
        uint32_t                    FramesCounter;

        mutable recursive_mutex     RunningSync;
        XManualResetEvent           DeviceIsRunningEvent;
        bool                        DeviceIsRunning;
        IAMVideoProcAmp*            VideoProcAmp;
        IAMCameraControl*           CameraControl;

        typedef pair<uint32_t, bool>    PropValue;
        map<XVideoProperty, PropValue>  VideoPropertiesToSet;
        map<XCameraProperty, PropValue> CameraPropertiesToSet;

        friend class SampleGrabber;
        
    public:
        XLocalVideoDeviceData( const string& deviceMoniker ) :
            Sync( ), DeviceMoniker( deviceMoniker ), Resolution( ), RequestedFps( 0 ), VideoInput( ), NeedToSetVideoInput( false ),
            Capabilities( ), VideoPins( ), IsCrossbarAvailable( false ), InfoCollectedEvent( ),
            JpegEncodingPreferred( false ), JpegEncodingEnabled( false ),
            Listener( nullptr ), ExitEvent( ), BackgroundThread( ), Running( false ),
            FramesCounter( 0 ),
            RunningSync( ), DeviceIsRunningEvent( ), DeviceIsRunning( false ), VideoProcAmp( nullptr ), CameraControl( nullptr ),
            VideoPropertiesToSet( ), CameraPropertiesToSet( )
        {
        }

        bool Start( );
        void SignalToStop( );
        void WaitForStop( );
        bool IsRunning( );
        uint32_t FramesReceived( );
        IVideoSourceListener* SetListener( IVideoSourceListener* listener );

        // Collect information about current video device
        bool CollectInfo( );

        bool IsDeviceRunning( ) const;
        bool WaitForDeviceRunning( uint32_t msec );

        // Device's video acquisition configuration
        bool IsVideoConfigSupported( ) const;

        XError SetVideoProperty( XVideoProperty property, int32_t value, bool automatic );
        XError GetVideoProperty( XVideoProperty property, int32_t* value, bool* automatic ) const;
        XError GetVideoPropertyRange( XVideoProperty property, int32_t* min, int32_t* max, int32_t* step, int32_t* default, bool* isAutomaticSupported ) const;

        // Camera's control configuration
        bool IsCameraConfigSupported( ) const;

        XError SetCameraProperty( XCameraProperty property, int32_t value, bool automatic );
        XError GetCameraProperty( XCameraProperty property, int32_t* value, bool* automatic ) const;
        XError GetCameraPropertyRange( XCameraProperty property, int32_t* min, int32_t* max, int32_t* step, int32_t* default, bool* isAutomaticSupported ) const;

    private:
        // Run video loop in a background thread
        static void WorkerThreadHandler( void* param );

        // Notify about error in the video source
        void NotifyError( const string& errorMessage, bool fatal = false );

        // Run video acquisition loop
        void RunVideo( bool run = true );
    };

    // Implementation of sample grabber's callback interface
    class SampleGrabber : public ISampleGrabberCB
    {
    private:
        XLocalVideoDeviceData* mParent;
        shared_ptr<XImage>     mImage;
        int                    mWidth;
        int                    mHeight;
        bool                   mSSSE3supported;

    public:
        SampleGrabber( XLocalVideoDeviceData* parent ) : mParent( parent ), mWidth( 0 ), mHeight( 0 )
        {
            int CPUInfo[4];

            __cpuid( CPUInfo, 1 );

            mSSSE3supported = ( ( CPUInfo[2] & ( 1 << 9 ) ) != 0 );
        }

        virtual ~SampleGrabber( ) { };

        // Set size of video frames provided by Sample Grabber
        void SetVideoSize( int width, int height )
        {
            mWidth  = width;
            mHeight = height;
        }

        // IUnknown's query interface - provide only sample grabber interface
        HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** ppvObject )
        {
            if ( riid == IID_ISampleGrabberCB )
            {
                *ppvObject = static_cast<ISampleGrabberCB*>( this );
                return S_OK;
            }
            return E_NOTIMPL;
        };

        // Do nothing with reference counter, since we control object's life time ourself
        ULONG STDMETHODCALLTYPE AddRef( void )  { return 0; }
        ULONG STDMETHODCALLTYPE Release( void ) { return 0; }

        // Don't support grabbing media samples
        HRESULT STDMETHODCALLTYPE SampleCB( double /* sampleTime */ , IMediaSample* /* pSample */ )
        {
            return E_NOTIMPL;
        }

        // Handle new video sample given in a buffer
        HRESULT STDMETHODCALLTYPE BufferCB( double /* sampleTime */, BYTE* pBuffer, long bufferLen )
        {
            lock_guard<recursive_mutex> lock( mParent->Sync );

            assert( mWidth );
            assert( mHeight );

            // check thread is still running and not signalled to stop
            if ( !mParent->ExitEvent.IsSignaled( ) )
            {
                mParent->FramesCounter++;
            }

            // if there is a valid buffer and a callback, then create an image and give it to user
            // otherwise don't bother wasting time
            if ( ( mParent->Listener != nullptr ) && ( pBuffer != nullptr ) && ( bufferLen != 0 ) )
            {
                if ( !mParent->JpegEncodingEnabled )
                {
                    if ( ( !mImage ) || ( mImage->Format( ) != XPixelFormat::RGB24 ) || ( mImage->Width( ) != mWidth ) || ( mImage->Height( ) != mHeight ) )
                    {
                        mImage = XImage::Allocate( mWidth, mHeight, XPixelFormat::RGB24 );
                    }

                    if ( mImage )
                        {
                        int      dstStride = mImage->Stride( );
                        int      srcStride = bufferLen / mHeight;
                        uint8_t* dst       = mImage->Data( ) + dstStride * ( mHeight - 1 );

                        /*
                        if ( BlueIndex == 0 )
                        {
                            int toCopy = mWidth * 3;

                            for ( int y = 0; y < mHeight; y++ )
                            {
                                memcpy( dst - y * dstStride, pBuffer + y * srcStride, toCopy );
                            }
                        }
                        else
                        */
                        {
                            if ( !mSSSE3supported )
                            {
                                for ( int y = 0; y < mHeight; y++ )
                                {
                                    uint8_t* dstRow = dst - y * dstStride;
                                    uint8_t* srcRow = pBuffer + y * srcStride;

                                    for ( int x = 0; x < mWidth; x++, dstRow += 3, srcRow += 3 )
                                    {
                                        dstRow[BlueIndex]  = srcRow[0];
                                        dstRow[GreenIndex] = srcRow[1];
                                        dstRow[RedIndex]   = srcRow[2];
                                    }
                                }
                            }
                            else
                            {
                                int packs = mWidth / 16;
                                int rem   = mWidth % 16;

                                __m128i swapIndeces_0      = _mm_set_epi8( -1, 12, 13, 14,  9, 10, 11,  6,  7,  8,  3,  4,  5,  0,  1,  2 );
                                __m128i swapIndeces_1      = _mm_set_epi8( 15, -1, 11, 12, 13,  8,  9, 10,  5,  6,  7,  2,  3,  4, -1,  0 );
                                __m128i swapIndeces_2      = _mm_set_epi8( 13, 14, 15, 10, 11, 12,  7,  8,  9,  4,  5,  6,  1,  2,  3, -1 );
                                __m128i chunk0IndecesFrom1 = _mm_set_epi8(  1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 );
                                __m128i chunk2IndecesFrom1 = _mm_set_epi8( -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 14 );
                                __m128i chunk1IndecesFrom0 = _mm_set_epi8( -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, -1 );
                                __m128i chunk1IndecesFrom2 = _mm_set_epi8( -1,  0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 );

                                for ( int y = 0; y < mHeight; y++ )
                                {
                                    uint8_t* dstRow = dst - y * dstStride;
                                    uint8_t* srcRow = pBuffer + y * srcStride;
                                    __m128i  chunk0, chunk1, chunk2;

                                    for ( int x = 0; x < packs; x++, srcRow += 48, dstRow += 48 )
                                    {
                                        chunk0 = _mm_loadu_si128( (__m128i*) srcRow );
                                        chunk1 = _mm_loadu_si128( (__m128i*) ( srcRow + 16 ) );
                                        chunk2 = _mm_loadu_si128( (__m128i*) ( srcRow + 32 ) );

                                        _mm_storeu_si128( (__m128i*) dstRow,
                                            _mm_or_si128( _mm_shuffle_epi8( chunk0, swapIndeces_0 ),
                                                          _mm_shuffle_epi8( chunk1, chunk0IndecesFrom1 ) ) );

                                        _mm_storeu_si128( (__m128i*) ( dstRow + 16 ),
                                            _mm_or_si128(
                                            _mm_or_si128( _mm_shuffle_epi8( chunk1, swapIndeces_1 ),
                                                          _mm_shuffle_epi8( chunk0, chunk1IndecesFrom0 ) ),
                                                          _mm_shuffle_epi8( chunk2, chunk1IndecesFrom2 ) ) );

                                        _mm_storeu_si128( (__m128i*) ( dstRow + 32 ),
                                            _mm_or_si128( _mm_shuffle_epi8( chunk2, swapIndeces_2 ),
                                                          _mm_shuffle_epi8( chunk1, chunk2IndecesFrom1 ) ) );
                                    }

                                    for ( int x = 0; x < rem; x++, dstRow += 3, srcRow += 3 )
                                    {
                                        dstRow[BlueIndex]  = srcRow[0];
                                        dstRow[GreenIndex] = srcRow[1];
                                        dstRow[RedIndex]   = srcRow[2];
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    shared_ptr<XImage> srcImage = XImage::Create( pBuffer, bufferLen, 1, bufferLen, XPixelFormat::JPEG );

                    if ( srcImage )
                    {
                        srcImage->CopyDataOrClone( mImage );
                    }
                }

                mParent->Listener->OnNewImage( mImage );
            }

            return 0;
        }
    };
} // namespace Private

// ==========================================================================

// Class constructor
XLocalVideoDevice::XLocalVideoDevice( const string& deviceMoniker ) :
    mData( new Private::XLocalVideoDeviceData( deviceMoniker ) )
{
}

// Class destructor
XLocalVideoDevice::~XLocalVideoDevice( )
{
    delete mData;
}

// Create class instance
const shared_ptr<XLocalVideoDevice> XLocalVideoDevice::Create( const string& deviceMoniker )
{
    return shared_ptr<XLocalVideoDevice>( new XLocalVideoDevice( deviceMoniker ) );
}

// Set video source listener
IVideoSourceListener* XLocalVideoDevice::SetListener( IVideoSourceListener* listener )
{
    return mData->SetListener( listener );
}

// Set device moniker for the video source (video source must not be running)
bool XLocalVideoDevice::SetDeviceMoniker( const string& moniker )
{
    lock_guard<recursive_mutex> lock( mData->Sync );
    bool ret = false;

    // set new device moniker only if the device is not running
    if ( !IsRunning( ) )
    {
        mData->DeviceMoniker = moniker;

        // reset collected information about the device
        mData->Capabilities.clear( );
        mData->VideoPins.clear( );
        mData->IsCrossbarAvailable = false;
        mData->InfoCollectedEvent.Reset( );

        // reset resolution and input
        mData->Resolution = XDeviceCapabilities( );
        mData->VideoInput = XDevicePinInfo( );

        ret = true;
    }

    return ret;
}

// Set resolution and frame rate of the device (video source must not be running)
bool XLocalVideoDevice::SetResolution( const XDeviceCapabilities& resolution, uint32_t requestedFps )
{
    lock_guard<recursive_mutex> lock( mData->Sync );
    bool ret = false;

    if ( !IsRunning( ) )
    {
        mData->Resolution   = resolution;
        mData->RequestedFps = requestedFps;
        ret = true;
    }

    return ret;
}

// Set video input of the device
void XLocalVideoDevice::SetVideoInput( const XDevicePinInfo& input )
{
    mData->VideoInput = input;
    mData->NeedToSetVideoInput = true;
}

// Get JPEG encoding preference
bool XLocalVideoDevice::IsJpegEncodingPreferred( ) const
{
    return mData->JpegEncodingPreferred;
}

// Set JPEG encoding preference
void XLocalVideoDevice::PreferJpegEncoding( bool prefer )
{
    mData->JpegEncodingPreferred = prefer;
}

// Check if running device was configured to provide JPEG images. Always returns
// false for not running devices. NOTE: provided image pixel format already will indicate
// this, but having extra property may be handy.
bool XLocalVideoDevice::IsJpegEncodingEnabled( ) const
{
    return mData->JpegEncodingEnabled;
}

// Start video device (start its background thread which manages video acquisition)
bool XLocalVideoDevice::Start( )
{
    return mData->Start( );
}

// Signal video source to stop and finalize its background thread
void XLocalVideoDevice::SignalToStop( )
{
    mData->SignalToStop( );
}

// Wait till video source stops
void XLocalVideoDevice::WaitForStop( )
{
    mData->WaitForStop( );
}

// Check if video source (its background thread) is running or not
bool XLocalVideoDevice::IsRunning( )
{
    return mData->IsRunning( );
}

// Get number of frames received since the the start of the video source
uint32_t XLocalVideoDevice::FramesReceived( )
{
    return mData->FramesReceived( );
}

// Get capabilities of the device (resolutions and frame rates)
const vector<XDeviceCapabilities> XLocalVideoDevice::GetCapabilities( )
{
    lock_guard<recursive_mutex> lock( mData->Sync );
    vector<XDeviceCapabilities> capabilities;

    if ( mData->CollectInfo( ) )
    {
        capabilities = mData->Capabilities;
    }

    return capabilities;
}

// Get available video pins of the device if any
const vector<XDevicePinInfo> XLocalVideoDevice::GetInputVideoPins( )
{
    lock_guard<recursive_mutex> lock( mData->Sync );
    vector<XDevicePinInfo> videoPins;

    if ( mData->CollectInfo( ) )
    {
        videoPins = mData->VideoPins;
    }

    return videoPins;
}

// Check if device supports crossbar, which would allow setting video input pin
bool XLocalVideoDevice::IsCrossbarSupported( )
{
    lock_guard<recursive_mutex> lock( mData->Sync );
    bool ret = false;

    if ( mData->CollectInfo( ) )
    {
        ret = mData->IsCrossbarAvailable;
    }

    return ret;
}

// Get list of available video devices in the system
vector<XDeviceName> XLocalVideoDevice::GetAvailableDevices( )
{
    vector<XDeviceName> devices;
    bool needToTerminateCOM = ( CoInitializeEx( nullptr, COINIT_MULTITHREADED ) != RPC_E_CHANGED_MODE );

    IMalloc* pMalloc = nullptr;
    HRESULT  hr      = CoGetMalloc( 1, &pMalloc );

    if ( SUCCEEDED( hr ) )
    {
        ICreateDevEnum* pSysDevEnum = nullptr;

        // create System Device Enumerator
        hr = CoCreateInstance( CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER,
                               IID_ICreateDevEnum, (void **) &pSysDevEnum );

        if ( SUCCEEDED( hr ) )
        {
            // obtain a class enumerator for the video device category
            IEnumMoniker* pEnumCat = nullptr;
            hr = pSysDevEnum->CreateClassEnumerator( CLSID_VideoInputDeviceCategory, &pEnumCat, 0 );

            if  ( ( SUCCEEDED( hr ) ) && ( pEnumCat != 0 ) )
            {
                // enumerate the monikers
                IMoniker* pMoniker = nullptr;
                ULONG     cFetched;

                while( pEnumCat->Next( 1, &pMoniker, &cFetched ) == S_OK )
                {
                    string   moniker, name;
                    LPOLESTR displayName;

                    // get moniker string
                    if ( SUCCEEDED( pMoniker->GetDisplayName( nullptr, nullptr, &displayName ) ) )
                    {
                        moniker = Private::Utf16to8( displayName );
                        pMalloc->Free( displayName );
                    }

                    IPropertyBag *pPropBag;
                    hr = pMoniker->BindToStorage( 0, 0, IID_IPropertyBag, (void **) &pPropBag );

                    if ( SUCCEEDED( hr ) )
                    {
                        // retrieve the filter's friendly name
                        VARIANT varName;
                        VariantInit( &varName );

                        hr = pPropBag->Read( L"FriendlyName", &varName, 0 );
                        if ( SUCCEEDED( hr ) )
                        {
                            name = Private::Utf16to8( varName.bstrVal );
                        }

                        VariantClear( &varName );
                        pPropBag->Release( );
                    }

                    devices.push_back( XDeviceName( moniker, name ) );
                    pMoniker->Release( );
                }

                pEnumCat->Release( );
            }

            pSysDevEnum->Release( );
        }
        
        pMalloc->Release( );
    }

    if ( needToTerminateCOM )
    {
        CoUninitialize( );
    }

    return devices;
}

// Check if device is running
bool XLocalVideoDevice::IsDeviceRunning( ) const
{
    return mData->IsDeviceRunning( );
}

// Wait till video configuration becomes available (milliseconds)
bool XLocalVideoDevice::WaitForDeviceRunning( uint32_t msec )
{
    return mData->WaitForDeviceRunning( msec );
}

// Check if video configuration is supported
bool XLocalVideoDevice::IsVideoConfigSupported( ) const
{
    return mData->IsVideoConfigSupported( );
}

// Set the specified video property
XError XLocalVideoDevice::SetVideoProperty( XVideoProperty property, int32_t value, bool automatic )
{
    return mData->SetVideoProperty( property, value, automatic );
}

// Get current value if the specified video property
XError XLocalVideoDevice::GetVideoProperty( XVideoProperty property, int32_t* value, bool* automatic ) const
{
    return mData->GetVideoProperty( property, value, automatic );
}

// Get range of values supported by the specified video property
XError XLocalVideoDevice::GetVideoPropertyRange( XVideoProperty property, int32_t* min, int32_t* max, int32_t* step, int32_t* default, bool* isAutomaticSupported ) const
{
    return mData->GetVideoPropertyRange( property, min, max, step, default, isAutomaticSupported );
}

// Check if camera configuration is supported
bool XLocalVideoDevice::IsCameraConfigSupported( ) const
{
    return mData->IsCameraConfigSupported( );
}

// Set the specified camera property
XError XLocalVideoDevice::SetCameraProperty( XCameraProperty property, int32_t value, bool automatic )
{
    return mData->SetCameraProperty( property, value, automatic );
}

// Get current value if the specified camera property
XError XLocalVideoDevice::GetCameraProperty( XCameraProperty property, int32_t* value, bool* automatic ) const
{
    return mData->GetCameraProperty( property, value, automatic );
}

// Get range of values supported by the specified camera property
XError XLocalVideoDevice::GetCameraPropertyRange( XCameraProperty property, int32_t* min, int32_t* max, int32_t* step, int32_t* default, bool* isAutomaticSupported ) const
{
    return mData->GetCameraPropertyRange( property, min, max, step, default, isAutomaticSupported );
}

namespace Private
{

// Start video source so it initializes and begins providing video frames
bool XLocalVideoDeviceData::Start( )
{
    lock_guard<recursive_mutex> lock( Sync );
    bool ret = true;

    if ( DeviceMoniker.empty( ) )
    {
        ret = false;
    }
    else
    {
        if ( !IsRunning( ) )
        {
            FramesCounter = 0;
            Running       = true;
            ExitEvent.Reset( );

            // reset info collection event, so calls which require it will block and wait for it
            InfoCollectedEvent.Reset( );

            BackgroundThread = thread( WorkerThreadHandler, this );
        }
    }

    return ret;
}

// Signal video to stop, so it could finalize and clean-up
void XLocalVideoDeviceData::SignalToStop( )
{
    lock_guard<recursive_mutex> lock( Sync );

    if ( IsRunning( ) )
    {
        ExitEvent.Signal( );
    }
}

// Wait till video source (its thread) stops
void XLocalVideoDeviceData::WaitForStop( )
{
    SignalToStop( );

    if ( ( IsRunning( ) ) || ( BackgroundThread.joinable( ) ) )
    {
        BackgroundThread.join( );
    }
}

// Check if video source is still running
bool XLocalVideoDeviceData::IsRunning( )
{
    lock_guard<recursive_mutex> lock( Sync );

    if ( ( !Running ) && ( BackgroundThread.joinable( ) ) )
    {
        BackgroundThread.join( );
    }

    return Running;
}

// Get number of frames received since the the start of the video source
uint32_t XLocalVideoDeviceData::FramesReceived( )
{
    lock_guard<recursive_mutex> lock( Sync );
    return FramesCounter;
}

// Set video source listener
IVideoSourceListener* XLocalVideoDeviceData::SetListener( IVideoSourceListener* listener )
{
    lock_guard<recursive_mutex> lock( Sync );
    IVideoSourceListener*       oldListener = Listener;

    Listener = listener;

    return oldListener;
}

// Collect some information about current video device
bool XLocalVideoDeviceData::CollectInfo( )
{
    bool ret = false;

    if ( !DeviceMoniker.empty( ) )
    {
        // check if capablities/info is already available
        if ( InfoCollectedEvent.IsSignaled( ) )
        {
            ret = true;
        }
        else
        {
            // check if thread is running
            if ( Running )
            {
                // if thread is already running, we just need to wait a bit for configuration
                if ( InfoCollectedEvent.Wait( 5000 ) )
                {
                    ret = true;
                }
            }
            else
            {
                // run video graph without starting video
                RunVideo( false );
                ret = true;
            }
        }
    }

    return ret;
}


// Run video loop in a background thread
void XLocalVideoDeviceData::WorkerThreadHandler( void* param )
{
    static_cast<XLocalVideoDeviceData*>( param )->RunVideo( );
}

// Notify about error in the video source
void XLocalVideoDeviceData::NotifyError( const string& errorMessage, bool fatal )
{
    lock_guard<recursive_mutex> lock( Sync );

    if ( Listener != nullptr )
    {
        Listener->OnError( errorMessage, fatal );
    }
}

// Run video acquisition loop
void XLocalVideoDeviceData::RunVideo( bool run )
{
    ICaptureGraphBuilder2*  pCaptureGraphBuilder    = nullptr;
    IFilterGraph2*          pFilterGraph            = nullptr;
    IGraphBuilder*          pGraphBuilder           = nullptr;
    IMediaControl*          pMediaControl           = nullptr;
    IMediaEventEx*          pMediaEventEx           = nullptr;

    ISampleGrabber*         pVideoSampleGrabber     = nullptr;
    IAMCrossbar*            pCrossbar               = nullptr;

    IBaseFilter*            pSourceFilter           = nullptr;
    IBaseFilter*            pVideoGrabberFilter     = nullptr;

    SampleGrabber*          sampleGrabberCallback   = new SampleGrabber( this );
    AM_MEDIA_TYPE           mediaType;
    HRESULT                 hr;

    bool                    needToTerminateCOM      = ( CoInitializeEx( nullptr, COINIT_MULTITHREADED ) != RPC_E_CHANGED_MODE );

    do
    {
        bool basicInitializationFailed = false;

        if (
            // create DirectShow's capture graph builder
            ( SUCCEEDED( CoCreateInstance( CLSID_CaptureGraphBuilder2, nullptr, CLSCTX_INPROC_SERVER,
                                           IID_ICaptureGraphBuilder2, (void**) &pCaptureGraphBuilder ) ) ) &&
            // create filter graph
            ( SUCCEEDED( CoCreateInstance( CLSID_FilterGraph, nullptr, CLSCTX_INPROC_SERVER,
                                           IID_IFilterGraph2, (void**) &pFilterGraph ) ) ) &&
            // get some interfaces we need later
            ( SUCCEEDED( pFilterGraph->QueryInterface( IID_IMediaControl, (void**) &pMediaControl ) ) ) &&
            ( SUCCEEDED( pFilterGraph->QueryInterface( IID_IGraphBuilder, (void**) &pGraphBuilder ) ) ) &&
            // set filter graph to builder
            ( SUCCEEDED( pCaptureGraphBuilder->SetFiltergraph( pGraphBuilder ) ) ) &&
            // create sample grabber, which would allow grabbing video frames
            ( SUCCEEDED( CoCreateInstance( CLSID_SampleGrabber, nullptr, CLSCTX_INPROC_SERVER,
                                           IID_ISampleGrabber, (void **) &pVideoSampleGrabber ) ) ) &&
            ( SUCCEEDED( pVideoSampleGrabber->QueryInterface( IID_IBaseFilter, (void**) &pVideoGrabberFilter ) ) )
           )
        {
            // require setting video input again
            NeedToSetVideoInput = true;

            // create source video device
            pSourceFilter = CreateFilter( DeviceMoniker );
            if ( pSourceFilter == nullptr )
            {
                NotifyError( "Did not find video device. Unplugged?" );
            }
            else
            {
                // add source and grabber to the graph
                pFilterGraph->AddFilter( pSourceFilter, L"source" );
                pFilterGraph->AddFilter( pVideoGrabberFilter, L"grabber" );

                // get crossbar
                hr = pCaptureGraphBuilder->FindInterface( &LOOK_UPSTREAM_ONLY, nullptr, pSourceFilter,
                                                           IID_IAMCrossbar, (void**) &pCrossbar );

                if ( ( SUCCEEDED( hr ) ) && ( pCrossbar != nullptr ) )
                {
                    VideoPins = ColletCrossbarVideoInputs( pCrossbar );
                    IsCrossbarAvailable = true;
                }

                // collect supported frame sizes/rates
                Capabilities = GetPinCapabilitiesAndConfigureIt( pCaptureGraphBuilder, pSourceFilter, &PIN_CATEGORY_CAPTURE, Resolution, RequestedFps );
                InfoCollectedEvent.Signal( );

                if ( run )
                {
                    bool devicePrepared = false;

                    // get interface to check for events. don't care if it fails.
                    pFilterGraph->QueryInterface( IID_IMediaEventEx, (void**) &pMediaEventEx );

                    // check if we need and cad do JPEG encoding
                    if ( JpegEncodingPreferred )
                    {
                        JpegEncodingEnabled = IsJpegEncodingAvailable( pSourceFilter );
                    }

                    // set media type we want
                    memset( &mediaType, 0, sizeof( AM_MEDIA_TYPE ) );

                    mediaType.majortype  = MEDIATYPE_Video;
                    mediaType.subtype    = ( JpegEncodingEnabled ) ? MEDIASUBTYPE_MJPG : MEDIASUBTYPE_RGB24;
                    mediaType.formattype = GUID_NULL;
                    mediaType.pbFormat   = nullptr;
                    mediaType.cbFormat   = 0;
                    mediaType.pUnk       = nullptr;

                    hr = pVideoSampleGrabber->SetMediaType( &mediaType );
                    if ( SUCCEEDED( hr ) )
                    {
                        // configure video sample grabber
                        pVideoSampleGrabber->SetBufferSamples( false );
                        pVideoSampleGrabber->SetOneShot( false );
                        pVideoSampleGrabber->SetCallback( sampleGrabberCallback, 1 );

                        // prepare rendering the graph
                        hr = pCaptureGraphBuilder->RenderStream( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pSourceFilter, nullptr, pVideoGrabberFilter );
                        if ( SUCCEEDED( hr ) )
                        {
                            // resolve video size
                            hr = pVideoSampleGrabber->GetConnectedMediaType( &mediaType );
                            if ( SUCCEEDED( hr ) )
                            {
                                if ( ( mediaType.formattype == FORMAT_VideoInfo ) ||
                                     ( mediaType.formattype == FORMAT_VideoInfo2 ) )
                                {
                                    if ( mediaType.formattype == FORMAT_VideoInfo )
                                    {
                                        VIDEOINFOHEADER* pVideoInfo = (VIDEOINFOHEADER*) mediaType.pbFormat;
                                        sampleGrabberCallback->SetVideoSize( pVideoInfo->bmiHeader.biWidth, pVideoInfo->bmiHeader.biHeight );
                                    }
                                    else if ( mediaType.formattype == FORMAT_VideoInfo2 )
                                    {
                                        VIDEOINFOHEADER2* pVideoInfo = (VIDEOINFOHEADER2*) mediaType.pbFormat;
                                        sampleGrabberCallback->SetVideoSize( pVideoInfo->bmiHeader.biWidth, pVideoInfo->bmiHeader.biHeight );
                                    }

                                    devicePrepared = true;

                                    // run the graph
                                    hr = pMediaControl->Run( );
                                    if ( FAILED( hr ) )
                                    {
                                        NotifyError( "Failed runnning device. Might be busy." );
                                    }
                                    else
                                    {
                                        // signal we are running and also check if we can do video configuration
                                        {
                                            lock_guard<recursive_mutex> lock( RunningSync );

                                            DeviceIsRunning = true;
                                            DeviceIsRunningEvent.Signal( );

                                            // get interface to control image adjustments
                                            hr = pSourceFilter->QueryInterface( IID_IAMVideoProcAmp, (void**) &VideoProcAmp );
                                            if ( ( SUCCEEDED( hr ) ) && ( VideoProcAmp != nullptr ) )
                                            {
                                                bool configOK = true;

                                                // configure all video properties, which were set before device got running
                                                for ( auto property : VideoPropertiesToSet )
                                                {
                                                    configOK &= SetVideoProperty( property.first, property.second.first, property.second.second );
                                                }
                                                VideoPropertiesToSet.clear( );

                                                if ( !configOK )
                                                {
                                                    NotifyError( "Failed applying video configuration" );
                                                }
                                            }

                                            // get interface to control camera
                                            hr = pSourceFilter->QueryInterface( IID_IAMCameraControl, (void**) &CameraControl );
                                            if ( ( SUCCEEDED( hr ) ) && ( CameraControl != nullptr ) )
                                            {
                                                bool configOK = true;

                                                // configure all camera properties, which were set before device got running
                                                for ( auto property : CameraPropertiesToSet )
                                                {
                                                    configOK &= SetCameraProperty( property.first, property.second.first, property.second.second );
                                                }
                                                CameraPropertiesToSet.clear( );

                                                if ( !configOK )
                                                {
                                                    NotifyError( "Failed applying camera configuration" );
                                                }
                                            }
                                        }

                                        // loop waiting for signal to stop and collecting some media events
                                        while ( !ExitEvent.Wait( 100 ) )
                                        {
                                            if ( pMediaEventEx != nullptr )
                                            {
                                                long     eventCode;
                                                LONG_PTR param1, param2;

                                                hr = pMediaEventEx->GetEvent( &eventCode, &param1, &param2, 0 );

                                                if ( SUCCEEDED( hr ) )
                                                {
                                                    pMediaEventEx->FreeEventParams( eventCode, param1, param2 );

                                                    if ( eventCode == EC_DEVICE_LOST )
                                                    {
                                                        NotifyError( "Device was lost. Unplugged?" );
                                                        break;
                                                    }
                                                }
                                            }

                                            if ( NeedToSetVideoInput )
                                            {
                                                NeedToSetVideoInput = false;

                                                if ( IsCrossbarAvailable )
                                                {
                                                    SetCurrentCrossbarInput( pCrossbar, VideoInput );
                                                    VideoInput = GetCurrentCrossbarInput( pCrossbar );
                                                }
                                            }
                                        }

                                        // signal we are no longer running and release video configuration interface
                                        {
                                            lock_guard<recursive_mutex> lock( RunningSync );
                                            RELEASE_COM( VideoProcAmp );
                                            RELEASE_COM( CameraControl );
                                            DeviceIsRunning = false;
                                            DeviceIsRunningEvent.Reset( );
                                        }

                                        // stop the capture graph
                                        pMediaControl->Stop( );
                                    }
                                }

                                FreeMediaType( mediaType );
                            }
                        }
                    }

                    if ( !devicePrepared )
                    {
                        NotifyError( "Failed preparing device to run." );
                    }

                    RELEASE_COM( pMediaEventEx );
                }

                pFilterGraph->RemoveFilter( pVideoGrabberFilter );
                pFilterGraph->RemoveFilter( pSourceFilter );
                RELEASE_COM( pCrossbar );
                RELEASE_COM( pSourceFilter );
            }
        }
        else
        {
            basicInitializationFailed = true;
        }

        // release all COM interfaces we own
        RELEASE_COM( pVideoGrabberFilter );
        RELEASE_COM( pVideoSampleGrabber );
        RELEASE_COM( pGraphBuilder );
        RELEASE_COM( pMediaControl );
        RELEASE_COM( pFilterGraph );
        RELEASE_COM( pCaptureGraphBuilder );

        // check if we failed on something basic
        if ( basicInitializationFailed )
        {
            NotifyError( "Failed DirectShow initialization", true );
            break;
        }
    }
    while ( ( run ) && ( !ExitEvent.Wait( 1000 ) ) );

    // set info collection event even if we failed, so another thread does not wait too long for info
    InfoCollectedEvent.Signal( );

    delete sampleGrabberCallback;

    if ( needToTerminateCOM )
    {
        CoUninitialize( );
    }

    {
        lock_guard<recursive_mutex> lock( Sync );
        Running = false;
        JpegEncodingEnabled = false;
    }
}

// Check if device is running
bool XLocalVideoDeviceData::IsDeviceRunning( ) const
{
    lock_guard<recursive_mutex> lock( RunningSync );
    return DeviceIsRunning;
}

// Wait till video configuration becomes available (milliseconds)
bool XLocalVideoDeviceData::WaitForDeviceRunning( uint32_t msec )
{
    return DeviceIsRunningEvent.Wait( msec );
}

// Check if video configuration is supported
bool XLocalVideoDeviceData::IsVideoConfigSupported( ) const
{
    // user is responsible to make sure device is running
    lock_guard<recursive_mutex> lock( RunningSync );
    return ( VideoProcAmp != nullptr );
}

static const VideoProcAmpProperty nativeVideoProperties[] =
{
    VideoProcAmp_Brightness,
    VideoProcAmp_Contrast,
    VideoProcAmp_Hue,
    VideoProcAmp_Saturation,
    VideoProcAmp_Sharpness,
    VideoProcAmp_Gamma,
    VideoProcAmp_ColorEnable,
    VideoProcAmp_WhiteBalance,
    VideoProcAmp_BacklightCompensation,
    VideoProcAmp_Gain
};

// Set the specified video property
XError XLocalVideoDeviceData::SetVideoProperty( XVideoProperty property, int32_t value, bool automatic )
{
    lock_guard<recursive_mutex> lock( RunningSync );
    XError                      ret = XError::Success;

    if ( ( property < XVideoProperty::Brightness ) || ( property > XVideoProperty::Gain ) )
    {
        ret = XError::UnknownProperty;
    }
    else if ( !DeviceIsRunning )
    {
        // save property value and try setting it when device gets runnings
        VideoPropertiesToSet[property] = PropValue( value, automatic );
    }
    else if ( VideoProcAmp == nullptr )
    {
        ret = XError::ConfigurationNotSupported;
    }
    else
    {
        HRESULT hr = VideoProcAmp->Set( nativeVideoProperties[static_cast<int>( property )], value, ( automatic ) ? VideoProcAmp_Flags_Auto : VideoProcAmp_Flags_Manual );

        if ( FAILED( hr ) )
        {
            ret = ( hr == E_PROP_ID_UNSUPPORTED ) ? XError::UnsupportedProperty : XError::Failed;
        }
    }

    return ret;
}

// Get current value if the specified video property
XError XLocalVideoDeviceData::GetVideoProperty( XVideoProperty property, int32_t* value, bool* automatic ) const
{
    lock_guard<recursive_mutex> lock( RunningSync );
    XError                      ret = XError::Success;

    if ( value == nullptr )
    {
        ret = XError::NullPointer;
    }
    else if ( ( property < XVideoProperty::Brightness ) || ( property > XVideoProperty::Gain ) )
    {
        ret = XError::UnknownProperty;
    }
    else if ( !DeviceIsRunning )
    {
        ret = XError::DeivceNotReady;
    }
    else if ( VideoProcAmp == nullptr )
    {
        ret = XError::ConfigurationNotSupported;
    }
    else
    {
        long    propValue = 0, propFlags = 0;
        HRESULT hr = VideoProcAmp->Get( nativeVideoProperties[static_cast<int>( property )], &propValue, &propFlags );

        if ( FAILED( hr ) )
        {
            ret = ( hr == E_PROP_ID_UNSUPPORTED ) ? XError::UnsupportedProperty : XError::Failed;
        }
        else
        {
            *value = propValue;

            if ( automatic != nullptr )
            {
                *automatic = ( propFlags == VideoProcAmp_Flags_Auto );
            }
        }
    }

    return ret;
}

// Get range of values supported by the specified video property
XError XLocalVideoDeviceData::GetVideoPropertyRange( XVideoProperty property, int32_t* min, int32_t* max, int32_t* step, int32_t* default, bool* isAutomaticSupported ) const
{
    lock_guard<recursive_mutex> lock( RunningSync );
    XError                      ret = XError::Success;

    if ( ( min == nullptr ) || ( max == nullptr ) || ( step == nullptr ) || ( default == nullptr ) || ( isAutomaticSupported == nullptr ) )
    {
        ret = XError::NullPointer;
    }
    else if ( ( property < XVideoProperty::Brightness ) || ( property > XVideoProperty::Gain ) )
    {
        ret = XError::UnknownProperty;
    }
    else if ( !DeviceIsRunning )
    {
        ret = XError::DeivceNotReady;
    }
    else if ( VideoProcAmp == nullptr )
    {
        ret = XError::ConfigurationNotSupported;
    }
    else
    {
        long    propMin = 0, propMax = 0, propStep = 0, propDef = 0, propFlags = 0;
        HRESULT hr = VideoProcAmp->GetRange( nativeVideoProperties[static_cast<int>( property )], &propMin, &propMax, &propStep, &propDef, &propFlags );

        if ( FAILED( hr ) )
        {
            ret = ( hr == E_PROP_ID_UNSUPPORTED ) ? XError::UnsupportedProperty : XError::Failed;
        }
        else
        {
            *min     = propMin;
            *max     = propMax;
            *step    = propStep;
            *default = propDef;

            *isAutomaticSupported = ( propFlags & VideoProcAmp_Flags_Auto );
        }
    }

    return ret;
}

// Check if camera configuration is supported
bool XLocalVideoDeviceData::IsCameraConfigSupported( ) const
{
    // user is responsible to make sure device is running
    lock_guard<recursive_mutex> lock( RunningSync );
    return ( CameraControl != nullptr );
}

static const CameraControlProperty nativeCameraProperties[] =
{
    CameraControl_Pan,
    CameraControl_Tilt,
    CameraControl_Roll,
    CameraControl_Zoom,
    CameraControl_Exposure,
    CameraControl_Iris,
    CameraControl_Focus
};

// Set the specified camera property
XError XLocalVideoDeviceData::SetCameraProperty( XCameraProperty property, int32_t value, bool automatic )
{
    lock_guard<recursive_mutex> lock( RunningSync );
    XError                      ret = XError::Success;

    if ( ( property < XCameraProperty::Pan ) || ( property > XCameraProperty::Focus ) )
    {
        ret = XError::UnknownProperty;
    }
    else if ( !DeviceIsRunning )
    {
        // save property value and try setting it when device gets runnings
        CameraPropertiesToSet[property] = PropValue( value, automatic );
    }
    else if ( CameraControl == nullptr )
    {
        ret = XError::ConfigurationNotSupported;
    }
    else
    {
        HRESULT hr = CameraControl->Set( nativeCameraProperties[static_cast<int>( property )], value, ( automatic ) ? CameraControl_Flags_Auto : CameraControl_Flags_Manual );

        // !!! NEED MORE DIGING !!!
        // Error 0x8007001F - A device attached to the system is not functioning
        //
        // Don't treat 0x8007001 as error. Found that setting camera property (exposure) to
        // automatic control always generates this error, but the setting is accepted and is
        // controlled automatically then. Setting same property to manual control does not generate
        // this error code. Might be camera/driver specific.
        if ( ( static_cast<uint32_t>( hr ) != 0x8007001F ) && ( FAILED( hr ) ) )
        {
            ret = ( hr == E_PROP_ID_UNSUPPORTED ) ? XError::UnsupportedProperty : XError::Failed;
        }
    }

    return ret;
}

// Get current value if the specified camera property
XError XLocalVideoDeviceData::GetCameraProperty( XCameraProperty property, int32_t* value, bool* automatic ) const
{
    lock_guard<recursive_mutex> lock( RunningSync );
    XError                      ret = XError::Success;

    if ( value == nullptr )
    {
        ret = XError::NullPointer;
    }
    else if ( ( property < XCameraProperty::Pan ) || ( property > XCameraProperty::Focus ) )
    {
        ret = XError::UnknownProperty;
    }
    else if ( !DeviceIsRunning )
    {
        ret = XError::DeivceNotReady;
    }
    else if ( CameraControl == nullptr )
    {
        ret = XError::ConfigurationNotSupported;
    }
    else
    {
        long    propValue = 0, propFlags = 0;
        HRESULT hr = CameraControl->Get( nativeCameraProperties[static_cast<int>( property )], &propValue, &propFlags );

        if ( FAILED( hr ) )
        {
            ret = ( hr == E_PROP_ID_UNSUPPORTED ) ? XError::UnsupportedProperty : XError::Failed;
        }
        else
        {
            *value = propValue;

            if ( automatic != nullptr )
            {
                *automatic = ( propFlags == CameraControl_Flags_Auto );
            }
        }
    }

    return ret;
}

// Get range of values supported by the specified camera property
XError XLocalVideoDeviceData::GetCameraPropertyRange( XCameraProperty property, int32_t* min, int32_t* max, int32_t* step, int32_t* default, bool* isAutomaticSupported ) const
{
    lock_guard<recursive_mutex> lock( RunningSync );
    XError                      ret = XError::Success;

    if ( ( min == nullptr ) || ( max == nullptr ) || ( step == nullptr ) || ( default == nullptr ) || ( isAutomaticSupported == nullptr ) )
    {
        ret = XError::NullPointer;
    }
    else if ( ( property < XCameraProperty::Pan ) || ( property > XCameraProperty::Focus ) )
    {
        ret = XError::UnknownProperty;
    }
    else if ( !DeviceIsRunning )
    {
        ret = XError::DeivceNotReady;
    }
    else if ( CameraControl == nullptr )
    {
        ret = XError::ConfigurationNotSupported;
    }
    else
    {
        long    propMin = 0, propMax = 0, propStep = 0, propDef = 0, propFlags = 0;
        HRESULT hr = CameraControl->GetRange( nativeCameraProperties[static_cast<int>( property )], &propMin, &propMax, &propStep, &propDef, &propFlags );

        if ( FAILED( hr ) )
        {
            ret = ( hr == E_PROP_ID_UNSUPPORTED ) ? XError::UnsupportedProperty : XError::Failed;
        }
        else
        {
            *min     = propMin;
            *max     = propMax;
            *step    = propStep;
            *default = propDef;

            *isAutomaticSupported = ( propFlags & CameraControl_Flags_Auto );
        }
    }

    return ret;
}

// Set video input of the provided crossbar's interface
void SetCurrentCrossbarInput( IAMCrossbar* pCrossbar, const XDevicePinInfo& videoInput )
{
    if ( ( pCrossbar != nullptr ) && ( videoInput.Type( ) != PinType::Unknown ) && ( videoInput.IsInput( ) ) )
    {
        long outPinsCount = 0, inPinsCount = 0;

        pCrossbar->get_PinCounts( &outPinsCount, &inPinsCount );

        long videoOutputPinIndex = -1;
        long videoInputPinIndex  = -1;
        long pinIndexRelated;
        long pinType;

        // find index of the video output pin
        for ( long i = 0; i < outPinsCount; i++ )
        {
            if ( FAILED( pCrossbar->get_CrossbarPinInfo( FALSE, i, &pinIndexRelated, &pinType ) ) )
                continue;

            if ( pinType == PhysConn_Video_VideoDecoder )
            {
                videoOutputPinIndex = i;
                break;
            }
        }

        // find index of the required input pin
        for ( long i = 0; i < inPinsCount; i++ )
        {
            if ( FAILED( pCrossbar->get_CrossbarPinInfo( TRUE, i, &pinIndexRelated, &pinType ) ) )
                continue;

            if ( ( pinType == static_cast<long>( videoInput.Type( ) ) ) && ( i == videoInput.Index( ) ) )
            {
                videoInputPinIndex = i;
                break;
            }
        }

        // try connecting pins
        if ( ( videoInputPinIndex != -1 ) && ( videoOutputPinIndex != -1 ) &&
             ( SUCCEEDED( pCrossbar->CanRoute( videoOutputPinIndex, videoInputPinIndex ) ) ) )
        {
            pCrossbar->Route( videoOutputPinIndex, videoInputPinIndex );
        }
    }
}

// Get current video input of the provided crossbar's interface
XDevicePinInfo GetCurrentCrossbarInput( IAMCrossbar* pCrossbar )
{
    XDevicePinInfo videoInput;

    if ( pCrossbar != nullptr )
    {
        long outPinsCount = 0, inPinsCount = 0;

        pCrossbar->get_PinCounts( &outPinsCount, &inPinsCount );

        long videoOutputPinIndex = -1;
        long pinIndexRelated;
        long pinType;

        // find index of the video output pin
        for ( long i = 0; i < outPinsCount; i++ )
        {
            if ( FAILED( pCrossbar->get_CrossbarPinInfo( FALSE, i, &pinIndexRelated, &pinType ) ) )
                continue;

            if ( pinType == PhysConn_Video_VideoDecoder )
            {
                videoOutputPinIndex = i;
                break;
            }
        }

        if ( videoOutputPinIndex != -1 )
        {
            long videoInputPinIndex = -1;

            // get index of the input pin connected to the output
            if ( SUCCEEDED( pCrossbar->get_IsRoutedTo( videoOutputPinIndex, &videoInputPinIndex ) ) )
            {
                if ( SUCCEEDED( pCrossbar->get_CrossbarPinInfo( TRUE, videoInputPinIndex, &pinIndexRelated, &pinType ) ) )
                {
                    videoInput = XDevicePinInfo( videoInputPinIndex, static_cast<PinType>( pinType ) );
                }
            }
        }
    }

    return videoInput;
}

// Collect available video input pins of the crossbar
vector<XDevicePinInfo> ColletCrossbarVideoInputs( IAMCrossbar* pCrossbar )
{
    vector<XDevicePinInfo> videoPins;

    if ( pCrossbar != nullptr )
    {
        long outPinsCount = 0, inPinsCount = 0;
        long pinIndexRelated;
        long pinType;

        pCrossbar->get_PinCounts( &outPinsCount, &inPinsCount );

        for ( long i = 0; i < inPinsCount; i++ )
        {
            if ( FAILED( pCrossbar->get_CrossbarPinInfo( TRUE, i, &pinIndexRelated, &pinType ) ) )
                continue;

            if ( pinType < PhysConn_Audio_Tuner )
            {
                videoPins.push_back( XDevicePinInfo( i, static_cast<PinType>( pinType ) ) );
            }
        }
    }

    return videoPins;
}

// Helper function to set AvgTimePerFrame for the specified media type
static void OverrideAverageTimePerFrame( AM_MEDIA_TYPE* pMediaType, uint32_t requestedFps )
{
    if ( ( pMediaType != nullptr ) && ( requestedFps != 0 ) )
    {
        if ( pMediaType->formattype == FORMAT_VideoInfo )
        {
            ( (VIDEOINFOHEADER*) pMediaType->pbFormat )->AvgTimePerFrame = 10000000LL / requestedFps;
        }
        else if ( pMediaType->formattype == FORMAT_VideoInfo2 )
        {
            ( (VIDEOINFOHEADER2*) pMediaType->pbFormat )->AvgTimePerFrame = 10000000LL / requestedFps;
        }
    }
}

// Enumerate specified category/type filter's capabilities and set its media type to one of those capabilities
vector<XDeviceCapabilities> GetPinCapabilitiesAndConfigureIt( ICaptureGraphBuilder2* pCaptureGraphBuilder,
                                                              IBaseFilter* pBaseFilter, const GUID* pCategory,
                                                              const XDeviceCapabilities& capToSet,
                                                              uint32_t requestedFps )
{
    vector<XDeviceCapabilities> capabilites;
    IAMStreamConfig*            pStreamConfig = nullptr;

    // find stream configuration interface for a filter of specified category/type
    HRESULT hr = pCaptureGraphBuilder->FindInterface( pCategory, &MEDIATYPE_Video, pBaseFilter, IID_IAMStreamConfig, (void**) &pStreamConfig );

    if ( ( SUCCEEDED( hr ) ) && ( pStreamConfig != 0 ) )
    {
        int                      count = 0, size = 0;
        VIDEO_STREAM_CONFIG_CAPS caps;
        AM_MEDIA_TYPE*           pMediaType      = nullptr;
        AM_MEDIA_TYPE*           pExactMediaType = nullptr;
        AM_MEDIA_TYPE*           pCloseMediaType = nullptr;

        pStreamConfig->GetNumberOfCapabilities( &count, &size );

        // iterate through all capablities
        for ( int i = 0; i < count; i++ )
        {
            hr = pStreamConfig->GetStreamCaps( i, &pMediaType, (BYTE*) &caps );

            if ( ( SUCCEEDED( hr ) ) && ( pMediaType != 0 ) )
            {
                bool isUseful = false;
                XDeviceCapabilities xcap;

                // check only video capabilities
                if ( pMediaType->formattype == FORMAT_VideoInfo )
                {
                    VIDEOINFOHEADER* pVideoInfo = (VIDEOINFOHEADER*) pMediaType->pbFormat;

                    xcap = XDeviceCapabilities(
                        pVideoInfo->bmiHeader.biWidth, pVideoInfo->bmiHeader.biHeight,
                        pVideoInfo->bmiHeader.biBitCount,
                        (int) ( 10000000LL / pVideoInfo->AvgTimePerFrame ),
                        (int) ( 10000000LL / caps.MinFrameInterval ),
                        (int) ( 10000000LL / caps.MaxFrameInterval ) );
                }
                else if ( pMediaType->formattype == FORMAT_VideoInfo2 )
                {
                    VIDEOINFOHEADER2* pVideoInfo = (VIDEOINFOHEADER2*) pMediaType->pbFormat;

                    xcap = XDeviceCapabilities(
                        pVideoInfo->bmiHeader.biWidth, pVideoInfo->bmiHeader.biHeight,
                        pVideoInfo->bmiHeader.biBitCount,
                        (int) ( 10000000LL / pVideoInfo->AvgTimePerFrame ),
                        (int) ( 10000000LL / caps.MinFrameInterval ),
                        (int) ( 10000000LL / caps.MaxFrameInterval ) );
                }

                // TODO: ignore 12 bpp format since graph fails to start playing that
                if ( ( xcap.Width( ) != 0 ) && ( xcap.BitCount( ) > 12 ) )
                {
                    // searching in vector is slow, but we don't expect it to be big
                    if ( find( capabilites.begin( ), capabilites.end( ), xcap ) == capabilites.end( ) )
                    {
                        capabilites.push_back( xcap );
                    }

                    if ( ( xcap == capToSet ) && ( pExactMediaType == 0 ) )
                    {
                        pExactMediaType = pMediaType;
                        isUseful        = true;
                    }
                    else if ( ( xcap.Width( )   == capToSet.Width( ) ) &&
                              ( xcap.Height( )  == capToSet.Height( ) ) &&
                              ( pCloseMediaType == 0 ) )
                    {
                        pCloseMediaType = pMediaType;
                        isUseful        = true;
                    }
                }

                if ( !isUseful )
                {
                    // free media type if it is no longer of use
                    DeleteMediaType( pMediaType );
                }
                pMediaType = nullptr;
            }
        }

        if ( pExactMediaType != nullptr )
        {
            OverrideAverageTimePerFrame( pExactMediaType, requestedFps );
            pStreamConfig->SetFormat( pExactMediaType );
        }
        else if ( pCloseMediaType != nullptr )
        {
            OverrideAverageTimePerFrame( pCloseMediaType, requestedFps );
            pStreamConfig->SetFormat( pCloseMediaType );
        }

        // free useful media types
        if ( pExactMediaType != nullptr )
        {
            DeleteMediaType( pExactMediaType );
        }
        if ( pCloseMediaType != nullptr )
        {
            DeleteMediaType( pCloseMediaType );
        }

        pStreamConfig->Release( );
    }

    return capabilites;
}

// Check if filter provides output pin supporting MEDIASUBTYPE_MJPG media subtype
bool IsJpegEncodingAvailable( IBaseFilter* pBaseFilter )
{
    bool       ret      = false; 
    IEnumPins* pPinEnum = nullptr;
    IPin*      pPin     = nullptr;

    if ( SUCCEEDED( pBaseFilter->EnumPins( &pPinEnum ) ) )
    {
        while ( ( pPinEnum->Next( 1, &pPin, 0 ) == S_OK ) && ( !ret ) )
        {
            PIN_DIRECTION pinDir;

            if ( SUCCEEDED( pPin->QueryDirection( &pinDir ) ) && ( pinDir == PINDIR_OUTPUT ) )
            {
                // enum preferred media types of the pin
                IEnumMediaTypes* pMediaEnum = nullptr;
                AM_MEDIA_TYPE*   pmt         = nullptr;

                if ( SUCCEEDED( pPin->EnumMediaTypes( &pMediaEnum ) ) )
                {
                    while ( ( pMediaEnum->Next( 1, &pmt, NULL ) == S_OK ) && ( !ret ) )
                    {
                        if ( ( pmt->majortype == MEDIATYPE_Video ) && ( pmt->subtype == MEDIASUBTYPE_MJPG ) )
                        {
                            ret = true;
                        }

                        DeleteMediaType( pmt );
                    }

                    pMediaEnum->Release( );
                }
            }

            pPin->Release( );
        }

        pBaseFilter->Release( );
    }

    return ret;
}

// Create filter described by the specified moniker string
IBaseFilter* CreateFilter( const string& moniker )
{
    IBaseFilter* pBaseFilter = nullptr;

    // check how much would it take to convert the string from UTF8 to UTF16
    int charsRequired = MultiByteToWideChar( CP_UTF8, 0, moniker.c_str( ), -1, nullptr, 0 );

    if ( charsRequired > 0 )
    {
        WCHAR* monikerUtf16 = new WCHAR[charsRequired];

        // convert string to UTF16
        if ( MultiByteToWideChar( CP_UTF8, 0, moniker.c_str( ), -1, monikerUtf16, charsRequired ) > 0 )
        {
            IBindCtx* pBindContext = nullptr;

            if ( ( SUCCEEDED( CreateBindCtx( 0, &pBindContext ) ) ) && ( pBindContext != 0 ) )
            {
                IMoniker* pDeviceMoniker = nullptr;
                ULONG     charsParsed    = 0;

                if ( ( SUCCEEDED( MkParseDisplayName( pBindContext, monikerUtf16, &charsParsed, &pDeviceMoniker ) ) ) &&
                     ( pDeviceMoniker != nullptr ) )
                {
                    pDeviceMoniker->BindToObject( nullptr, nullptr, IID_IBaseFilter, (void**) &pBaseFilter );
                    pDeviceMoniker->Release( );
                }

                pBindContext->Release( );
            }
        }

        delete [] monikerUtf16;
    }

    return pBaseFilter;
}

// Convert specfied wide character string to UTF8
string Utf16to8( LPCWSTR utf16string )
{
    string ret;

    int bytesRequired = WideCharToMultiByte( CP_UTF8, 0, utf16string, -1, nullptr, 0, nullptr, nullptr );

    if ( bytesRequired > 0 )
    {
        char* utf8string = new char[bytesRequired];

        if ( WideCharToMultiByte( CP_UTF8, 0, utf16string, -1, utf8string, bytesRequired, nullptr, nullptr ) > 0 )
        {
            ret = string( utf8string );
        }

        delete[] utf8string;
    }

	return ret;
}

// Free media type - release memory taken by some of its members
void FreeMediaType( AM_MEDIA_TYPE& mt )
{
    if ( mt.cbFormat != 0 )
    {
        CoTaskMemFree( (void*) mt.pbFormat );
        mt.cbFormat = 0;
        mt.pbFormat = nullptr;
    }
    if ( mt.pUnk != nullptr )
    {
        mt.pUnk->Release( );
        mt.pUnk = nullptr;
    }
}

// Delete media type - free "inner" member and then delete structure itself
void DeleteMediaType( AM_MEDIA_TYPE* pmt )
{
    if ( pmt != nullptr )
    {
        FreeMediaType( *pmt ); 
        CoTaskMemFree( pmt );
    }
}

} // namespace Private
