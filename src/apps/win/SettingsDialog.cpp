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

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>

#include "resource.h"
#include "SettingsDialog.hpp"
#include "Tools.hpp"
#include "UiTools.hpp"
#include "AppConfig.hpp"

using namespace std;

#define STR_ERROR   TEXT( "Error" )

static int CALLBACK BrowseFolderCallback( HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData );

// Message handler for Settings dialog box
INT_PTR CALLBACK SettingsDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    static const WCHAR* iconColors[]  = { L"Blue", L"Green", L"Orange", L"Red" };
    static const WCHAR* authMethods[] = { L"Basic", L"Digest" };

    static AppConfig* appConfig   = nullptr;
    static HICON      hIcon       = NULL;
    static HICON      hFolderIcon = NULL;
    int               wmId;
    int               wmEvent;

    switch ( message )
    {
    case WM_INITDIALOG:
        {
            uint16_t iconColor = 0;
            uint16_t authMethodIndex = 1;

            // load icons
            if ( hIcon == NULL )
            {
                hIcon = (HICON) LoadImage( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_SETTINGS ), IMAGE_ICON,
                                           GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );
            
            }

            if ( hFolderIcon == NULL )
            {
                hFolderIcon = (HICON) LoadImage( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_FOLDER ), IMAGE_ICON,
                                                 GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );

            }

            // set icons
            if ( hIcon )
            {
                SendMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );
            }

            if ( hFolderIcon )
            {
                SendMessage( GetDlgItem( hDlg, IDC_CUSTOM_WEB_BUTTON ), BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hFolderIcon );
            }

            // configure icon colors' combo box
            HWND hwndIconColorCombo = GetDlgItem( hDlg, IDC_ICON_COLOR_COMBO );

            for ( int i = 0; i < sizeof( iconColors ) / sizeof( iconColors[0] ); i++ )
            {
                SendMessage( hwndIconColorCombo, CB_ADDSTRING, 0, (LPARAM) iconColors[i] );
            }

            // configure auth methods combo box
            HWND hwndAuthMethodCombo = GetDlgItem( hDlg, IDC_AUTH_METHOD_COMBO );

            for ( int i = 0; i < sizeof( authMethods ) / sizeof( authMethods[0] ); i++ )
            {
                SendMessage( hwndAuthMethodCombo, CB_ADDSTRING, 0, ( LPARAM ) authMethods[i] );
            }

            CenterWindowTo( hDlg, GetParent( hDlg ) );

            // get current configuration
            appConfig = (AppConfig*) lParam;

            if ( appConfig != nullptr )
            {
                InitUpDownControl( GetDlgItem( hDlg, IDC_JPEG_Q_SPIN ), GetDlgItem( hDlg, IDC_JPEG_Q_EDIT ), 1, 100, appConfig->JpegQuality( ) );
                InitUpDownControl( GetDlgItem( hDlg, IDC_MJPEG_RATE_SPIN ), GetDlgItem( hDlg, IDC_MJPEG_RATE_EDIT ), 1, 30, appConfig->MjpegFrameRate( ) );
                InitUpDownControl( GetDlgItem( hDlg, IDC_HTTP_PORT_SPIN ), GetDlgItem( hDlg, IDC_HTTP_PORT_EDIT ), 1, 65535, appConfig->HttpPort( ) );

                wstring strWebContent = Utf8to16( appConfig->CustomWebContent( ) );
                SetWindowText( GetDlgItem( hDlg, IDC_CUSTOM_WEB_EDIT ), strWebContent.c_str( ) );

                wstring strCameraTitle = Utf8to16( appConfig->CameraTitle( ) );
                SetWindowText( GetDlgItem( hDlg, IDC_CAMERA_TITLE_EDIT ), strCameraTitle.c_str( ) );

                SendMessage( GetDlgItem( hDlg, IDC_OVERLAY_TIMESTAMP ), BM_SETCHECK, ( appConfig->TimestampOverlay( ) ) ? BST_CHECKED : BST_UNCHECKED, 0 );
                SendMessage( GetDlgItem( hDlg, IDC_OVERLAY_TITLE ), BM_SETCHECK, ( appConfig->CameraTitleOverlay( ) ) ? BST_CHECKED : BST_UNCHECKED, 0 );

                string strOverlayTextColor = XargbToString( appConfig->OverlayTextColor( ) );
                SetWindowTextA( GetDlgItem( hDlg, IDC_OVERLAY_TEXT_COLOR ), strOverlayTextColor.c_str( ) );

                string strOverlayBgColor = XargbToString( appConfig->OverlayBackgroundColor( ) );
                SetWindowTextA( GetDlgItem( hDlg, IDC_OVERLAY_BG_COLOR ), strOverlayBgColor.c_str( ) );

                SendMessage( GetDlgItem( hDlg, IDC_SYS_TRAY_CHECK ), BM_SETCHECK, ( appConfig->MinimizeToSystemTray( ) ) ? BST_CHECKED : BST_UNCHECKED, 0 );

                iconColor = appConfig->WindowIconIndex( );
                if ( iconColor >= 4 )
                {
                    iconColor = 0;
                }

                authMethodIndex = ( appConfig->AuthenticationMethod( ) == "basic" ) ? 0 : 1;
            }

            SendMessage( hwndIconColorCombo, CB_SETCURSEL, iconColor, 0 );
            SendMessage( hwndAuthMethodCombo, CB_SETCURSEL, authMethodIndex, 0 );

            return (INT_PTR) TRUE;
        }

    case WM_DESTROY:
        appConfig = nullptr;

        if ( hIcon )
        {
            DestroyIcon( hIcon );
        }

        return (INT_PTR) TRUE;

    case WM_COMMAND:
        wmId    = LOWORD( wParam );
        wmEvent = HIWORD( wParam );

        if ( ( wmId == IDOK ) || ( wmId == IDCANCEL ) )
        {
            bool allFine = true;

            if ( wmId == IDOK )
            {
                if ( appConfig )
                {
                    appConfig->SetJpegQuality( (uint16_t) SendMessage( GetDlgItem( hDlg, IDC_JPEG_Q_SPIN ), UDM_GETPOS32, 0, 0 ) );
                    appConfig->SetMjpegFrameRate( (uint16_t) SendMessage( GetDlgItem( hDlg, IDC_MJPEG_RATE_SPIN ), UDM_GETPOS32, 0, 0 ) );
                    appConfig->SetHttpPort( (uint16_t) SendMessage( GetDlgItem( hDlg, IDC_HTTP_PORT_SPIN ), UDM_GETPOS32, 0, 0 ) );
                    appConfig->SetMinimizeToSystemTray( SendMessage( GetDlgItem( hDlg, IDC_SYS_TRAY_CHECK ), BM_GETCHECK, 0, 0 ) == BST_CHECKED );
                    appConfig->SetWindowIconIndex( (uint16_t) SendMessage( GetDlgItem( hDlg, IDC_ICON_COLOR_COMBO ), CB_GETCURSEL, 0, 0 ) );
                    appConfig->SetAuthenticationMethod( ( SendMessage( GetDlgItem( hDlg, IDC_AUTH_METHOD_COMBO ), CB_GETCURSEL, 0, 0 ) == 0 ) ? "basic" : "digest" );

                    appConfig->SetCustomWebContent( GetWindowString( GetDlgItem( hDlg, IDC_CUSTOM_WEB_EDIT ), true ) );
                    appConfig->SetCameraTitle( GetWindowString( GetDlgItem( hDlg, IDC_CAMERA_TITLE_EDIT ), true ) );

                    appConfig->SetTimestampOverlay( SendMessage( GetDlgItem( hDlg, IDC_OVERLAY_TIMESTAMP ), BM_GETCHECK, 0, 0 ) == BST_CHECKED );
                    appConfig->SetCameraTitleOverlay( SendMessage( GetDlgItem( hDlg, IDC_OVERLAY_TITLE ), BM_GETCHECK, 0, 0 ) == BST_CHECKED );

                    string strOverlayTextColor = GetWindowString( GetDlgItem( hDlg, IDC_OVERLAY_TEXT_COLOR ), true );
                    string strOverlayBgColor   = GetWindowString( GetDlgItem( hDlg, IDC_OVERLAY_BG_COLOR ), true );
                    xargb  overlayTextColor    = { 0xFF000000 };
                    xargb  overlayBgColor      = { 0xFF000000 };

                    if ( StringToXargb( strOverlayTextColor, &overlayTextColor ) )
                    {
                        appConfig->SetOverlayTextColor( overlayTextColor );
                    }
                    else
                    {
                        CenteredMessageBox( hDlg, L"Invalid overlay text color. Must be 6 or 8 HEX string (HTML format).", STR_ERROR, MB_OK | MB_ICONERROR );
                        allFine = false;
                    }

                    if ( StringToXargb( strOverlayBgColor, &overlayBgColor ) )
                    {
                        appConfig->SetOverlayBackgroundColor( overlayBgColor );
                    }
                    else
                    {
                        CenteredMessageBox( hDlg, L"Invalid overlay background color. Must be 6 or 8 HEX string (HTML format).", STR_ERROR, MB_OK | MB_ICONERROR );
                        allFine = false;
                    }
                }
            }

            if ( allFine )
            {
                EndDialog( hDlg, wmId );
            }
            return (INT_PTR) TRUE;
        }
        else if ( wmEvent == EN_CHANGE )
        {
            if ( wmId == IDC_JPEG_Q_EDIT )
            {
                EnsureUpDownBuddyInRange( GetDlgItem( hDlg, IDC_JPEG_Q_SPIN ), GetDlgItem( hDlg, IDC_JPEG_Q_EDIT ) );
            }
            else if ( wmId == IDC_MJPEG_RATE_EDIT )
            {
                EnsureUpDownBuddyInRange( GetDlgItem( hDlg, IDC_MJPEG_RATE_SPIN ), GetDlgItem( hDlg, IDC_MJPEG_RATE_EDIT ) );
            }
            else if ( wmId == IDC_HTTP_PORT_EDIT )
            {
                EnsureUpDownBuddyInRange( GetDlgItem( hDlg, IDC_HTTP_PORT_SPIN ), GetDlgItem( hDlg, IDC_HTTP_PORT_EDIT ) );
            }
            else
            {
                break;
            }
            
            return (INT_PTR) TRUE;
        }
        else if ( wmId == IDC_CUSTOM_WEB_BUTTON )
        {
            BROWSEINFO browseInfo = { 0 };

            browseInfo.hwndOwner = hDlg;
            browseInfo.lpszTitle = L"Select custom web root folder:";
            browseInfo.ulFlags   = BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON | BIF_RETURNONLYFSDIRS;
            browseInfo.lParam    = reinterpret_cast<LPARAM>( hDlg );
            browseInfo.lpfn      = BrowseFolderCallback;

            PIDLIST_ABSOLUTE pidl = SHBrowseForFolder( &browseInfo );

            if ( pidl != NULL )
            {
                WCHAR selectedPath[MAX_PATH];

                if ( SHGetPathFromIDList( pidl, selectedPath ) )
                {
                    SetWindowText( GetDlgItem( hDlg, IDC_CUSTOM_WEB_EDIT ), selectedPath );
                }
                else
                {
                    CenteredMessageBox( hDlg, L"Invalid folder was selected.", L"Error", MB_OK | MB_ICONERROR );
                }
            }
        }
        break;
    }

    return (INT_PTR) FALSE;
}

// Callback for the dialog used to select fodler
int CALLBACK BrowseFolderCallback( HWND hwnd, UINT uMsg, LPARAM /* lParam */, LPARAM lpData )
{
    if ( uMsg == BFFM_INITIALIZED )
    {
        HWND    hwndParent         = reinterpret_cast<HWND>( lpData );
        wstring strCustomWebFolder = Utf8to16( GetWindowString( GetDlgItem( hwndParent, IDC_CUSTOM_WEB_EDIT ), true ) );

        SendMessage( hwnd, BFFM_SETSELECTION, TRUE, (LPARAM) strCustomWebFolder.c_str( ) );

        CenterWindowTo( hwnd, GetParent( hwnd ) );
    }

    return 0;
}
