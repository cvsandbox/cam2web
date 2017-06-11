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
#include <vector>

#include "resource.h"
#include "SettingsDialog.hpp"
#include "EditUserDialog.hpp"
#include "Tools.hpp"
#include "UiTools.hpp"
#include "AppConfig.hpp"

#include "XWebServer.hpp"

using namespace std;

static const WCHAR* STR_ANYONE = L"Anyone";
static const WCHAR* STR_USERS  = L"Users";
static const WCHAR* STR_ADMINS = L"Admins";

#define USER_GROUP_TO_INDEX(group)  ( ( group == UserGroup::Admin ) ? 2 : ( ( group == UserGroup::User ) ? 1 : 0 ) )
#define INDEX_TO_USER_GROUP(index)  ( ( index == 2 ) ? UserGroup::Admin : ( ( index == 1 ) ? UserGroup::User : UserGroup::Anyone ) )

typedef map<string, pair<string, UserGroup>> UsersMap;

// Populate combo boxes with items and make initial selections
static void InitComboBoxes( HWND hwndViewersBox, HWND hwndConfiguratorsBox, UserGroup viewersGroup, UserGroup configuratorsGroup )
{
    SendMessage( hwndViewersBox, CB_ADDSTRING, 0, (LPARAM) STR_ANYONE );
    SendMessage( hwndViewersBox, CB_ADDSTRING, 0, (LPARAM) STR_USERS );
    SendMessage( hwndViewersBox, CB_ADDSTRING, 0, (LPARAM) STR_ADMINS );
    SendMessage( hwndConfiguratorsBox, CB_ADDSTRING, 0, (LPARAM) STR_ANYONE );
    SendMessage( hwndConfiguratorsBox, CB_ADDSTRING, 0, (LPARAM) STR_USERS );
    SendMessage( hwndConfiguratorsBox, CB_ADDSTRING, 0, (LPARAM) STR_ADMINS );

    SendMessage( hwndViewersBox, CB_SETCURSEL, USER_GROUP_TO_INDEX( viewersGroup ), 0 );
    SendMessage( hwndConfiguratorsBox, CB_SETCURSEL, USER_GROUP_TO_INDEX( configuratorsGroup ), 0 );
}

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

    // select full row when item is selected in the list view
    ListView_SetExtendedListViewStyle( hwndListView, LVS_EX_FULLROWSELECT );
}

// Add user to the users' list view
static void AddUserToListView( HWND hwndListView, const string& userName, UserGroup userGroup )
{
    wstring userNameUnicode = Utf8to16( userName );
    LVITEM  lvI;

    lvI.pszText   = (WCHAR*) userNameUnicode.c_str( );
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

    ListView_SetItemState( hwndListView, lvI.iItem, LVIS_SELECTED, LVIS_SELECTED );
}

// Message handler for Access Rights dialog box
INT_PTR CALLBACK AccessRightsDialogProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
    static AppConfig* appConfig = nullptr;
    static UsersMap*  pUsers    = nullptr;
    static HICON      hIcon     = NULL;
    int               wmId;
    int               wmEvent;

    UsersMap Users;

    switch ( message )
    {
    case WM_INITDIALOG:

        pUsers = new UsersMap( );

        CenterWindowTo( hDlg, GetParent( hDlg ) );

        hIcon = (HICON) LoadImage( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDI_ACCESS ), IMAGE_ICON,
                                   GetSystemMetrics( SM_CXSMICON ), GetSystemMetrics( SM_CYSMICON ), 0 );

        if ( hIcon )
        {
            SendMessage( hDlg, WM_SETICON, ICON_SMALL, (LPARAM) hIcon );
        }

        appConfig = (AppConfig*) lParam;

        if ( appConfig != nullptr )
        {
            HWND hwndUsersList = GetDlgItem( hDlg, IDC_USERS_LIST_VIEW );

            SetWindowText( GetDlgItem( hDlg, IDC_AUTH_DOMAIN_EDIT ), Utf8to16( appConfig->AuthDomain( ) ).c_str( ) );

            InitComboBoxes( GetDlgItem( hDlg, IDC_VIEWERS_COMBO ), GetDlgItem( hDlg, IDC_CONFIGURATORS_COMBO ),
                            static_cast<UserGroup>( appConfig->ViewersGroup( ) ),
                            static_cast<UserGroup>( appConfig->ConfiguratorsGroup( ) ) );
            InitUsersListView( hwndUsersList );
        }
        return (INT_PTR) TRUE;

    case WM_DESTROY:
        appConfig = nullptr;
        delete pUsers;

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
                    appConfig->SetViewersGroup( static_cast<uint16_t>(
                        INDEX_TO_USER_GROUP( SendMessage( GetDlgItem( hDlg, IDC_VIEWERS_COMBO ), CB_GETCURSEL, 0, 0 ) ) ) );
                    appConfig->SetConfiguratorsGroup( static_cast<uint16_t>(
                        INDEX_TO_USER_GROUP( SendMessage( GetDlgItem( hDlg, IDC_CONFIGURATORS_COMBO ), CB_GETCURSEL, 0, 0 ) ) ) );
                }
            }

            EndDialog( hDlg, wmId );
            return (INT_PTR) TRUE;
        }
        else if ( ( wmId == IDC_ADD_USER_BUTTON ) || ( wmId == IDC_EDIT_USER_BUTTON ) || ( wmId == IDC_DELETE_USER_BUTTON ) )
        {
            HWND           hwndUsersList = GetDlgItem( hDlg, IDC_USERS_LIST_VIEW );
            vector<string> existingUserNames;

            for ( UsersMap::const_iterator it = pUsers->begin( ); it != pUsers->end( ); ++it )
            {
                existingUserNames.push_back( it->first );
            }

            if ( wmId == IDC_ADD_USER_BUTTON )
            {
                UserInfo userInfo;

                userInfo.ExistingUserNames = existingUserNames;

                if ( DialogBoxParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_EDIT_USER_BOX ), hDlg, EditUserDialogProc, (LPARAM) &userInfo ) == IDOK )
                {
                    UserGroup userGroup = ( userInfo.IsAdmin ) ? UserGroup::Admin : UserGroup::User;

                    AddUserToListView( hwndUsersList, userInfo.Name, userGroup );
                    pUsers->insert( UsersMap::value_type( userInfo.Name, pair<string, UserGroup>( "yo", userGroup  ) ) );
                }
            }
            else if ( wmId == IDC_EDIT_USER_BUTTON )
            {
                UserInfo userInfo = { "Bob", "**********", true };

                if ( DialogBoxParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_EDIT_USER_BOX ), hDlg, EditUserDialogProc, (LPARAM) &userInfo ) == IDOK )
                {
                }
            }
            else if ( wmId == IDC_DELETE_USER_BUTTON )
            {

            }

            SetFocus( hwndUsersList );
        }
    }

    return (INT_PTR) FALSE;
}
