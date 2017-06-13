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

#include "resource.h"
#include "Tools.hpp"
#include "UiTools.hpp"
#include "SettingsDialog.hpp"
#include "AccessRightsDialog.hpp"

#include "XLocalVideoDevice.hpp"
#include "XLocalVideoDeviceConfig.hpp"
#include "XWebServer.hpp"
#include "XVideoSourceToWeb.hpp"
#include "XObjectConfigurationSerializer.hpp"
#include "XObjectConfigurationRequestHandler.hpp"
#include "XObjectInformationRequestHandler.hpp"
#include "AppConfig.hpp"

// Release build embeds web resources into executable
#ifdef NDEBUG
    #include "index.html.h"
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

#define MAX_LOADSTRING          (100)

#define IDC_STATIC_CAMERAS      (501)
#define IDC_COMBO_CAMERAS       (502)
#define IDC_STATIC_RESOLUTIONS  (503)
#define IDC_COMBO_RESOLUTIONS   (504)
#define IDC_BUTTON_START        (505)
#define IDC_LINK_STATUS         (506)

#define STR_ERROR               TEXT( "Error" )
#define STR_START_STREAMING     TEXT( "&Start streaming" )
#define STR_STOP_STREAMING      TEXT( "&Stop streaming" )

#define WM_UPDATE_UI            (WM_USER + 1)

#define TIMER_ID_EVENT          (0xB0B)

// Forward declarations of functions included in this code module:
LRESULT CALLBACK MainWndProc( HWND, UINT, WPARAM, LPARAM );
INT_PTR CALLBACK AboutDlgProc( HWND, UINT, WPARAM, LPARAM );
void GetVideoDevices( );

// Place holder for all global variable the application needs
class AppData
{
public:
    TCHAR       szTitle[MAX_LOADSTRING];            // title bar text
    TCHAR       szWindowClass[MAX_LOADSTRING];      // the main window class name
    HINSTANCE   hInst;                              // current instance

    HWND        hwndMain;
    HWND        hwndCamerasCombo;
    HWND        hwndResolutionsCombo;
    HWND        hwndStartButton;
    HWND        hwndStatusLink;

    vector<XDeviceName>             devices;
    vector<XDeviceCapabilities>     cameraCapabilities;
    shared_ptr<XLocalVideoDevice>   camera;
    XDeviceName                     selectedDeviceName;
    XDeviceCapabilities             selectedResolutuion;
    shared_ptr<IObjectConfigurator> cameraConfig;
    shared_ptr<AppConfig>           appConfig;

    XWebServer                      server;
    XVideoSourceToWeb               video2web;

    bool                            streamingInProgress;

    string                          appFolder;
    string                          appConfigFile;

    XObjectConfigurationSerializer  appConfigSerializer;
    XObjectConfigurationSerializer  cameraConfigSerializer;

    AppData( ) :
        hInst( NULL ), hwndMain( NULL ), hwndCamerasCombo( NULL ),
        hwndResolutionsCombo( NULL ), hwndStartButton( NULL ), hwndStatusLink( NULL ),
        devices( ), cameraCapabilities( ), camera( ), selectedDeviceName( ), selectedResolutuion( ),
        cameraConfig( ), appConfig( new AppConfig( ) ), server( ), video2web( ),
        streamingInProgress( false ),
        appFolder( ".\\" ), appConfigFile( "cam2web.cfg" ),
        appConfigSerializer( ), cameraConfigSerializer( )
    {
        // find user' home folder to store settings
        WCHAR homeFolder[MAX_PATH];

        if ( SUCCEEDED( SHGetFolderPathW( NULL, CSIDL_PROFILE, NULL, 0, homeFolder ) ) )
        {
            appFolder = Utf16to8( homeFolder );
            appFolder += "\\cam2web\\";
        }

        appConfigFile = appFolder + "app.cfg";

        CreateDirectory( Utf8to16( appFolder ).c_str( ), nullptr );

        appConfigSerializer = XObjectConfigurationSerializer( appConfigFile, appConfig );

        // load application settings
        appConfigSerializer.LoadConfiguration( );
        appConfig->SetUsersFileName( appFolder + "users.txt" );
    }
};
AppData* gData = NULL;

// Register class of the main window
ATOM MyRegisterClass( HINSTANCE hInstance )
{
    WNDCLASSEX wcex;

    wcex.cbSize        = sizeof( WNDCLASSEX );
    wcex.style         = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc   = MainWndProc;
    wcex.cbClsExtra    = 0;
    wcex.cbWndExtra    = 0;
    wcex.hInstance     = hInstance;
    wcex.hIcon         = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_CAM2WEB ) );
    wcex.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = (HBRUSH) ( COLOR_WINDOW );
    wcex.lpszMenuName  = MAKEINTRESOURCE( IDC_CAM2WEB );
    wcex.lpszClassName = gData->szWindowClass;
    wcex.hIconSm       = LoadIcon( wcex.hInstance, MAKEINTRESOURCE( IDI_CAM2WEB ) );

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
BOOL CreateMainWindow( HINSTANCE hInstance, int nCmdShow )
{
    INITCOMMONCONTROLSEX initControls = { sizeof( INITCOMMONCONTROLSEX ), ICC_LINK_CLASS };

    InitCommonControlsEx( &initControls );

    HWND hwndMain = CreateWindow( gData->szWindowClass, gData->szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, NULL, NULL, hInstance, NULL );
    if ( hwndMain == NULL )
    {
        return FALSE;
    }

    gData->hwndMain = hwndMain;

    // cameras' combo and label
    HWND hWindLabel = CreateWindow( WC_STATIC, TEXT( "&Camera:" ), WS_CHILD | WS_VISIBLE | WS_GROUP,
        10, 14, 60, 20, hwndMain, (HMENU) IDC_STATIC_CAMERAS, hInstance, NULL );

    gData->hwndCamerasCombo = CreateWindow( WC_COMBOBOX, TEXT( "" ),
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VSCROLL | WS_VISIBLE | WS_TABSTOP,
        70, 10, 200, 100, hwndMain, (HMENU) IDC_COMBO_CAMERAS, hInstance, NULL );

    // resolutions' combo and label
    hWindLabel = CreateWindow( WC_STATIC, TEXT( "&Resolution:" ), WS_CHILD | WS_VISIBLE,
        10, 39, 60, 20, hwndMain, (HMENU) IDC_STATIC_RESOLUTIONS, hInstance, NULL );

    gData->hwndResolutionsCombo = CreateWindow( WC_COMBOBOX, TEXT( "" ),
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VSCROLL | WS_VISIBLE | WS_TABSTOP,
        70, 35, 200, 150, hwndMain, (HMENU) IDC_COMBO_RESOLUTIONS, hInstance, NULL );

    // streaming start/stop button and link
    gData->hwndStartButton = CreateWindow( WC_BUTTON, STR_START_STREAMING,
        WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_TABSTOP,
        10, 70, 260, 40, hwndMain, (HMENU) IDC_BUTTON_START, hInstance, NULL );

    gData->hwndStatusLink = CreateWindowEx( 0, WC_LINK, TEXT( "" ),
        WS_CHILD | WS_TABSTOP,
        10, 120, 260, 20, hwndMain, (HMENU) IDC_LINK_STATUS, hInstance, NULL );

    // set default font for the window and its childrent
    HGDIOBJ hFont = GetStockObject( DEFAULT_GUI_FONT );

    SendMessage( hwndMain, WM_SETFONT, (WPARAM) hFont, TRUE );
    EnumChildWindows( hwndMain, SetWindowFont, (LPARAM) hFont );

    // ----
    GetVideoDevices( );

    CenterWindowTo( hwndMain, GetDesktopWindow( ) );
    ShowWindow( hwndMain, nCmdShow );
    UpdateWindow( hwndMain );

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

    MyRegisterClass( hInstance );

    // create main window of the application
    if ( CreateMainWindow( hInstance, nCmdShow ) )
    {
        HACCEL hAccelTable = LoadAccelerators( hInstance, MAKEINTRESOURCE( IDC_CAM2WEB ) );
        MSG    msg;

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

// Create video source object for the selected device and get its available resolutions
static void CreateDeviceAndGetResolutions( )
{
    int cameraIndex = SendMessage( gData->hwndCamerasCombo, (UINT) CB_GETCURSEL, 0, 0 );

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
                swprintf( strResolution, 255, TEXT( "%d x %d, %d bpp, %d fps" ), cap.Width( ), cap.Height( ), cap.BitCount( ), cap.MaximumFrameRate( ) );

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
        }
    }
}

// Populate list of available devices
void GetVideoDevices( )
{
    gData->devices = XLocalVideoDevice::GetAvailableDevices( );

    if ( gData->devices.empty( ) )
    {
        SendMessage( gData->hwndCamerasCombo, CB_ADDSTRING, 0, (LPARAM) TEXT( "No video devices found" ) );

        EnableWindow( gData->hwndCamerasCombo, FALSE );
        EnableWindow( gData->hwndResolutionsCombo, FALSE );
        EnableWindow( gData->hwndStartButton, FALSE );
    }
    else
    {
        const string lastUsedCameraMoniker = gData->appConfig->CameraMoniker( );
        TCHAR        deviceName[256];
        int          indexToSelect = 0;
        int          index = 0;

        for ( auto device : gData->devices )
        {
            _tcsncpy( deviceName, Utf8to16( device.Name( ) ).c_str( ), 255 );

            SendMessage( gData->hwndCamerasCombo, CB_ADDSTRING, 0, (LPARAM) deviceName );

            if ( device.Moniker( ) == lastUsedCameraMoniker )
            {
                indexToSelect = index;
            }

            index++;
        }

        SendMessage( gData->hwndCamerasCombo, CB_SETCURSEL, indexToSelect, 0 );
        CreateDeviceAndGetResolutions( );
    }
}

// Start streaming of the selected video source
static bool StartVideoStreaming( )
{
    bool ret = false;

    if ( gData->camera )
    {
        string    cameraMoniker   = gData->selectedDeviceName.Moniker( );
        int       resolutionIndex = SendMessage( gData->hwndResolutionsCombo, (UINT) CB_GETCURSEL, 0, 0 );
        UserGroup viewersGroup    = static_cast<UserGroup>( gData->appConfig->ViewersGroup( ) );
        UserGroup configGroup     = static_cast<UserGroup>( gData->appConfig->ConfiguratorsGroup( ) );

        if ( ( resolutionIndex >= 0 ) && ( resolutionIndex < (int) gData->cameraCapabilities.size( ) ) )
        {
            gData->selectedResolutuion = gData->cameraCapabilities[resolutionIndex];
            gData->camera->SetResolution( gData->selectedResolutuion );
        }

        // remember selected camera and resolution
        gData->appConfig->SetCameraMoniker( cameraMoniker );
        gData->appConfig->SetLastVideoResolution( static_cast<uint16_t>( gData->selectedResolutuion.Width( ) ),
                                                  static_cast<uint16_t>( gData->selectedResolutuion.Height( ) ),
                                                  static_cast<uint16_t>( gData->selectedResolutuion.BitCount( ) ),
                                                  static_cast<uint16_t>( gData->selectedResolutuion.MaximumFrameRate( ) ) );
        gData->appConfigSerializer.SaveConfiguration( );

        // prepare some read-only informational properties of the camera
        PropertyMap cameraInfo;
        char        strVideoSize[32];

        sprintf( strVideoSize,      "%d", gData->selectedResolutuion.Width( ) );
        sprintf( strVideoSize + 16, "%d", gData->selectedResolutuion.Height( ) );

        cameraInfo.insert( PropertyMap::value_type( "device", gData->selectedDeviceName.Name( ) ) );
        cameraInfo.insert( PropertyMap::value_type( "width",  strVideoSize ) );
        cameraInfo.insert( PropertyMap::value_type( "height", strVideoSize + 16 ) );

        // allow camera configuration through simplified configurator object
        gData->cameraConfig = make_shared<XLocalVideoDeviceConfig>( gData->camera );

        // use MD5 of camera's moniker as file name to store camera's setting - just to avoid unfriendly characters which may happen in the moniker
        string cameraSettingsFileName = gData->appFolder + GetMd5Hash( (const uint8_t*) cameraMoniker.c_str( ), cameraMoniker.length( ) ) + ".cfg";

        // restore camera settings
        gData->cameraConfigSerializer = XObjectConfigurationSerializer( cameraSettingsFileName, gData->cameraConfig );
        gData->cameraConfigSerializer.LoadConfiguration( );

        // set JPEG quality
        gData->video2web.SetJpegQuality( gData->appConfig->JpegQuality( ) );

        // set authentication domain and load users' list
        gData->server.SetAuthDomain( gData->appConfig->AuthDomain( ) );
        gData->server.LoadUsersFromFile( gData->appConfig->UsersFileName( ) );

        // configure web server and handler
        gData->server.SetPort( gData->appConfig->HttpPort( ) ).
                      AddHandler( make_shared<XObjectConfigurationRequestHandler>( "/camera/config", gData->cameraConfig ), configGroup ).
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
                          AddHandler( make_shared<XEmbeddedContentHandler>( "cameraproperties.html", &web_cameraproperties_directshow_html ), configGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "jquery.js", &web_jquery_js ), viewersGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "jquery.mobile.js", &web_jquery_mobile_js ), viewersGroup ).
                          AddHandler( make_shared<XEmbeddedContentHandler>( "jquery.mobile.css", &web_jquery_mobile_css ), viewersGroup );
#endif
        }

        gData->camera->SetListener( gData->video2web.VideoSourceListener( ) );

        if ( !gData->camera->Start( ) )
        {
            MessageBox( gData->hwndMain, TEXT( "Failed starting video source" ), STR_ERROR, MB_OK | MB_ICONERROR );
        }
        else if ( !gData->server.Start( ) )
        {
            MessageBox( gData->hwndMain, TEXT( "Failed starting web server" ), STR_ERROR, MB_OK | MB_ICONERROR );
            gData->camera->SignalToStop( );
            gData->camera->WaitForStop( );
        }
        else
        {
            wstring newWindowTitle = gData->szTitle;

            newWindowTitle += L" :: ";
            newWindowTitle += Utf8to16( gData->selectedDeviceName.Name( ) );

            SetWindowText( gData->hwndMain, newWindowTitle.c_str( ) );

            // setup timer to save camera configuration from time to time
            SetTimer( gData->hwndMain, TIMER_ID_EVENT, 60000, NULL );

            ret = true;
        }
    }

    return ret;
}

// Stop streaming of the current video source
static void StopVideoStreaming( )
{
    KillTimer( gData->hwndMain, TIMER_ID_EVENT );

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

    SetWindowText( gData->hwndMain, gData->szTitle );
}

// Toggle video streaming state
static void ToggleStreaming( )
{
    // start/stop streaming
    if ( gData->streamingInProgress )
    {
        StopVideoStreaming( );
        gData->streamingInProgress = false;
    }
    else
    {
        if ( StartVideoStreaming( ) )
        {
            gData->streamingInProgress = true;
        }
    }

    // update UI controls
    PostMessage( gData->hwndMain, WM_UPDATE_UI, 0, 0 );
}

// Main window's message handler
LRESULT CALLBACK MainWndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
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

        case IDC_BUTTON_START:
            if ( wmEvent == BN_CLICKED )
            {
                EnableWindow( gData->hwndStartButton, FALSE );
                EnableWindow( gData->hwndCamerasCombo, FALSE );
                EnableWindow( gData->hwndResolutionsCombo, FALSE );

                std::async( ToggleStreaming );
            }
        default:
            return DefWindowProc( hWnd, message, wParam, lParam );
        }
        break;

    case WM_SETFOCUS:
        SetFocus( gData->hwndStartButton );
        break;

    case WM_DESTROY:
        if ( gData->streamingInProgress )
        {
            StopVideoStreaming( );
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
                LITEM   item    = pNMLink->item;

                if ( ( ( (LPNMHDR) lParam )->hwndFrom == gData->hwndStatusLink ) && ( item.iLink == 0 ) )
                {
                    ShellExecute( NULL, L"open", item.szUrl, NULL, NULL, SW_SHOW );
                }
            }
            break;
        }
        break;

    case WM_UPDATE_UI:
        {
            BOOL   enableCameraSelection = TRUE;
            int    showStatusLink        = SW_HIDE;
            TCHAR* startButtonText       = STR_START_STREAMING;

            // update UI to reflect current streaming status
            if ( gData->streamingInProgress )
            {
                TCHAR strStatusLinkText[256];

                swprintf( strStatusLinkText, 255, TEXT( "<a href=\"http://localhost:%d/\">Streaming on port %d ...</a>" ),
                    gData->appConfig->HttpPort( ), gData->appConfig->HttpPort( ) );
                SetWindowText( gData->hwndStatusLink, strStatusLinkText );
                
                startButtonText       = STR_STOP_STREAMING;
                enableCameraSelection = FALSE;
                showStatusLink        = SW_SHOW;
            }

            SetWindowText( gData->hwndStartButton, startButtonText );
            ShowWindow( gData->hwndStatusLink, showStatusLink );
            EnableWindow( gData->hwndCamerasCombo, enableCameraSelection );
            EnableWindow( gData->hwndResolutionsCombo, enableCameraSelection );
            EnableWindow( gData->hwndStartButton, TRUE );
            SetFocus( gData->hwndStartButton );
        }
        break;

    case WM_TIMER:
        if ( wParam == TIMER_ID_EVENT )
        {
            // save camera settings
            gData->cameraConfigSerializer.SaveConfiguration( );
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
