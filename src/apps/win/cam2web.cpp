/*
    cam2web - streaming camera to web

    Copyright (C) 2017-2020, cvsandbox, cvsandbox@gmail.com

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

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <tchar.h>
#include <future>
#include <chrono>
#include <numeric>

#include <XLocalVideoDevice.hpp>
#include <XLocalVideoDeviceConfig.hpp>
#include <XWebServer.hpp>
#include <XVideoSourceToWeb.hpp>
#include <XVideoFrameDecorator.hpp>
#include <XObjectConfigurationSerializer.hpp>
#include <XObjectConfigurationRequestHandler.hpp>

#include "resource.h"
#include "Tools.hpp"
#include "UiTools.hpp"
#include "SettingsDialog.hpp"
#include "AccessRightsDialog.hpp"
#include "AppConfig.hpp"

// Release build embeds web resources into executable
#ifdef NDEBUG
    #include "index.html.h"
    #include "admin.html.h"
    #include "styles.css.h"
    #include "cam2web.png.h"
    #include "cam2web_white.png.h"
    #include "camera.js.h"
    #include "cameraproperties.html.h"
    #include "cameraproperties.js.h"
    #include "jquery.js.h"
    #include "jquery.mobile.js.h"
    #include "jquery.mobile.css.h"
#endif

// Enable visual styles by using ComCtl32.dll version 6 or later
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;
using namespace std::chrono;

#define MAX_LOADSTRING          (100)

#define IDC_COMBO_CAMERAS       (501)
#define IDC_COMBO_RESOLUTIONS   (502)
#define IDC_EDIT_FRAME_RATE     (503)
#define IDC_SPIN_FRAME_RATE     (504)
#define IDC_BUTTON_START        (505)
#define IDC_LINK_STATUS         (506)
#define IDC_STATIC_ERROR_MSG    (507)
#define IDC_STATUS_BAR          (508)
#define IDC_SYS_TRAY_ID         (509)

#define STR_ERROR               TEXT( "Error" )
#define STR_START_STREAMING     TEXT( "&Start streaming" )
#define STR_STOP_STREAMING      TEXT( "&Stop streaming" )

#define WM_UPDATE_UI            (WM_USER + 1)
#define WM_UPDATE_ERROR         (WM_USER + 2)
#define WM_SYS_TRAY_NOTIFY      (WM_USER + 3)

#define TIMER_ID_CAMERA_CONFIG  (0xB0B)
#define TIMER_ID_FPS_UPDATE     (0xBADB0B)
#define TIMER_ID_ACTIVITY_CHECK (0xF00D)

// Number of the last FPS values to average
#define FPS_HISTORY_LENGTH (10)

// Information provided on version request
#define STR_INFO_PRODUCT        "cam2web"
#define STR_INFO_VERSION        "1.2.0"
#define STR_INFO_PLATFORM       "Windows"

// Default names of some configuration files/folders
#define STR_CONFIG_FOLDER       "cam2web";
#define STR_CONFIG_FILE         "cam2web.cfg"
#define STR_USERS_FILE          "users.txt"

// Available application icons
static const int AppIconIds[]       = { IDI_CAM2WEB, IDI_CAM2WEB_GREEN, IDI_CAM2WEB_ORANGE, IDI_CAM2WEB_RED };
static const int AppActiveIconIds[] = { IDI_CAMERA_ACTIVE_BLUE, IDI_CAMERA_ACTIVE_GREEN, IDI_CAMERA_ACTIVE_ORANGE, IDI_CAMERA_ACTIVE_RED };

// Forward declarations of functions included in this code module:
LRESULT CALLBACK MainWndProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR CALLBACK AboutDlgProc( HWND, UINT, WPARAM, LPARAM );
void GetVideoDevices( );

// Listener for camera errors
class CameraErrorListener : public IVideoSourceListener
{
public:
    // New video frame notification
    virtual void OnNewImage( const shared_ptr<const XImage>& image );
    // Video source error notification
    virtual void OnError( const string& errorMessage, bool fatal );
};

// Helper class to keep streaming status
class StreamingStatusController : public IObjectConfigurator
{
private:
    bool streamingInProgress;

    static const string STR_RUNNING;

public:
    StreamingStatusController( ) : streamingInProgress( false )
    { }

    bool IsStreaming( ) const { return streamingInProgress; }
    void SetStreamingStatus( bool isStreaming ) { streamingInProgress = isStreaming; }

    XError SetProperty( const string& propertyName, const string& value );
    XError GetProperty( const string& propertyName, string& value ) const;
    map<string, string> GetAllProperties( ) const;
};

const string StreamingStatusController::STR_RUNNING = "running";

// Place holder for all global variables the application needs
class AppData
{
public:
    TCHAR       szTitle[MAX_LOADSTRING];            // title bar text
    TCHAR       szWindowClass[MAX_LOADSTRING];      // the main window class name
    HINSTANCE   hInst;                              // current instance

    bool        autoStartStreaming;
    bool        minimizeWindowOnStart;
    bool        showNoUI;
    uint16_t    adminPort;

    HWND        hwndMain;
    HWND        hwndCamerasCombo;
    HWND        hwndResolutionsCombo;
    HWND        hwndFrameRateEdit;
    HWND        hwndFrameRateSpin;
    HWND        hwndStartButton;
    HWND        hwndStatusLink;
    HWND        hwndStatusBar;
    HICON       hiconAppIcon;
    HICON       hiconAppActiveIcon;
    HICON       hiconTrayIcon;
    HICON       hiconTrayActiveIcon;

    vector<XDeviceName>             devices;
    vector<XDeviceCapabilities>     cameraCapabilities;
    shared_ptr<XLocalVideoDevice>   camera;
    XDeviceName                     selectedDeviceName;
    XDeviceCapabilities             selectedResolutuion;
    shared_ptr<IObjectConfigurator> cameraConfig;
    shared_ptr<AppConfig>           appConfig;

    XWebServer                      server;
    XWebServer                      adminServer;
    XVideoSourceToWeb               video2web;

    shared_ptr<StreamingStatusController>   streamingStatus;

    string                          appFolder;
    string                          appConfigFile;

    XObjectConfigurationSerializer  appConfigSerializer;
    XObjectConfigurationSerializer  cameraConfigSerializer;

    string                          lastVideoSourceError;

    XVideoSourceListenerChain       listenerChain;
    XVideoFrameDecorator            videoDecorator;
    CameraErrorListener             cameraErrorListener;

    uint32_t                        lastFramesGot;
    vector<float>                   lastFpsValues;
    int                             nextFpsIndex;
    steady_clock::time_point        lastFpsCheck;

    AppData( ) :
        hInst( NULL ), hwndMain( NULL ), hwndCamerasCombo( NULL ),
        autoStartStreaming( false ), minimizeWindowOnStart( false ), showNoUI( false ), adminPort( 0 ),
        hwndResolutionsCombo( NULL ), hwndStartButton( NULL ), hwndStatusLink( NULL ), hwndStatusBar( NULL ),
        hiconAppIcon( NULL ), hiconAppActiveIcon( NULL ), hiconTrayIcon( NULL ), hiconTrayActiveIcon( NULL ),
        devices( ), cameraCapabilities( ), camera( ), selectedDeviceName( ), selectedResolutuion( ),
        cameraConfig( ), appConfig( new AppConfig( ) ), server( ), adminServer( ), video2web( ),
        streamingStatus( make_shared<StreamingStatusController>( ) ),
        appFolder( ".\\" ), appConfigFile( STR_CONFIG_FILE ),
        appConfigSerializer( ), cameraConfigSerializer( ), lastVideoSourceError( ),
        listenerChain( ), videoDecorator( ), cameraErrorListener( )
    {
        // find user' home folder to store settings
        WCHAR homeFolder[MAX_PATH];

        if ( SUCCEEDED( SHGetFolderPathW( NULL, CSIDL_PROFILE, NULL, 0, homeFolder ) ) )
        {
            appFolder = Utf16to8( homeFolder );
            appFolder += '\\';
            appFolder += STR_CONFIG_FOLDER;
            appFolder += '\\';
        }

        appConfigFile = appFolder + STR_CONFIG_FILE;

        CreateDirectory( Utf8to16( appFolder ).c_str( ), nullptr );

        appConfig->SetUsersFileName( appFolder + STR_USERS_FILE );
    }

    void LoadAppIcons( );
    void UpdateFpsInfo( );
    void UpdateWebActivity( );
    void StartAdminServer( );
};
AppData* gData = NULL;

// New video frame notification - clear error
void CameraErrorListener::OnNewImage( const shared_ptr<const XImage>& /* image */ )
{
    if ( !gData->lastVideoSourceError.empty( ) )
    {
        gData->lastVideoSourceError.clear( );
        PostMessage( gData->hwndMain, WM_UPDATE_ERROR, 0, 0 );
    }
};

// Video source error notification
void CameraErrorListener::OnError( const string& errorMessage, bool fatal )
{
    gData->lastVideoSourceError = ( ( fatal ) ? "Fatal: " : "" ) + errorMessage;
    PostMessage( gData->hwndMain, WM_UPDATE_ERROR, 0, 0 );
}

// Parse command line and override default settings
static bool ParseCommandLine( int argc, WCHAR* argv[], AppData* appData )
{
    int  i;

    for ( i = 0; i < argc; i++ )
    {
        wstring key;
        wstring value;
        WCHAR*  ptrDelimiter = wcschr( argv[i], L':' );

        if ( argv[i][0] != '/' )
            break;

        if ( ptrDelimiter == nullptr )
        {
            key = wstring( argv[i] + 1 );
        }
        else
        {
            key   = wstring( argv[i] + 1, ptrDelimiter - argv[i] - 1 );
            value = wstring( ptrDelimiter + 1 );

            if ( value.empty( ) )
            {
                // delimiter is there, but no value - bogus
                break;
            }
        }

        if ( key == L"fcfg" )
        {
            if ( !value.empty( ) )
            {
                string fileName = Utf16to8( value );

                // if the specified configuration file does not include any slashes, then assume it is home folder
                if ( ( fileName.find( '/' ) != string::npos ) || ( fileName.find( '\\' ) != string::npos ) )
                {
                    appData->appConfigFile = fileName;
                }
                else
                {
                    appData->appConfigFile = appData->appFolder + fileName;
                }
            }
            else
            {
                break;
            }
        }
        else if ( key == L"start" )
        {
            if ( value.empty( ) )
            {
                appData->autoStartStreaming = true;
            }
            else
            {
                break;
            }
        }
        else if ( key == L"minimize" )
        {
            if ( value.empty( ) )
            {
                appData->minimizeWindowOnStart = true;
            }
            else
            {
                break;
            }
        }
        else if ( key == L"adminport" )
        {
            if ( !value.empty( ) )
            {
                int port;

                if ( swscanf( value.c_str( ), L"%u", &port ) == 1 )
                {
                    appData->adminPort = static_cast< uint16_t >( port );
                }
            }
        }
        else if (key == L"noui" )
        {
            if ( value.empty( ) )
            {
                appData->showNoUI = true;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    return ( i == argc );
}

// Register class of the main window
static ATOM MyRegisterClass( HINSTANCE hInstance )
{
    WNDCLASSEX wcex;
    uint16_t   iconIndex = ( gData->appConfig == nullptr ) ? 0 : gData->appConfig->WindowIconIndex( );

    if ( iconIndex > sizeof( AppIconIds ) / sizeof( AppIconIds[0] ) )
    {
        iconIndex = 0;
    }

    wcex.cbSize        = sizeof( WNDCLASSEX );
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = MainWndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon( hInstance, MAKEINTRESOURCE( AppIconIds[iconIndex] ) );
    wcex.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH) ( COLOR_WINDOW );
    wcex.lpszMenuName  = MAKEINTRESOURCE( IDC_CAM2WEB );
    wcex.lpszClassName = gData->szWindowClass;
    wcex.hIconSm       = LoadIcon( hInstance, MAKEINTRESOURCE( AppIconIds[iconIndex] ) );

    return RegisterClassEx( &wcex );
}

// Callback used to set font of window's children
static BOOL __stdcall SetWindowFont( HWND hwnd, LPARAM lParam )
{
    HGDIOBJ hFont = (HGDIOBJ) lParam;

    SendMessage( hwnd, WM_SETFONT, (WPARAM) hFont, TRUE );
    return TRUE;
}

// Create main window of the application
static BOOL CreateMainWindow( HINSTANCE hInstance, int nCmdShow )
{
    INITCOMMONCONTROLSEX initControls = { sizeof( INITCOMMONCONTROLSEX ), ICC_LINK_CLASS };

    InitCommonControlsEx( &initControls );

    HWND hwndMain = CreateWindow( gData->szWindowClass, gData->szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 220, NULL, NULL, hInstance, NULL );
    if ( hwndMain == NULL )
    {
        return FALSE;
    }

    gData->hwndMain = hwndMain;

    ResizeWindowToClientSize( hwndMain, 300, 195 );

    // cameras' combo and label
    HWND hWindLabel = CreateWindow( WC_STATIC, TEXT( "&Camera:" ), WS_CHILD | WS_VISIBLE | WS_GROUP,
        10, 14, 70, 20, hwndMain, (HMENU) IDC_STATIC, hInstance, NULL );

    gData->hwndCamerasCombo = CreateWindow( WC_COMBOBOX, TEXT( "" ),
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VSCROLL | WS_VISIBLE | WS_TABSTOP,
        80, 10, 210, 100, hwndMain, (HMENU) IDC_COMBO_CAMERAS, hInstance, NULL );

    // resolutions' combo and label
    hWindLabel = CreateWindow( WC_STATIC, TEXT( "&Resolution:" ), WS_CHILD | WS_VISIBLE,
        10, 39, 70, 20, hwndMain, (HMENU) IDC_STATIC, hInstance, NULL );

    gData->hwndResolutionsCombo = CreateWindow( WC_COMBOBOX, TEXT( "" ),
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VSCROLL | WS_VISIBLE | WS_TABSTOP,
        80, 35, 210, 150, hwndMain, (HMENU) IDC_COMBO_RESOLUTIONS, hInstance, NULL );

    // frame rate overriding
    hWindLabel = CreateWindow( WC_STATIC, TEXT( "Fr&ame rate:" ), WS_CHILD | WS_VISIBLE,
        10, 64, 70, 20, hwndMain, (HMENU) IDC_STATIC, hInstance, NULL );

    gData->hwndFrameRateEdit = CreateWindowEx( WS_EX_LEFT | WS_EX_CLIENTEDGE, WC_EDIT, NULL,
        WS_CHILDWINDOW | WS_VISIBLE | WS_BORDER | WS_TABSTOP | ES_NUMBER | ES_LEFT,
        80, 60, 210, 20, hwndMain, (HMENU) IDC_EDIT_FRAME_RATE, hInstance, NULL );

    gData->hwndFrameRateSpin = CreateWindowEx( WS_EX_LEFT | WS_EX_LTRREADING, UPDOWN_CLASS, NULL,
        WS_CHILDWINDOW | WS_VISIBLE | UDS_AUTOBUDDY | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_HOTTRACK,
        0, 0, 0, 0, hwndMain, (HMENU) IDC_SPIN_FRAME_RATE, hInstance, NULL );

    // streaming start/stop button
    gData->hwndStartButton = CreateWindow( WC_BUTTON, STR_START_STREAMING,
        WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_TABSTOP,
        10, 95, 280, 40, hwndMain, (HMENU) IDC_BUTTON_START, hInstance, NULL );

    // HTTP link to open camera in local browser
    gData->hwndStatusLink = CreateWindowEx( 0, WC_LINK, TEXT( "" ),
        WS_CHILD | WS_TABSTOP,
        10, 145, 280, 20, hwndMain, (HMENU) IDC_LINK_STATUS, hInstance, NULL );

    // status bar
    int sbLabelsWidth[2] = { 220, -1 };

    gData->hwndStatusBar = CreateStatusWindow( WS_CHILD | WS_VISIBLE, nullptr, hwndMain, IDC_STATUS_BAR );
    SendMessage( gData->hwndStatusBar, SB_SETPARTS, 2, (LPARAM) sbLabelsWidth );

    // set default font for the window and its childrent
    HGDIOBJ hFont = GetStockObject( DEFAULT_GUI_FONT );

    SendMessage( hwndMain, WM_SETFONT, (WPARAM) hFont, TRUE );
    EnumChildWindows( hwndMain, SetWindowFont, (LPARAM) hFont );

    // restore main window's position or center it
    if ( ( gData->appConfig->MainWindowX( ) == 0x0FFFFFFF ) || ( gData->appConfig->MainWindowY( ) == 0x0FFFFFFF ) )
    {
        CenterWindowTo( hwndMain, GetDesktopWindow( ) );
    }
    else
    {
        SetWindowPos( hwndMain, HWND_TOP, gData->appConfig->MainWindowX( ), gData->appConfig->MainWindowY( ), 0, 0, SWP_NOSIZE );
        EnsureWindowVisible( hwndMain );
    }

    if ( !gData->showNoUI )
    {
        ShowWindow( hwndMain, nCmdShow );
        UpdateWindow( hwndMain );
    }

    return TRUE;
}

// Application entry point
int APIENTRY _tWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

#ifdef _DEBUG
    _CrtMemState memStateAtStart;
    //_CrtSetBreakAlloc( 653 );
#endif

    _CrtMemCheckpoint( &memStateAtStart );

    int ret = 0;

    gData = new AppData;
    gData->hInst = hInstance;

    // initialize global strings
    LoadString( hInstance, IDS_APP_TITLE, gData->szTitle, MAX_LOADSTRING );
    LoadString( hInstance, IDC_CAM2WEB, gData->szWindowClass, MAX_LOADSTRING );

    if ( ( lpCmdLine != nullptr ) && ( lpCmdLine[0] != L'\0' ) )
    {
        int     argc;
        LPWSTR* argv = CommandLineToArgvW( lpCmdLine, &argc );

        if ( ( argv != nullptr ) && ( argc > 0 ) )
        {
            if ( !ParseCommandLine( argc, argv, gData ) )
            {
                MessageBox( NULL, L"An unknown command line option is provided.\n\n"
                                  L"Available command line options:\n\n"
                                  L"/fcfg:<name>\n"
                                  L"\tName of the file to store application's configuration.\n\n"
                                  L"/start\n"
                                  L"\tAuto-start camera streaming on application start.\n\n"
                                  L"/minimize\n"
                                  L"\tMinimize application's window on start.\n"
                                  L"/noui\n"
                                  L"\tStart application with no UI",
                            gData->szTitle, MB_OK | MB_ICONINFORMATION );
            }
        }
    }

    // load application settings
    gData->appConfigSerializer = XObjectConfigurationSerializer( gData->appConfigFile, gData->appConfig );
    gData->appConfigSerializer.LoadConfiguration( );

    MyRegisterClass( hInstance );

    gData->LoadAppIcons( );

    // create main window of the application
    if ( CreateMainWindow( hInstance, ( gData->minimizeWindowOnStart ) ? SW_MINIMIZE : nCmdShow ) )
    {
        // register for Connected Suspend Events
        typedef int( WINAPI* MYPROC )( HWND, int );
        HINSTANCE hinstLib = LoadLibrary( L"User32.dll" );

        if ( hinstLib != NULL )
        {
            MYPROC registerSuspendResumeProcAddr = ( MYPROC ) GetProcAddress( hinstLib, "RegisterSuspendResumeNotification" );
            if ( registerSuspendResumeProcAddr != NULL )
            {
                ( registerSuspendResumeProcAddr )( gData->hwndMain, DEVICE_NOTIFY_WINDOW_HANDLE );
            }
            FreeLibrary( hinstLib );
        }

        HACCEL hAccelTable = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDC_CAM2WEB ) );
        MSG    msg;

        // get the list of video devices
        GetVideoDevices( );

        // check if streaming should start automatically
        if ( gData->autoStartStreaming )
        {
            PostMessage( gData->hwndMain, WM_COMMAND, MAKELONG( IDC_BUTTON_START, BN_CLICKED ), 0 );
        }

        // start an instance of web server for administration only
        gData->StartAdminServer( );

        // main message loop
        while ( GetMessage( &msg, NULL, 0, 0 ) )
        {
            if ( ( !TranslateAccelerator( gData->hwndMain, hAccelTable, &msg ) ) &&
                 ( !IsDialogMessage( gData->hwndMain, &msg ) ) )
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }

        ret = (int) msg.wParam;
    }

    delete gData;

    _CrtMemDumpAllObjectsSince( &memStateAtStart );

    return ret;
}

// Get the range of supported frame rates for the selected resolution
static void GetFrameRateRange( )
{
    int resolutionIndex = static_cast<int>( SendMessage( gData->hwndResolutionsCombo, (UINT) CB_GETCURSEL, 0, 0 ) );

    if ( ( resolutionIndex >= 0 ) && ( resolutionIndex < (int) gData->cameraCapabilities.size( ) ) )
    {
        XDeviceCapabilities selectedCaps = gData->cameraCapabilities[resolutionIndex];
        int                 requestedFps = static_cast<int>( SendMessage( gData->hwndFrameRateSpin, UDM_GETPOS32, 0, 0 ) );

        if ( requestedFps == 0 )
        {
            // nothing was chosen by user yet, so try the one used last time
            requestedFps = gData->appConfig->LastRequestedFrameRate( );

            if ( ( requestedFps < selectedCaps.MinimumFrameRate( ) ) ||
                 ( requestedFps > selectedCaps.MaximumFrameRate( ) ) )
            {
                requestedFps = selectedCaps.AverageFrameRate( );
            }
        }
        else
        {
            // always prefer default average frame when changing resolution
            requestedFps = selectedCaps.AverageFrameRate( );
        }

        InitUpDownControl( gData->hwndFrameRateSpin, gData->hwndFrameRateEdit,
                           static_cast<uint16_t>( selectedCaps.MinimumFrameRate( ) ),
                           static_cast<uint16_t>( selectedCaps.MaximumFrameRate( ) ),
                           static_cast<uint16_t>( requestedFps ) );
        EnableWindow( gData->hwndFrameRateEdit, ( selectedCaps.MinimumFrameRate( ) != selectedCaps.MaximumFrameRate( ) ) );
    }
    else
    {
        InitUpDownControl( gData->hwndFrameRateSpin, gData->hwndFrameRateEdit, 0, 0, 0 );
        EnableWindow( gData->hwndFrameRateEdit, FALSE );
    }
}

// Create video source object for the selected device and get its available resolutions
static void CreateDeviceAndGetResolutions( )
{
    int cameraIndex = static_cast<int>( SendMessage( gData->hwndCamerasCombo, (UINT) CB_GETCURSEL, 0, 0 ) );

    SendMessage( gData->hwndResolutionsCombo, CB_RESETCONTENT, 0, 0 );

    if ( ( cameraIndex >= 0 ) && ( cameraIndex < (int) gData->devices.size( ) ) )
    {
        uint16_t width = 0, height = 0, bpp = 0, fps = 0;

        gData->selectedDeviceName = gData->devices[cameraIndex];
        gData->camera = XLocalVideoDevice::Create( gData->selectedDeviceName.Moniker( ) );

        // get resolution last used
        gData->appConfig->GetLastVideoResolution( &width, &height, &bpp, &fps );

        // if no preference was made yet, try something default
        if ( width == 0 )
        {
            width  = 640;
            height = 480;
            bpp    = 24;
            fps    = 30;
        }

        if ( gData->camera )
        {
            TCHAR strResolution[256];
            bool  foundExactMatch = false;
            int   exactMatchIndex = 0;
            int   closeMatchIndex = 0;
            int   closeMatchBpp   = 0;
            int   index           = 0;
            int   minAreaDiff     = 1000000;
            int   areaDiff;

            gData->cameraCapabilities = gData->camera->GetCapabilities( );

            for ( auto cap : gData->cameraCapabilities )
            {
                swprintf( strResolution, 255, TEXT( "%d x %d, %d bpp, %d fps" ), cap.Width( ), cap.Height( ), cap.BitCount( ), cap.AverageFrameRate( ) );

                SendMessage( gData->hwndResolutionsCombo, CB_ADDSTRING, 0, (LPARAM) strResolution );

                if ( ( width == cap.Width( ) ) && ( height == cap.Height( ) ) && ( bpp == cap.BitCount( ) ) && ( fps == cap.MaximumFrameRate( ) ) )
                {
                    exactMatchIndex = index;
                    foundExactMatch = true;
                }
                else
                {
                    areaDiff = abs( width * height - cap.Width( ) * cap.Height( ) );

                    if ( ( areaDiff < minAreaDiff ) ||
                         ( ( areaDiff == minAreaDiff ) && ( closeMatchBpp < cap.BitCount( ) ) ) )
                    {
                        minAreaDiff     = areaDiff;
                        closeMatchIndex = index;
                        closeMatchBpp   = cap.BitCount( );
                    }
                }

                index++;
            }

            SendMessage( gData->hwndResolutionsCombo, CB_SETCURSEL, ( foundExactMatch ) ? exactMatchIndex : closeMatchIndex, 0 );
            GetFrameRateRange( );

            // if camera supports MJPEG, lets use it
            gData->camera->PreferJpegEncoding( true );
        }
    }
}

// Populate list of available devices
void GetVideoDevices( )
{
    int indexToSelect   = 0;
    int preferenceIndex = -1;

    gData->devices = XLocalVideoDevice::GetAvailableDevices( );

    if ( gData->devices.empty( ) )
    {
        SendMessage( gData->hwndCamerasCombo, CB_ADDSTRING, 0, (LPARAM) TEXT( "No video devices found" ) );

        EnableWindow( gData->hwndCamerasCombo, FALSE );
        EnableWindow( gData->hwndResolutionsCombo, FALSE );
        EnableWindow( gData->hwndFrameRateEdit, FALSE );
        EnableWindow( gData->hwndStartButton, FALSE );
    }
    else
    {
        const string lastUsedCameraMoniker = gData->appConfig->CameraMoniker( );
        const string devicePreference      = gData->appConfig->DevicePreference( );
        TCHAR        deviceName[256];
        int          index = 0;

        for ( auto device : gData->devices )
        {
            _tcsncpy( deviceName, Utf8to16( device.Name( ) ).c_str( ), 255 );

            SendMessage( gData->hwndCamerasCombo, CB_ADDSTRING, 0, (LPARAM) deviceName );

            if ( device.Name( ) == devicePreference )
            {
                preferenceIndex = index;
            }
            if ( device.Moniker( ) == lastUsedCameraMoniker )
            {
                indexToSelect = index;
            }

            index++;
        }
    }

    SendMessage( gData->hwndCamerasCombo, CB_SETCURSEL, ( preferenceIndex != -1 ) ? preferenceIndex : indexToSelect, 0 );
    CreateDeviceAndGetResolutions( );
}

// Start streaming of the selected video source
static bool StartVideoStreaming( )
{
    bool ret = false;

    if ( gData->camera )
    {
        string    cameraMoniker   = gData->selectedDeviceName.Moniker( );
        int       resolutionIndex = static_cast<int>( SendMessage( gData->hwndResolutionsCombo, (UINT) CB_GETCURSEL, 0, 0 ) );
        UserGroup viewersGroup    = static_cast<UserGroup>( gData->appConfig->ViewersGroup( ) );
        UserGroup configGroup     = static_cast<UserGroup>( gData->appConfig->ConfiguratorsGroup( ) );
        uint16_t  requestedFps    = static_cast<uint16_t>( SendMessage( gData->hwndFrameRateSpin, UDM_GETPOS32, 0, 0 ) );

        if ( ( resolutionIndex >= 0 ) && ( resolutionIndex < (int) gData->cameraCapabilities.size( ) ) )
        {
            gData->selectedResolutuion = gData->cameraCapabilities[resolutionIndex];
            gData->camera->SetResolution( gData->selectedResolutuion, requestedFps );
        }

        // remember selected camera and resolution
        gData->appConfig->SetCameraMoniker( cameraMoniker );
        gData->appConfig->SetLastVideoResolution( static_cast<uint16_t>( gData->selectedResolutuion.Width( ) ),
                                                  static_cast<uint16_t>( gData->selectedResolutuion.Height( ) ),
                                                  static_cast<uint16_t>( gData->selectedResolutuion.BitCount( ) ),
                                                  static_cast<uint16_t>( gData->selectedResolutuion.MaximumFrameRate( ) ) );
        gData->appConfig->SetLastRequestedFrameRate( requestedFps );
        gData->appConfigSerializer.SaveConfiguration( );

        // some read-only information about the version
        PropertyMap versionInfo;

        versionInfo.insert( PropertyMap::value_type( "product", STR_INFO_PRODUCT ) );
        versionInfo.insert( PropertyMap::value_type( "version", STR_INFO_VERSION ) );
        versionInfo.insert( PropertyMap::value_type( "platform", STR_INFO_PLATFORM ) );

        // prepare some read-only informational properties of the camera
        PropertyMap cameraInfo;
        char        strVideoSize[32];
        string      cameraTitle = gData->appConfig->CameraTitle( );

        if ( cameraTitle.empty( ) )
        {
            cameraTitle = gData->selectedDeviceName.Name( );
        }

        sprintf( strVideoSize,      "%d", gData->selectedResolutuion.Width( ) );
        sprintf( strVideoSize + 16, "%d", gData->selectedResolutuion.Height( ) );

        cameraInfo.insert( PropertyMap::value_type( "device", gData->selectedDeviceName.Name( ) ) );
        cameraInfo.insert( PropertyMap::value_type( "title",  cameraTitle ) );
        cameraInfo.insert( PropertyMap::value_type( "width",  strVideoSize ) );
        cameraInfo.insert( PropertyMap::value_type( "height", strVideoSize + 16 ) );

        // allow camera configuration through simplified configurator object
        gData->cameraConfig = make_shared<XLocalVideoDeviceConfig>( gData->camera );

        // use MD5 of camera's moniker as file name to store camera's setting - just to avoid unfriendly characters which may happen in the moniker
        string cameraSettingsFileName = gData->appFolder + GetMd5Hash( (const uint8_t*) cameraMoniker.c_str( ), static_cast<int>( cameraMoniker.length( ) ) ) + ".cfg";

        // restore camera settings
        gData->cameraConfigSerializer = XObjectConfigurationSerializer( cameraSettingsFileName, gData->cameraConfig );
        gData->cameraConfigSerializer.LoadConfiguration( );

        // set JPEG quality
        gData->video2web.SetJpegQuality( gData->appConfig->JpegQuality( ) );

        // set authentication domain and load users' list
        gData->server.SetAuthDomain( gData->appConfig->AuthDomain( ) );
        gData->server.SetAuthenticationMethod( ( gData->appConfig->AuthenticationMethod( ) == "basic" ) ?
                                               Authentication::Basic : Authentication::Digest );
        gData->server.LoadUsersFromFile( gData->appConfig->UsersFileName( ) );

        // configure web server and handler
        gData->server.SetPort( gData->appConfig->HttpPort( ) ).
                      AddHandler( make_shared<XObjectInformationRequestHandler>( "/version", make_shared<XObjectInformationMap>( versionInfo ) ) ).
                      AddHandler( make_shared<XObjectConfigurationRequestHandler>( "/camera/config", gData->cameraConfig ), configGroup ).
                      AddHandler( make_shared<XObjectInformationRequestHandler>( "/camera/properties", make_shared<XLocalVideoDevicePropsInfo>( gData->camera ) ), configGroup ).
                      AddHandler( make_shared<XObjectInformationRequestHandler>( "/camera/info", make_shared<XObjectInformationMap>( cameraInfo ) ), viewersGroup ).
                      AddHandler( gData->video2web.CreateJpegHandler( "/camera/jpeg" ), viewersGroup ).
                      AddHandler( gData->video2web.CreateMjpegHandler( "/camera/mjpeg", gData->appConfig->MjpegFrameRate( ) ), viewersGroup );

        // check if custom web content is available
        if ( !gData->appConfig->CustomWebContent( ).empty( ) )
        {
            gData->server.SetDocumentRoot( gData->appConfig->CustomWebContent( ) );
        }
        else
        {
#ifdef _DEBUG
            // load web content from files in debug builds
            gData->server.SetDocumentRoot( "./web/" );
#else
            // web content is embeded in release builds to get single executable
            gData->server.AddHandler( make_shared<XEmbeddedContentHandler>( "/", &web_index_html ), viewersGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "index.html", &web_index_html ), viewersGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "styles.css", &web_styles_css ), viewersGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "cam2web.png", &web_cam2web_png ) ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "cam2web_white.png", &web_cam2web_white_png ) ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "camera.js", &web_camera_js ), viewersGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "cameraproperties.js", &web_cameraproperties_js ), viewersGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "cameraproperties.html", &web_cameraproperties_html ), configGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "jquery.js", &web_jquery_js ), viewersGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "jquery.mobile.js", &web_jquery_mobile_js ), viewersGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "jquery.mobile.css", &web_jquery_mobile_css ), viewersGroup );
#endif
        }

        // configure video frame decorator
        gData->videoDecorator.SetCameraTitle( gData->appConfig->CameraTitle( ) );
        gData->videoDecorator.SetTimestampOverlay( gData->appConfig->TimestampOverlay( ) );
        gData->videoDecorator.SetCameraTitleOverlay( gData->appConfig->CameraTitleOverlay( ) );
        gData->videoDecorator.SetOverlayTextColor( gData->appConfig->OverlayTextColor( ) );
        gData->videoDecorator.SetOverlayBackgroundColor( gData->appConfig->OverlayBackgroundColor( ) );

        // set video source listeners' chain
        gData->listenerChain.Clear( );
        gData->listenerChain.Add( &gData->videoDecorator );
        gData->listenerChain.Add( gData->video2web.VideoSourceListener( ) );
        gData->listenerChain.Add( &gData->cameraErrorListener );

        gData->camera->SetListener( &gData->listenerChain );

        if ( !gData->camera->Start( ) )
        {
            CenteredMessageBox( gData->hwndMain, TEXT( "Failed starting video source." ), STR_ERROR, MB_OK | MB_ICONERROR );
        }
        else if ( !gData->server.Start( ) )
        {
            CenteredMessageBox( gData->hwndMain, TEXT( "Failed starting web server." ), STR_ERROR, MB_OK | MB_ICONERROR );
            gData->camera->SignalToStop( );
            gData->camera->WaitForStop( );
        }
        else
        {
            wstring newWindowTitle = gData->szTitle;

            newWindowTitle += L" :: ";
            newWindowTitle += Utf8to16( gData->selectedDeviceName.Name( ) );

            SetWindowText( gData->hwndMain, newWindowTitle.c_str( ) );

            // reset FPS info
            gData->lastFpsValues.clear( );
            gData->lastFramesGot = 0;
            gData->nextFpsIndex  = 0;
            gData->lastFpsCheck  = steady_clock::now( );

            // setup timer to save camera configuration from time to time
            SetTimer( gData->hwndMain, TIMER_ID_CAMERA_CONFIG, 60000, NULL );
            // setup timer to update camera FPS information
            SetTimer( gData->hwndMain, TIMER_ID_FPS_UPDATE, 1000, NULL );
            // setup timer to monitor for web server activity
            SetTimer( gData->hwndMain, TIMER_ID_ACTIVITY_CHECK, 1000, NULL );
           
            ret = true;
        }
    }

    return ret;
}

// Stop streaming of the current video source
static void StopVideoStreaming( )
{
    KillTimer( gData->hwndMain, TIMER_ID_CAMERA_CONFIG );
    KillTimer( gData->hwndMain, TIMER_ID_FPS_UPDATE );
    KillTimer( gData->hwndMain, TIMER_ID_ACTIVITY_CHECK );

    if ( gData->camera )
    {
        // save camera settings
        gData->cameraConfigSerializer.SaveConfiguration( );
        gData->cameraConfigSerializer = XObjectConfigurationSerializer( );

        gData->camera->SignalToStop( );
        gData->camera->WaitForStop( );
    }

    gData->server.Stop( );
    gData->server.ClearHandlers( );
    gData->server.ClearUsers( );

    gData->lastVideoSourceError.clear( );

    SetWindowText( gData->hwndMain, gData->szTitle );

    // clean status bar
    SendMessage( gData->hwndStatusBar, SB_SETTEXT, 0, (LPARAM) nullptr );
    SendMessage( gData->hwndStatusBar, SB_SETTEXT, 1, (LPARAM) nullptr );
}

// Toggle video streaming state
static void ToggleStreaming( )
{
    // start/stop streaming
    if ( gData->streamingStatus->IsStreaming( ) )
    {
        StopVideoStreaming( );
        gData->streamingStatus->SetStreamingStatus( false );
    }
    else
    {
        if ( StartVideoStreaming( ) )
        {
            gData->streamingStatus->SetStreamingStatus( true );
        }
    }

    // update UI controls
    PostMessage( gData->hwndMain, WM_UPDATE_UI, 0, 0 );
    PostMessage( gData->hwndMain, WM_UPDATE_ERROR, 0, 0 );
}

// Minimize window to system tray
void MinimizeToTray( HWND hwnd )
{
    NOTIFYICONDATA notifyData = { };

    notifyData.cbSize           = sizeof( notifyData );
    notifyData.hWnd             = hwnd;
    notifyData.uID              = IDC_SYS_TRAY_ID;
    notifyData.uFlags           = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    notifyData.uCallbackMessage = WM_SYS_TRAY_NOTIFY;
    notifyData.hIcon            = gData->hiconTrayIcon;

    GetWindowText( hwnd, notifyData.szTip, sizeof( notifyData.szTip ) / sizeof( WCHAR ) );
    Shell_NotifyIcon( NIM_ADD, &notifyData );

    gData->UpdateWebActivity( );

    // hide the window
    ShowWindow( hwnd, SW_HIDE );
}

// Update tool tip text for the sys tray icon
void UpdateTrayTip( HWND hwnd, const WCHAR* szTip )
{
    NOTIFYICONDATA notifyData = { };

    notifyData.cbSize = sizeof( notifyData );
    notifyData.hWnd   = hwnd;
    notifyData.uID    = IDC_SYS_TRAY_ID;
    notifyData.uFlags = NIF_TIP;

    wcsncpy( notifyData.szTip, szTip, sizeof( notifyData.szTip ) / sizeof( WCHAR ) );

    Shell_NotifyIcon( NIM_MODIFY, &notifyData );
}

// Restor window from system tray
void RestoreFromTray( HWND hwnd )
{
    NOTIFYICONDATA notifyData = { };

    notifyData.cbSize = sizeof( notifyData );
    notifyData.hWnd   = hwnd;
    notifyData.uID    = IDC_SYS_TRAY_ID;

    Shell_NotifyIcon( NIM_DELETE, &notifyData );

    // restore window back to normal size
    ShowWindow( hwnd, SW_RESTORE );
    SetActiveWindow( hwnd );
    SetForegroundWindow( hwnd );
}

// Restart camera streaming
void RestartStreaming( )
{
    StopVideoStreaming( );
    gData->streamingStatus->SetStreamingStatus( false );

    StartVideoStreaming( );
    gData->streamingStatus->SetStreamingStatus( true );
}

// Main window's message handler
LRESULT CALLBACK MainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    static HBRUSH errorLabelBrush = CreateSolidBrush( RGB( 255, 230, 230 ) );

    int wmId, wmEvent;

    switch ( message )
    {
    case WM_CREATE:
        {
            HMENU hMainMenu = GetMenu( hWnd );

            SetMenuItemIcon( hMainMenu, IDM_SETTINGS, IDI_SETTINGS );
            SetMenuItemIcon( hMainMenu, IDM_ACCESS_RIGHTS, IDI_ACCESS );
            SetMenuItemIcon( hMainMenu, IDM_ABOUT, IDI_ABOUT );
        }
        break;

    case WM_COMMAND:
        wmId    = LOWORD( wParam );
        wmEvent = HIWORD( wParam );

        switch ( wmId )
        {
        case IDM_ABOUT:
            DialogBox( gData->hInst, MAKEINTRESOURCE( IDD_ABOUTBOX ), hWnd, AboutDlgProc );
            break;

        case IDM_SETTINGS:
            if ( DialogBoxParam( gData->hInst, MAKEINTRESOURCE( IDD_SETTINGS_BOX ), hWnd, SettingsDlgProc, (LPARAM) gData->appConfig.get( ) ) == IDOK )
            {
                gData->appConfigSerializer.SaveConfiguration( );

                gData->LoadAppIcons( );
                gData->UpdateWebActivity( );
            }
            break;

        case IDM_ACCESS_RIGHTS:
            if ( DialogBoxParam( gData->hInst, MAKEINTRESOURCE( IDD_ACCESS_RIGHTS_BOX ), hWnd, AccessRightsDialogProc, (LPARAM) gData->appConfig.get( ) ) == IDOK )
            {
                gData->appConfigSerializer.SaveConfiguration( );
            }
            break;

        case IDM_EXIT:
            DestroyWindow( hWnd );
            break;

        case IDC_COMBO_CAMERAS:
            if ( wmEvent == CBN_SELCHANGE )
            {
                CreateDeviceAndGetResolutions( );
            }
            break;

        case IDC_COMBO_RESOLUTIONS:
            if ( wmEvent == CBN_SELCHANGE )
            {
                GetFrameRateRange( );
            }
            break;

        case IDC_BUTTON_START:
            if ( wmEvent == BN_CLICKED )
            {
                EnableWindow( gData->hwndStartButton, FALSE );
                EnableWindow( gData->hwndCamerasCombo, FALSE );
                EnableWindow( gData->hwndResolutionsCombo, FALSE );

                std::thread( ToggleStreaming ).detach( );
            }
            break;

        case IDC_EDIT_FRAME_RATE:
            if ( wmEvent == EN_KILLFOCUS )
            {
                EnsureUpDownBuddyInRange( gData->hwndFrameRateSpin, gData->hwndFrameRateEdit );
            }
            break;

        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
        }
        break;

    case WM_SETFOCUS:
        SetFocus( gData->hwndStartButton );
        break;

    case WM_DESTROY:
        if ( gData->streamingStatus->IsStreaming( ) )
        {
            StopVideoStreaming( );
        }

        // save main window's position
        {
            WINDOWPLACEMENT wndPlace = { 0 };

            GetWindowPlacement( hWnd, &wndPlace );

            gData->appConfig->SetMainWindowXY( wndPlace.rcNormalPosition.left, wndPlace.rcNormalPosition.top );

            gData->appConfigSerializer.SaveConfiguration( );
        }

        PostQuitMessage( 0 );
        break;

    case WM_NOTIFY:
        switch ( ( (LPNMHDR) lParam )->code )
        {
        case NM_CLICK:
        case NM_RETURN:
            {
                PNMLINK pNMLink = (PNMLINK) lParam;

                if ( ( ( (LPNMHDR) lParam )->hwndFrom == gData->hwndStatusLink ) && ( pNMLink->item.iLink == 0 ) )
                {
                    ShellExecute( NULL, L"open", pNMLink->item.szUrl, NULL, NULL, SW_SHOW );
                }
            }
            break;
        }
        break;

    case WM_UPDATE_UI:
        {
            BOOL   enableCameraSelection    = TRUE;
            BOOL   enableFrameRateSelection = FALSE;
            int    showStatusLink           = SW_HIDE;
            TCHAR* startButtonText          = STR_START_STREAMING;

            // update UI to reflect current streaming status
            if ( gData->streamingStatus->IsStreaming( ) )
            {
                WCHAR  strStatusLinkText[256];
                WCHAR  strPortNumber[16] = L"";
                string localIp = GetLocalIpAddress( );

                if ( localIp.empty( ) )
                {
                    localIp = "localhost";
                }

                if ( gData->appConfig->HttpPort( ) != 80 )
                {
                    swprintf( strPortNumber, 15, L":%d", gData->appConfig->HttpPort( ) );
                }

                swprintf( strStatusLinkText, 255, L"<a href=\"http://%s%s/\">Streaming on port %d ...</a>",
                          Utf8to16( localIp ).c_str( ), strPortNumber, gData->appConfig->HttpPort( ) );
                SetWindowText( gData->hwndStatusLink, strStatusLinkText );

                startButtonText       = STR_STOP_STREAMING;
                enableCameraSelection = FALSE;
                showStatusLink        = SW_SHOW;
            }
            else
            {
                int minFps, maxFps;

                SendMessage( gData->hwndFrameRateSpin, UDM_GETRANGE32, (WPARAM) &minFps, (LPARAM) &maxFps );

                enableFrameRateSelection = ( minFps != maxFps );
            }

            SetWindowText( gData->hwndStartButton, startButtonText );
            
            if ( !gData->showNoUI )
            {
                ShowWindow( gData->hwndStatusLink, showStatusLink );
            }

            EnableWindow( gData->hwndCamerasCombo, enableCameraSelection );
            EnableWindow( gData->hwndResolutionsCombo, enableCameraSelection );
            EnableWindow( gData->hwndFrameRateEdit, enableFrameRateSelection );
            EnableWindow( gData->hwndStartButton, TRUE );
            SetFocus( gData->hwndStartButton );

            gData->UpdateWebActivity( );
        }
        break;

    case WM_UPDATE_ERROR:
        if ( gData->lastVideoSourceError.empty( ) )
        {
            SendMessage( gData->hwndStatusBar, SB_SETTEXT, 0, (LPARAM) nullptr );
        }
        else
        {
            wstring strError = Utf8to16( gData->lastVideoSourceError );
            SendMessage( gData->hwndStatusBar, SB_SETTEXT, 0, (LPARAM) strError.c_str( ) );
        }
        break;

    case WM_TIMER:
        if ( wParam == TIMER_ID_CAMERA_CONFIG )
        {
            // save camera settings
            gData->cameraConfigSerializer.SaveConfiguration( );
        }
        else if ( wParam == TIMER_ID_FPS_UPDATE )
        {
            gData->UpdateFpsInfo( );
        }
        else if ( wParam == TIMER_ID_ACTIVITY_CHECK )
        {
            gData->UpdateWebActivity( );
        }
        break;

    case WM_SIZE:

        if ( wParam == SIZE_MINIMIZED )
        {
            if ( gData->appConfig->MinimizeToSystemTray( ) )
            {
                MinimizeToTray( hWnd );
            }
        }
        break;

    case WM_SYS_TRAY_NOTIFY:

        if ( LOWORD( lParam ) == WM_LBUTTONUP )
        {
            RestoreFromTray( hWnd );
        }
        break;

    case WM_SETTEXT:
        UpdateTrayTip( hWnd, (WCHAR*) lParam );
        return DefWindowProc( hWnd, message, wParam, lParam );

    case WM_POWERBROADCAST:
        if ( ( wParam == PBT_APMRESUMEAUTOMATIC ) && ( gData->streamingStatus->IsStreaming( ) ) )
        {
            std::thread( RestartStreaming ).detach( );
        }
        break;

    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}

// Message handler for About dialog box
INT_PTR CALLBACK AboutDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    static HICON hIcon = NULL;

    switch ( message )
    {
    case WM_INITDIALOG:
        {
            hIcon = (HICON) LoadImage( gData->hInst, MAKEINTRESOURCE( IDI_ABOUT ), IMAGE_ICON,
                                       GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );

            if ( hIcon )
            {
                SendMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );
            }

            CenterWindowTo( hDlg, GetParent( hDlg ) );

            return (INT_PTR) TRUE;
        }

    case WM_DESTROY:
        if ( hIcon )
        {
            DestroyIcon( hIcon );
        }
        break;

    case WM_COMMAND:
        if ( ( LOWORD( wParam ) == IDOK ) || ( LOWORD( wParam ) == IDCANCEL ) )
        {
            EndDialog( hDlg, LOWORD( wParam ) );
            return (INT_PTR) TRUE;
        }
        break;

    case WM_NOTIFY:
        switch ( ( (LPNMHDR) lParam )->code )
        {
            case NM_CLICK:
            case NM_RETURN:
            {
                PNMLINK pNMLink = (PNMLINK) lParam;

                ShellExecute( NULL, L"open", ( pNMLink->hdr.idFrom == IDC_LINK_EMAIL ) ?
                    L"mailto:cvsandbox@gmail.com" : L"https://github.com/cvsandbox/cam2web", NULL, NULL, SW_SHOW );

                return (INT_PTR) TRUE;
            }
            break;
        }
        break;
    }
    return (INT_PTR) FALSE;
}

// Load some icons to be used by the application
void AppData::LoadAppIcons( )
{
    uint16_t iconIndex = ( appConfig == nullptr ) ? 0 : appConfig->WindowIconIndex( );

    if ( iconIndex > sizeof( AppIconIds ) / sizeof( AppIconIds[0] ) )
    {
        iconIndex = 0;
    }

    if ( hiconAppIcon != NULL )
    {
        DestroyIcon( hiconAppIcon );
    }

    if ( hiconAppActiveIcon != NULL )
    {
        DestroyIcon( hiconAppActiveIcon );
    }

    if ( hiconTrayIcon != NULL )
    {
        DestroyIcon( hiconTrayIcon );
    }

    if ( hiconTrayActiveIcon != NULL )
    {
        DestroyIcon( hiconTrayActiveIcon );
    }

    hiconAppIcon        = LoadIcon( GetModuleHandle( NULL ), MAKEINTRESOURCE( AppIconIds[iconIndex] ) );
    hiconAppActiveIcon  = LoadIcon( GetModuleHandle( NULL ), MAKEINTRESOURCE( AppActiveIconIds[iconIndex] ) );

    hiconTrayIcon       = (HICON) LoadImage( GetModuleHandle( NULL ), MAKEINTRESOURCE( AppIconIds[iconIndex] ), IMAGE_ICON,
                                             GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );

    hiconTrayActiveIcon = (HICON) LoadImage( GetModuleHandle( NULL ), MAKEINTRESOURCE( AppActiveIconIds[iconIndex] ), IMAGE_ICON,
                                             GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );
}

// Update FPS information provided in status bar
void AppData::UpdateFpsInfo( )
{
    if ( ( camera ) && ( camera->IsRunning( ) ) )
    {
        steady_clock::time_point timeNow   = steady_clock::now( );
        auto                     timeTaken = duration_cast<std::chrono::milliseconds>( timeNow - lastFpsCheck ).count( );
        uint32_t                 framesGot = camera->FramesReceived( );
        float                    fpsValue  = (float) ( framesGot - lastFramesGot ) / ( (float) timeTaken / 1000.0f );

        if ( lastFpsValues.size( ) < FPS_HISTORY_LENGTH )
        {
            lastFpsValues.push_back( fpsValue );
        }
        else
        {
            lastFpsValues[nextFpsIndex] = fpsValue;

            nextFpsIndex++;
            nextFpsIndex %= FPS_HISTORY_LENGTH;
        }

        // get average value
        float fpsAvg = std::accumulate( lastFpsValues.begin( ), lastFpsValues.end( ), 0.0f ) / lastFpsValues.size( );
        WCHAR strFps[32];

        swprintf( strFps, 31, L"FPS: %0.1f", fpsAvg );
        SendMessage( gData->hwndStatusBar, SB_SETTEXT, 1, (LPARAM) strFps );

        lastFramesGot = framesGot;
        lastFpsCheck  = timeNow;
    }
    else
    {
        SendMessage( gData->hwndStatusBar, SB_SETTEXT, 1, (LPARAM) nullptr );
    }
}

// Check last time web server was accessed
void AppData::UpdateWebActivity( )
{
    bool    wasAccessedAtAll    = false;
    auto    timeSinceLastAccess = duration_cast<std::chrono::milliseconds>( steady_clock::now( ) - server.LastAccessTime( &wasAccessedAtAll ) ).count( );
    bool    activeIcon          = ( streamingStatus->IsStreaming( ) ) && ( wasAccessedAtAll ) && ( timeSinceLastAccess <= 1000 );

    NOTIFYICONDATA notifyData = { };

    notifyData.cbSize = sizeof( notifyData );
    notifyData.hWnd   = hwndMain;
    notifyData.uID    = IDC_SYS_TRAY_ID;
    notifyData.uFlags = NIF_ICON;
    notifyData.hIcon  = ( activeIcon ) ? hiconTrayActiveIcon : hiconTrayIcon;;

    Shell_NotifyIcon( NIM_MODIFY, &notifyData );

    SendMessage( hwndMain, WM_SETICON, ICON_SMALL, (LRESULT) ( ( activeIcon ) ? hiconAppActiveIcon : hiconAppIcon ) );
}

// Start administration web server (if requested)
void AppData::StartAdminServer( )
{
    if ( adminPort != 0 )
    {
        // some read-only information about the version
        PropertyMap versionInfo;

        versionInfo.insert( PropertyMap::value_type( "product", STR_INFO_PRODUCT ) );
        versionInfo.insert( PropertyMap::value_type( "version", STR_INFO_VERSION ) );
        versionInfo.insert( PropertyMap::value_type( "platform", STR_INFO_PLATFORM ) );

        // set authentication domain and load users' list
        adminServer.SetAuthDomain( appConfig->AuthDomain( ) );
        adminServer.SetAuthenticationMethod( ( appConfig->AuthenticationMethod( ) == "basic" ) ?
                                             Authentication::Basic : Authentication::Digest );
        adminServer.LoadUsersFromFile( appConfig->UsersFileName( ) );

        // configure web server and handler
        adminServer.SetPort( adminPort ).
                    AddHandler( make_shared<XObjectInformationRequestHandler>( "/version", make_shared<XObjectInformationMap>( versionInfo ) ), UserGroup::Admin ).
                    AddHandler( make_shared<XObjectConfigurationRequestHandler>( "/status", streamingStatus ), UserGroup::Admin );

#ifdef _DEBUG
        // load web content from files in debug builds
        adminServer.SetDocumentRoot( "./web/" );
#else
        // web content is embeded in release builds to get single executable
        adminServer.AddHandler( make_shared<XEmbeddedContentHandler>( "/", &web_admin_html ), UserGroup::Admin ).
            AddHandler( make_shared<XEmbeddedContentHandler>( "index.html", &web_admin_html ), UserGroup::Admin ).
            AddHandler( make_shared<XEmbeddedContentHandler>( "styles.css", &web_styles_css ), UserGroup::Admin ).
                    AddHandler( make_shared<XEmbeddedContentHandler>( "cam2web.png", &web_cam2web_png ) ).
                    AddHandler( make_shared<XEmbeddedContentHandler>( "cam2web_white.png", &web_cam2web_white_png ) ).
                    AddHandler( make_shared<XEmbeddedContentHandler>( "jquery.js", &web_jquery_js ), UserGroup::Admin ).
                    AddHandler( make_shared<XEmbeddedContentHandler>( "jquery.mobile.js", &web_jquery_mobile_js ), UserGroup::Admin ).
                    AddHandler( make_shared<XEmbeddedContentHandler>( "jquery.mobile.css", &web_jquery_mobile_css ), UserGroup::Admin );
#endif

        if ( !adminServer.Start( ) )
        {
            MessageBox( hwndMain, TEXT( "Failed starting administration web server." ), STR_ERROR, MB_OK | MB_ICONERROR );
        }
    }
}

// Set property of streaming status
XError StreamingStatusController::SetProperty( const string& propertyName, const string& value )
{
    XError ret = XError::Success;
    int    numericValue;

    // currently all setting are numeric, so scan it
    int scannedCount = sscanf( value.c_str( ), "%d", &numericValue );

    if ( scannedCount != 1 )
    {
        ret = XError::InvalidPropertyValue;
    }
    else if ( propertyName == STR_RUNNING )
    {
        bool newStatus = ( numericValue == 1 );

        if ( newStatus != streamingInProgress )
        {
            // toggle streaming
            PostMessage( gData->hwndMain, WM_COMMAND, MAKELONG( IDC_BUTTON_START, BN_CLICKED ), 0 );
        }
    }
    else
    {
        ret = XError::UnknownProperty;
    }

    return ret;
}

// Get property of streaming status
XError StreamingStatusController::GetProperty( const string& propertyName, string& value ) const
{
    XError ret = XError::Success;
    int    numericValue = 0;

    if ( propertyName == STR_RUNNING )
    {
        numericValue = ( streamingInProgress ) ? 1 : 0;
    }
    else
    {
        ret = XError::UnknownProperty;
    }

    if ( ret == XError::Success )
    {
        char buffer[32];

        sprintf( buffer, "%d", numericValue );
        value = buffer;
    }

    return ret;
}

// Get all properties of streaming status controller
map<string, string> StreamingStatusController::GetAllProperties( ) const
{
    map<string, string> properties;

    properties.insert( pair<string, string>( STR_RUNNING, ( streamingInProgress ) ? "1" : "0" ) );

    return properties;
}
