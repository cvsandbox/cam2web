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

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <windows.h>
#include <commctrl.h>

#include "resource.h"
#include "SettingsDialog.hpp"
#include "Tools.hpp"
#include "UiTools.hpp"
#include "AppConfig.hpp"

#include "XWebServer.hpp"

using namespace std;

static const WCHAR* STR_ANYONE = L"Anyone";
static const WCHAR* STR_USERS  = L"Users";
static const WCHAR* STR_ADMINS = L"Admins";

// Initialize list view of existing users
static void InitUsersListView( HWND hwndListView )
{
    // add columns to the list view
    LVCOLUMN    lvc;

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt  = LVCFMT_LEFT;

    lvc.iSubItem = 0;
    lvc.pszText  = L"Name";
    lvc.cx       = 200;

    ListView_InsertColumn( hwndListView, 0, &lvc );

    lvc.iSubItem = 1;
    lvc.pszText  = L"Role";
    lvc.cx       = 100;

    ListView_InsertColumn( hwndListView, 1, &lvc );

    // add image list to the list view
    HMODULE     hModule    = GetModuleHandle( NULL );
    HIMAGELIST  hImageList = ImageList_Create( GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), ILC_COLOR32, 2, 2 );
    HICON       hiconItem;

    hiconItem = LoadIcon( hModule, MAKEINTRESOURCE( IDI_USER ) );
    ImageList_AddIcon( hImageList, hiconItem );
    DestroyIcon( hiconItem );

    hiconItem = LoadIcon( hModule, MAKEINTRESOURCE( IDI_ADMIN ) );
    ImageList_AddIcon( hImageList, hiconItem );
    DestroyIcon( hiconItem );

    ListView_SetImageList( hwndListView, hImageList, LVSIL_SMALL );

    //
    ListView_SetExtendedListViewStyle( hwndListView, LVS_EX_FULLROWSELECT );
}

static void AddUserToListView( HWND hwndListView, const WCHAR* szName, UserGroup userGroup )
{
    LVITEM lvI;

    lvI.pszText   = (WCHAR*) szName;
    lvI.mask      = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    lvI.stateMask = 0;
    lvI.iSubItem  = 0;
    lvI.state     = 0;
    lvI.iImage    = ( userGroup == UserGroup::Admin ) ? 1 : 0;
    lvI.iItem     = ListView_GetItemCount( hwndListView );

    ListView_InsertItem( hwndListView, &lvI );

    lvI.pszText   = ( userGroup == UserGroup::Admin ) ? L"Admin" : L"User";
    lvI.mask      = LVIF_TEXT;
    lvI.iSubItem  = 1;

    ListView_SetItem( hwndListView, &lvI );
}

// Message handler for Access Rights dialog box
INT_PTR CALLBACK AccessRightsDialogProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    static AppConfig* appConfig = nullptr;
    static HICON      hIcon     = NULL;
    int               wmId;
    int               wmEvent;

    switch ( message )
    {
    case WM_INITDIALOG:
        {
            HWND hwndViewrsCombo        = GetDlgItem( hDlg, IDC_VIEWERS_COMBO );
            HWND hwndConfiguratorsCombo = GetDlgItem( hDlg, IDC_CONFIGURATORS_COMBO );
            HWND hwndUsersList          = GetDlgItem( hDlg, IDC_USERS_LIST_VIEW );

            appConfig = (AppConfig*) lParam;

            hIcon = (HICON) LoadImage( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ACCESS  ), IMAGE_ICON,
                                       GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );

            if ( hIcon )
            {
                SendMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );
            }


            CenterWindowTo( hDlg, GetParent( hDlg ) );
            InitUsersListView( hwndUsersList );

            // init combox boxes 
            SendMessage( hwndViewrsCombo, CB_ADDSTRING, 0, (LPARAM) STR_ANYONE );
            SendMessage( hwndViewrsCombo, CB_ADDSTRING, 0, (LPARAM) STR_USERS );
            SendMessage( hwndViewrsCombo, CB_ADDSTRING, 0, (LPARAM) STR_ADMINS );
            SendMessage( hwndConfiguratorsCombo, CB_ADDSTRING, 0, (LPARAM) STR_ANYONE );
            SendMessage( hwndConfiguratorsCombo, CB_ADDSTRING, 0, (LPARAM) STR_USERS );
            SendMessage( hwndConfiguratorsCombo, CB_ADDSTRING, 0, (LPARAM) STR_ADMINS );

            SendMessage( hwndViewrsCombo, CB_SETCURSEL, 0, 0 );
            SendMessage( hwndConfiguratorsCombo, CB_SETCURSEL, 1, 0 );

            int i1 = SendMessage( hwndConfiguratorsCombo, CB_GETCURSEL, 0, 0 );

            //
            AddUserToListView( hwndUsersList, L"Andrew", UserGroup::User );
            AddUserToListView( hwndUsersList, L"Bob", UserGroup::Admin );

            ListView_SetItemState( hwndUsersList, 0, LVIS_SELECTED, LVIS_SELECTED );

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
            if ( wmId == IDOK )
            {
                if ( appConfig )
                {

                }
            }

            EndDialog( hDlg, wmId );
            return (INT_PTR) TRUE;
        }
    }

    return (INT_PTR) FALSE;
}
