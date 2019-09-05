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

#include "XWebServer.hpp"
#include "XManualResetEvent.hpp"

#include <map>
#include <list>
#include <mutex>

#include <mongoose.h>

#ifdef WIN32
    #include <windows.h>
#endif

using namespace std;
using namespace std::chrono;

namespace Private
{
    #define DEFAULT_AUTH_DOMAIN "cam2web"

    /* ================================================================= */
    /* Web request implementation using Mangoose APIs                    */
    /* ================================================================= */
    class MangooseWebRequest : public IWebRequest
    {
        friend class XWebServerData;

    private:
        struct http_message* mMessage;

    private:
        MangooseWebRequest( struct http_message* message ) :
            mMessage( message )
        {
        }

    public:
        string Uri( ) const
        {
            return string( mMessage->uri.p, mMessage->uri.p + mMessage->uri.len );
        }
        string Method( ) const
        {
            return string( mMessage->method.p, mMessage->method.p + mMessage->method.len );
        }
        string Proto( ) const
        {
            return string( mMessage->proto.p, mMessage->proto.p + mMessage->proto.len );
        }
        string Query( ) const
        {
            return string( mMessage->query_string.p, mMessage->query_string.p + mMessage->query_string.len );
        }
        string Body( ) const
        {
            return string( mMessage->body.p, mMessage->body.p + mMessage->body.len );
        }
        string GetVariable( const string& name ) const
        {
            string         ret;
            char           value[200];
            struct mg_str* body = mMessage->query_string.len > 0 ? &mMessage->query_string : &mMessage->body;

            if ( body->len < sizeof( value ) )
            {
                mg_get_http_var( body, name.c_str( ), value, sizeof( value ) );
                ret = value;
            }
            else
            {
                char* temp = new char[body->len + 1];
                mg_get_http_var( body, name.c_str( ), temp, sizeof( body->len ) );
                ret = temp;
                delete [] temp;
            }

            return ret;
        }
        map<string, string> Headers( ) const
        {
            map<string, string> headers;

            for ( int i = 0; ( i < MG_MAX_HTTP_HEADERS ) && ( mMessage->header_names[i].len > 0 ); i++ )
            {
                headers.insert( pair<string, string>(
                        string( mMessage->header_names[i].p, mMessage->header_names[i].p + mMessage->header_names[i].len ),
                        string( mMessage->header_values[i].p, mMessage->header_values[i].p + mMessage->header_values[i].len )
                    ) );
            }

            return headers;
        }
    };

    /* ================================================================= */
    /* Web response implementation using Mangoose APIs                   */
    /* ================================================================= */
    class MangooseWebResponse : public IWebResponse
    {
        friend class XWebServerData;

    private:
        struct mg_connection* mConnection;
        IWebRequestHandler*   mHandler;

    private:
        MangooseWebResponse( struct mg_connection* connection, IWebRequestHandler* handler = nullptr ) :
            mConnection( connection ), mHandler( handler )
        {
        }

        // Set handler owning this response
        void SetHandler( IWebRequestHandler* handler )
        {
            mHandler = handler;
        }

    public:
        // Length of data, which is still enqueued for sending
        size_t ToSendDataLength( ) const 
        {
            return mConnection->send_mbuf.len;
        }

        // Send the specified buffer into response
        void Send( const uint8_t* buffer, size_t length )
        {
            mg_send( mConnection, buffer, static_cast<int>( length ) );
        }

        // Print formatted response
        void Printf( const char *fmt, ... )
        {
            va_list list;

            va_start( list, fmt );
            mg_vprintf( mConnection, fmt, list );
            va_end( list );
        }

        // Send the specified buffer as a chunk into response
        void SendChunk( const uint8_t* buffer, size_t length )
        {
            mg_send_http_chunk( mConnection, (const char*) buffer , length );
        }

        // Print formatted chunk into response
        void PrintfChunk( const char* fmt, ... )
        {
            char    mem[MG_VPRINTF_BUFFER_SIZE];
            char*   buf = mem;
            int     len;
            va_list list;

            va_start( list, fmt );
            len = mg_avprintf( &buf, sizeof( mem ), fmt, list );
            va_end( list );

            if ( len >= 0 )
            {
                mg_send_http_chunk( mConnection, buf, len );
            }

            if ( ( buf != mem ) && ( buf != nullptr ) )
            {
                free( buf );
            }
        }

        // Send the specified error code as response
        void SendError( int errorCode, const char* reason = nullptr )
        {
            mg_http_send_error( mConnection, errorCode, reason );
        }

        // Close connection associated with the response
        void CloseConnection( )
        {
            // with mongoose we can only signal connection should be closed
            mConnection->flags |= MG_F_CLOSE_IMMEDIATELY;
        }

        // Generate timer event for the connection associated with the response
        // after the specified number of milliseconds
        void SetTimer( uint32_t msec )
        {
            mConnection->user_data = mHandler;
            mg_set_timer( mConnection, mg_time( ) + (double) msec / 1000 );
        }
    };

    /* ================================================================= */
    /* Data associated with request handler                              */
    /* ================================================================= */
    class RequestHandlerData
    {
    public:
        shared_ptr<IWebRequestHandler>  Handler;
        UserGroup                       AllowedUserGroup;
        steady_clock::time_point        LastAccessTime;
        bool                            WasAccessed;
    public:
        RequestHandlerData( ) :
            Handler( ), AllowedUserGroup( UserGroup::Anyone ),
            LastAccessTime( ), WasAccessed( false )
        { }

        RequestHandlerData( const shared_ptr<IWebRequestHandler>& handler, UserGroup allowedUserGroup ) :
            Handler( handler), AllowedUserGroup( allowedUserGroup )
        { }
    };

    /* ================================================================= */
    /* Private data/implementation of the web server                     */
    /* ================================================================= */
    class XWebServerData
    {
    public:
        recursive_mutex           DataSync;
        string                    DocumentRoot;
        string                    AuthDomain;
        Authentication            AuthMethod;
        uint16_t                  Port;

        steady_clock::time_point  LastAccessTime;
        bool                      WasAccessed;

    private:
        struct mg_mgr             EventManager;
        struct mg_serve_http_opts ServerOptions;

        char*                     ActiveDocumentRoot;
        string                    ActiveAuthDomain;
        Authentication            ActiveAuthMethod;

        XManualResetEvent         NeedToStop;
        XManualResetEvent         IsStopped;
        recursive_mutex           StartSync;
        bool                      IsRunning;

        typedef map<string, RequestHandlerData> HandlersMap;
        typedef list<RequestHandlerData>        HandlersList;

        HandlersMap  FileHandlers;
        HandlersList FolderHandlers;

        HandlersMap  ActiveFileHandlers;
        HandlersList ActiveFolderHandlers;

        typedef map<string, pair<string, UserGroup>> UsersMap;

        UsersMap Users;

    public:
        XWebServerData( const string& documentRoot, uint16_t port ) :
            DataSync( ), DocumentRoot( documentRoot ), AuthDomain( DEFAULT_AUTH_DOMAIN ), AuthMethod( Authentication::Digest ), Port( port ),
            LastAccessTime( ), WasAccessed( false ),
            EventManager( { 0 } ), ServerOptions( { 0 } ),
            ActiveDocumentRoot( nullptr ), ActiveAuthDomain( ), ActiveAuthMethod( Authentication::Digest ),
            NeedToStop( ), IsStopped( ), StartSync( ), IsRunning( false )
        {
            ServerOptions.enable_directory_listing = "no";
        }

        ~XWebServerData( )
        {
            Stop( );
        }

        bool Start( );
        void Stop( );
        void Cleanup( );
        void AddHandler( const shared_ptr<IWebRequestHandler>& handler, UserGroup userGroup );
        void RemoveHandler( const shared_ptr<IWebRequestHandler>& handler );
        void ClearHandlers( );
        RequestHandlerData* FindHandler( const string& uri );
        steady_clock::time_point HandlerLastAccessTime( const string& handlerUri, bool* pWasAccessed = nullptr );

        void AddUser( const string& name, const string& digestHa1, UserGroup group );
        void RemoveUser( const string& name );
        uint32_t LoadUsersFromFile( const string& fileName );
        void ClearUsers( );

        void SendAuthenticationRequest( struct mg_connection* con );
        UserGroup CheckAuthentication( struct http_message* msg );

        static void* pollHandler( void* param );
        static void eventHandler( struct mg_connection* connection, int event, void* param );
    };
}

/* ================================================================= */
/* Implementation of the IWebRequestHandler                          */
/* ================================================================= */

IWebRequestHandler::IWebRequestHandler( const string& uri, bool canHandleSubContent ) :
    mUri( uri ), mCanHandleSubContent( canHandleSubContent )
{
    // make sure all URIs start with /
    if ( mUri[0] != '/' )
    {
        mUri = '/' + mUri;
    }

    // make sure nothing finishes with /
    while ( ( mUri.length( ) > 1 ) && ( mUri.back( ) == '/' ) )
    {
        mUri.pop_back( );
    }
}

/* ================================================================= */
/* Implementation of the XEmbeddedContentHandler                     */
/* ================================================================= */

XEmbeddedContentHandler::XEmbeddedContentHandler( const string& uri, const XEmbeddedContent* content ) :
    IWebRequestHandler( uri, false ), mContent( content )
{
}

// Handle request providing given embedded content
void XEmbeddedContentHandler::HandleHttpRequest( const IWebRequest& /* request */, IWebResponse& response )
{
    response.Printf( "HTTP/1.1 200 OK\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %u\r\n"
                     "\r\n", mContent->Type, mContent->Length );
    response.Send( mContent->Body, mContent->Length );
}

/* ================================================================= */
/* Implementation of the XWebServer                                  */
/* ================================================================= */

XWebServer::XWebServer( const string& documentRoot, uint16_t port ) :
    mData( new Private::XWebServerData( documentRoot, port ) )
{

}

XWebServer::~XWebServer( )
{
    delete mData;
}

// Get/Set document root
string XWebServer::DocumentRoot( ) const
{
    return mData->DocumentRoot;
}
XWebServer& XWebServer::SetDocumentRoot( const string& documentRoot )
{
    lock_guard<recursive_mutex> lock( mData->DataSync );
    mData->DocumentRoot = documentRoot;
    return *this;
}

// Get/Set digest authentication domain
std::string XWebServer::AuthDomain( ) const
{
    return mData->AuthDomain;
}
XWebServer& XWebServer::SetAuthDomain( const std::string& authDomain )
{
    lock_guard<recursive_mutex> lock( mData->DataSync );
    mData->AuthDomain = authDomain;
    return *this;
}

// Get/Set authentication method (default Digest)
Authentication XWebServer::AuthenticationMethod( ) const
{
    return mData->AuthMethod;
}
XWebServer& XWebServer::SetAuthenticationMethod( Authentication authMethod )
{
    lock_guard<recursive_mutex> lock( mData->DataSync );
    mData->AuthMethod = authMethod;
    return *this;
}

// We really love Windows.h, indeed
#pragma push_macro( "SetPort" )
#undef SetPort

// Get/Set port
uint16_t XWebServer::Port( ) const
{
    return mData->Port;
}
XWebServer& XWebServer::SetPort( uint16_t port )
{
    lock_guard<recursive_mutex> lock( mData->DataSync );
    mData->Port = port;
    return *this;
}

#pragma pop_macro( "SetPort" )

// Start/Stop the Web server
bool XWebServer::Start( )
{
    return mData->Start( );
}
void XWebServer::Stop( )
{
    mData->Stop( );
}

// Get time of the last access/request to the web server
steady_clock::time_point XWebServer::LastAccessTime( bool* pWasAccessed )
{
    if ( pWasAccessed )
    {
        *pWasAccessed = mData->WasAccessed;
    }
    return mData->LastAccessTime;
}

// Get time of the last access/request to the specified handler
steady_clock::time_point XWebServer::LastAccessTime( const string& handlerUri, bool* pWasAccessed )
{
    return mData->HandlerLastAccessTime( handlerUri, pWasAccessed );
}

// Add new web request handler
XWebServer& XWebServer::AddHandler( const shared_ptr<IWebRequestHandler>& handler, UserGroup allowedUserGroup )
{
    mData->AddHandler( handler, allowedUserGroup );
    return *this;
}

// Remove the specified handler
void XWebServer::RemoveHandler( const std::shared_ptr<IWebRequestHandler>& handler )
{
    mData->RemoveHandler( handler );
}

// Remove all handlers
void XWebServer::ClearHandlers( )
{
    mData->ClearHandlers( );
}

// Add/Remove user to access protected request handlers
XWebServer& XWebServer::AddUser( const string& name, const string& digestHa1, UserGroup group )
{
    mData->AddUser( name, digestHa1, group );
    return *this;
}
void XWebServer::RemoveUser( const string& name )
{
    mData->RemoveUser( name );
}

// Load users from file having "htdigest" format
uint32_t XWebServer::LoadUsersFromFile( const string& fileName )
{
    return mData->LoadUsersFromFile( fileName );
}

// Clear the list of users who can access the web server
void XWebServer::ClearUsers( )
{
    return mData->ClearUsers( );
}

// Calculate HA1 as defined by Digest authentication algorithm, MD5(user:domain:pass).
string XWebServer::CalculateDigestAuthHa1( const string& user, const string& domain, const string& pass )
{
    char ha1[33];

    cs_md5( ha1, user.c_str( ), user.length( ), ":", static_cast<size_t>( 1 ),
                 domain.c_str( ), domain.length( ), ":", static_cast<size_t>( 1 ),
                 pass.c_str( ), pass.length( ), nullptr );

    return string( ha1 );
}

/* ================================================================= */
/* Private implemenetation of the XWebServer                         */
/* ================================================================= */

namespace Private
{

// Start instance of a Web server
bool XWebServerData::Start( )
{
    lock_guard<recursive_mutex> lock( StartSync );
    char strPort[16];

    {
        lock_guard<recursive_mutex> dataLock( DataSync );

        sprintf( strPort, "%u", Port );

        // set document root
        if ( !DocumentRoot.empty( ) )
        {
            ActiveDocumentRoot = new char[DocumentRoot.length( ) + 1];
            strcpy( ActiveDocumentRoot, DocumentRoot.c_str( ) );

            ServerOptions.document_root = ActiveDocumentRoot;
        }

        // get a copy of handlers, so we don't need to guard it while server is running
        ActiveFileHandlers   = FileHandlers;
        ActiveFolderHandlers = FolderHandlers;
        ActiveAuthDomain     = AuthDomain;
        ActiveAuthMethod     = AuthMethod;
    }

    mg_mgr_init( &EventManager, this );

    NeedToStop.Reset( );
    IsStopped.Reset( );

    WasAccessed    = false;
    LastAccessTime = steady_clock::time_point( );

    struct mg_connection* connection = mg_bind( &EventManager, strPort, eventHandler );

    if ( connection != nullptr )
    {
        mg_set_protocol_http_websocket( connection );

        if ( mg_start_thread( pollHandler, this ) != nullptr )
        {
            IsRunning = true;
        }
    }

    if ( !IsRunning )
    {
        Cleanup( );
    }

    return IsRunning;
}

// Stop running Web server
void XWebServerData::Stop( )
{
    lock_guard<recursive_mutex> lock( StartSync );

    if ( IsRunning )
    {
        NeedToStop.Signal( );
        IsStopped.Wait( );

        Cleanup( );

        IsRunning = false;
    }
}

// Clean-up resources
void XWebServerData::Cleanup( )
{
    mg_mgr_free( &EventManager );

    if ( ActiveDocumentRoot != nullptr )
    {
        delete[] ActiveDocumentRoot;
    }
}

// Add web server request handler
void XWebServerData::AddHandler( const shared_ptr<IWebRequestHandler>& handler, UserGroup allowedUserGroup )
{
    lock_guard<recursive_mutex> lock( DataSync );

    if ( handler->CanHandleSubContent( ) )
    {
        FolderHandlers.push_back( RequestHandlerData( handler, allowedUserGroup ) );
    }
    else
    {
        FileHandlers.insert( pair<string, RequestHandlerData>( handler->Uri( ), RequestHandlerData( handler, allowedUserGroup ) ) );
    }
}

// Remove the specified request handler
void XWebServerData::RemoveHandler( const shared_ptr<IWebRequestHandler>& handler )
{
    lock_guard<recursive_mutex> lock( DataSync );

    FileHandlers.erase( handler->Uri( ) );

    for ( HandlersList::iterator itHandler = FolderHandlers.begin( ); itHandler != FolderHandlers.end( ); itHandler++ )
    {
        if ( itHandler->Handler == handler )
        {
            FolderHandlers.erase( itHandler );
            break;
        }
    }
}

// Remove all handlers
void XWebServerData::ClearHandlers( )
{
    lock_guard<recursive_mutex> lock( DataSync );

    FileHandlers.clear( );
    FolderHandlers.clear( );
}

// Find request handler for the specified URI
RequestHandlerData* XWebServerData::FindHandler( const string& uri )
{
    RequestHandlerData*   handler = nullptr;
    HandlersMap::iterator fileIt  = ActiveFileHandlers.find( uri );

    if ( fileIt != ActiveFileHandlers.end( ) )
    {
        handler = &fileIt->second;
    }
    else
    {
        for ( auto& folderHandlerData : ActiveFolderHandlers )
        {
            if ( uri.compare( 0, folderHandlerData.Handler->Uri( ).length( ), folderHandlerData.Handler->Uri( ) ) == 0 )
            {
                handler = &folderHandlerData;
                break;
            }
        }
    }

    return handler;
}

// Get time of the last access/request to the specified handler
steady_clock::time_point XWebServerData::HandlerLastAccessTime( const string& handlerUri, bool* pWasAccessed )
{
    RequestHandlerData*      handlerData = FindHandler( handlerUri );
    steady_clock::time_point lastAccess;
    bool                     wasAccessed = false;

    if ( handlerData != nullptr )
    {
        lastAccess  = handlerData->LastAccessTime;
        wasAccessed = handlerData->WasAccessed;
    }

    if ( pWasAccessed != nullptr )
    {
        *pWasAccessed = wasAccessed;
    }

    return lastAccess;
}

// Add/Remove user to/from the list of user who can access protected request handlers
void XWebServerData::AddUser( const string& name, const string& digestHa1, UserGroup group )
{
    lock_guard<recursive_mutex> lock( DataSync );

    Users[name] = pair<string, UserGroup>( digestHa1, group );
}
void XWebServerData::RemoveUser( const string& name )
{
    lock_guard<recursive_mutex> lock( DataSync );

    Users.erase( name );
}

// Load users from file having "htdigest" format
uint32_t XWebServerData::LoadUsersFromFile( const string& fileName )
{
    uint32_t userCounter = 0;
    FILE*    file        = nullptr;

#ifdef WIN32
    {
        int charsRequired = MultiByteToWideChar( CP_UTF8, 0, fileName.c_str( ), -1, NULL, 0 );

        if ( charsRequired > 0 )
        {
            WCHAR* filenameUtf16 = (WCHAR*) malloc( sizeof( WCHAR ) * charsRequired );

            if ( MultiByteToWideChar( CP_UTF8, 0, fileName.c_str( ), -1, filenameUtf16, charsRequired ) > 0 )
            {
                file = _wfopen( filenameUtf16, L"r" );
            }

            free( filenameUtf16 );
        }
    }
#else
    file = fopen( fileName.c_str( ), "r" );
#endif

    if ( file )
    {
        char buff[256];

        // htdigest tool does not escape ':' found in user name or auth domain, so there is
        // no way to resolve what is what in cases like this:
        // test::cam2web::6568681ab858b3cce0f75bba97c9db8e
        // so we'll ignore anything like this

        while ( fgets( buff, 256, file ) != nullptr )
        {
            string    userName;
            string    userDomain;
            string    digestHa1;
            UserGroup group     = UserGroup::Anyone;
            char*     realmPtr  = strchr( buff, ':' );
            char*     digestPtr = nullptr;

            if ( realmPtr != nullptr )
            {
                userName  = string( buff, realmPtr );
                digestPtr = strchr( ++realmPtr, ':' );

                if ( digestPtr != nullptr )
                {
                    userDomain = string( realmPtr, digestPtr );

                    // copy the rest as digest HA1
                    digestHa1 = string( ++digestPtr );
                    // remove trailing spaces
                    while ( ( digestHa1.back( ) == ' '  ) || ( digestHa1.back( ) == '\t' ) ||
                            ( digestHa1.back( ) == '\n' ) || ( digestHa1.back( ) == '\r' ) )
                    {
                        digestHa1.pop_back( );
                    }

                    // original htdigest format has only user_name:domain:ha1, but we'll handle slightly modified
                    // version, where an extra value may come - user group id
                    size_t delIndex = digestHa1.find( ':' );

                    if ( delIndex != string::npos )
                    {
                        int groupId;

                        if ( sscanf( digestHa1.c_str( ) + delIndex + 1, "%d", &groupId ) == 1 )
                        {
                            if ( ( groupId > 0 ) && ( groupId <= 3 ) )
                            {
                                group = static_cast<UserGroup>( groupId );
                            }
                        }

                        digestHa1 = digestHa1.substr( 0, delIndex );
                    }

                }
            }

            // make sure digest is 32 character long
            if ( digestHa1.length( ) == 32 )
            {
                if ( userDomain == AuthDomain )
                {
                    AddUser( userName, digestHa1, ( group != UserGroup::Anyone ) ? group : ( ( userName == "admin" ) ? UserGroup::Admin : UserGroup::User ) );
                    userCounter++;
                }
            }
        }

        fclose( file );
    }

    return userCounter;
}

// Discard all users
void XWebServerData::ClearUsers( )
{
    lock_guard<recursive_mutex> lock( DataSync );

    Users.clear( );
}

// Thread to poll web events
void* XWebServerData::pollHandler( void* param )
{
    XWebServerData* self = (XWebServerData*) param;

    while ( !self->NeedToStop.Wait( 0 ) )
    {
        mg_mgr_poll( &self->EventManager, 1000 );
    }

    self->IsStopped.Signal( );

    return nullptr;
}

// Send HTTP authentication request (basic or digest)
void XWebServerData::SendAuthenticationRequest( struct mg_connection* con )
{
    if ( ActiveAuthMethod == Authentication::Basic )
    {
        mg_printf( con,
            "HTTP/1.1 401 Unauthorized\r\n"
            "WWW-Authenticate: Basic realm=\"%s\"\r\n"
            "Content-Length: 0\r\n\r\n",
            ActiveAuthDomain.c_str( ) );
    }
    else
    {
        mg_printf( con,
            "HTTP/1.1 401 Unauthorized\r\n"
            "WWW-Authenticate: Digest qop=\"auth\", "
            "realm=\"%s\", nonce=\"%lx\"\r\n"
            "Content-Length: 0\r\n\r\n",
            ActiveAuthDomain.c_str( ), ( unsigned long ) mg_time( ) );
    }
}

static bool check_nonce( const char* nonce )
{
    unsigned long now = (unsigned long) mg_time( );
    unsigned long val = (unsigned long) strtoul( nonce, NULL, 16 );
    return ( now >= val ) && ( now - val < 3600 );
}

// Check authentication (basic/digest) and resolve group of the incoming user
UserGroup XWebServerData::CheckAuthentication( struct http_message* msg )
{
    UserGroup       userGroup = UserGroup::Anyone;
    struct mg_str*  hdr;

    if ( ( msg != nullptr ) && ( ( hdr = mg_get_http_header( msg, "Authorization" ) ) != nullptr ) )
    {
        if ( mg_strncmp( *hdr, mg_mk_str( "Basic " ), 6 ) == 0 )
        {
            // HTTP Basic authentication
            char* buffer = static_cast<char*>( calloc( hdr->len, 1 ) );

            if ( buffer )
            {
                string user, password;
                char*  ptrDel = nullptr;

                cs_base64_decode( (unsigned char*) hdr->p + 6, static_cast<int>( hdr->len ), buffer, NULL );
                ptrDel = strchr( buffer, ':' );

                if ( ptrDel )
                {
                    user     = string( buffer, ptrDel - buffer );
                    password = string( ptrDel + 1 );
                }

                free( buffer );

                if ( ( !user.empty( ) ) && ( !password.empty( ) ) )
                {
                    string digestHa1 = XWebServer::CalculateDigestAuthHa1( user, AuthDomain, password );
                    lock_guard<recursive_mutex> lock( DataSync );

                    // first find the user
                    UsersMap::const_iterator itUser = Users.find( user );

                    if ( itUser != Users.end( ) )
                    {
                        if ( itUser->second.first == digestHa1 )
                        {
                            userGroup = itUser->second.second;
                        }
                    }
                }
            }
        }
        else if ( mg_strncmp( *hdr, mg_mk_str( "Digest " ), 6 ) == 0 )
        {
            // HTTP Digest authentication
            char user[50], cnonce[45], response[40], uri[200], qop[20], nc[20], nonce[30];
            char expectedResponse1[33];
            char expectedResponse2[33];

            if ( ( mg_http_parse_header( hdr, "username", user, sizeof( user ) ) != 0 ) &&
                 ( mg_http_parse_header( hdr, "cnonce", cnonce, sizeof( cnonce ) ) != 0 ) &&
                 ( mg_http_parse_header( hdr, "response", response, sizeof( response ) ) != 0 ) &&
                 ( mg_http_parse_header( hdr, "uri", uri, sizeof( uri ) ) != 0 ) &&
                 ( mg_http_parse_header( hdr, "qop", qop, sizeof( qop ) ) != 0 ) &&
                 ( mg_http_parse_header( hdr, "nc", nc, sizeof( nc ) ) != 0 ) &&
                 ( mg_http_parse_header( hdr, "nonce", nonce, sizeof( nonce ) ) != 0 ) )
            {
                // got some authentication data to check
                if ( check_nonce( nonce ) )
                {
                    lock_guard<recursive_mutex> lock( DataSync );

                    // first find the user
                    UsersMap::const_iterator itUser = Users.find( user );

                    if ( itUser != Users.end( ) )
                    {
                        char ha2[33];

                        // HA2 = MD5( method:digestURI )
                        cs_md5( ha2, msg->method.p, static_cast<size_t>( msg->method.len ),
                                     ":", static_cast<size_t>( 1 ),
                                     msg->uri.p, static_cast<size_t>( msg->uri.len + ( msg->query_string.len ? msg->query_string.len + 1 : 0 ) ),
                                     nullptr );

                        // response = MD5( HA1:nonce:nonceCount:cnonce:qop:HA2 )
                        cs_md5( expectedResponse1, 
                                itUser->second.first.c_str( ), itUser->second.first.length( ), // HA1 of the user
                                ":", static_cast<size_t>( 1 ),
                                nonce, strlen( nonce ),
                                ":", static_cast<size_t>( 1 ),
                                nc, strlen( nc ),
                                ":", static_cast<size_t>( 1 ),
                                cnonce, strlen( cnonce ),
                                ":", static_cast<size_t>( 1 ),
                                qop, strlen( qop ),
                                ":", static_cast<size_t>( 1 ),
                                ha2, static_cast<size_t>( 32 ),
                                nullptr );

                        if ( msg->query_string.len != 0 )
                        {
                            // Found some clients (like .NET's HttpWebRequest), which calculate HA2 using URI without query part.
                            // So need to calculate both variants, to make all clients happy.

                            cs_md5( ha2, msg->method.p, static_cast<size_t>( msg->method.len ),
                                    ":", static_cast<size_t>( 1 ),
                                    msg->uri.p, static_cast<size_t>( msg->uri.len ),
                                    nullptr );

                            cs_md5( expectedResponse2,
                                    itUser->second.first.c_str( ), itUser->second.first.length( ), // HA1 of the user
                                    ":", static_cast<size_t>( 1 ),
                                    nonce, strlen( nonce ),
                                    ":", static_cast<size_t>( 1 ),
                                    nc, strlen( nc ),
                                    ":", static_cast<size_t>( 1 ),
                                    cnonce, strlen( cnonce ),
                                    ":", static_cast<size_t>( 1 ),
                                    qop, strlen( qop ),
                                    ":", static_cast<size_t>( 1 ),
                                    ha2, static_cast<size_t>( 32 ),
                                    nullptr );
                        }

                        if ( ( strcmp( response, expectedResponse1 ) == 0 ) ||
                             ( ( msg->query_string.len != 0 ) && ( strcmp( response, expectedResponse2 ) == 0 ) ) )
                        {
                            userGroup = itUser->second.second;
                        }
                    }
                }
            }
        }
    }

    return userGroup;
}

// Mangoose web server event handler
void XWebServerData::eventHandler( struct mg_connection* connection, int event, void* param )
{
    XWebServerData* self = (XWebServerData*) connection->mgr->user_data;

    static bool isAuth = false;

    if ( event == MG_EV_HTTP_REQUEST )
    {
        struct http_message* message = static_cast<struct http_message*>( param );
        MangooseWebRequest   request( message );
        MangooseWebResponse  response( connection );
        string               uri = request.Uri( );
        UserGroup            authUserGroup = self->CheckAuthentication( message );

        // make sure nothing finishes with / except the root
        while ( ( uri.back( ) == '/' ) && ( uri.length( ) != 1 ) )
        {
            uri.pop_back( );
        }

        // try finding handler for the URI
        RequestHandlerData* handlerData = self->FindHandler( uri );

        if ( handlerData != nullptr )
        {
            if ( static_cast<int>( authUserGroup ) < static_cast<int>( handlerData->AllowedUserGroup ) )
            {
                self->SendAuthenticationRequest( connection );
            }
            else
            {
                response.SetHandler( handlerData->Handler.get( ) );
                // handle request with the found handler
                handlerData->Handler->HandleHttpRequest( request, response );

                handlerData->WasAccessed    = true;
                handlerData->LastAccessTime = steady_clock::now( );
            }
        }
        else if ( self->ActiveDocumentRoot )
        {
            // use static content
            mg_serve_http( connection, message, self->ServerOptions );
        }
        else
        {
            // send 404 error - not found
            response.SendError( 404 );
        }
    }
    else if ( event == MG_EV_TIMER )
    {
        if ( connection->user_data != nullptr )
        {
            IWebRequestHandler* handler = static_cast<IWebRequestHandler*>( connection->user_data );
            MangooseWebResponse response( connection, handler );

            connection->user_data = nullptr;

            handler->HandleTimer( response );
        }
    }

    if ( ( event != MG_EV_POLL ) && ( event != MG_EV_CLOSE ) )
    {
        self->WasAccessed    = true;
        self->LastAccessTime = steady_clock::now( );
    }
}

} // namespace Private
