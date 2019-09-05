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

#ifndef XWEB_SERVER_HPP
#define XWEB_SERVER_HPP

#include <stdint.h>
#include <string>
#include <memory>
#include <map>
#include <chrono>

#include "XInterfaces.hpp"

namespace Private
{
    class XWebServerData;
}

enum class UserGroup
{
    Anyone = 0,
    User   = 1,
    Power  = 2,
    Admin  = 3
};

enum class Authentication
{
    Basic  = 0,
    Digest = 1
};

/* ================================================================= */
/* Web request data                                                  */
/* ================================================================= */
class IWebRequest
{
public:
    virtual ~IWebRequest( ) { }

    virtual std::string Uri( )    const = 0;
    virtual std::string Method( ) const = 0;
    virtual std::string Proto( )  const = 0;
    virtual std::string Query( )  const = 0;
    virtual std::string Body( )   const = 0;

    virtual std::string GetVariable( const std::string& name ) const = 0;

    virtual std::map<std::string, std::string> Headers( ) const = 0;
};

/* ================================================================= */
/* Web response methods                                              */
/* ================================================================= */
class IWebResponse
{
public:
    virtual ~IWebResponse( ) { }

    // Length of data, which is still enqueued for sending
    virtual size_t ToSendDataLength( ) const = 0;

    virtual void Send( const uint8_t* buffer, size_t length ) = 0;
    virtual void Printf( const char* fmt, ... ) = 0;

    virtual void SendChunk( const uint8_t* buffer, size_t length ) = 0;
    virtual void PrintfChunk( const char* fmt, ... ) = 0;

    virtual void SendError( int errorCode, const char* reason = nullptr ) = 0;

    virtual void CloseConnection( ) = 0;

    // Generate timer event for the connection associated with the response
    // after the specified number of milliseconds
    virtual void SetTimer( uint32_t msec ) = 0;
};

/* ================================================================= */
/* Handler of a web request                                          */
/* ================================================================= */
class IWebRequestHandler
{
protected:
    IWebRequestHandler( const std::string& uri, bool canHandleSubContent );

public:
    virtual ~IWebRequestHandler( ) { }

    const std::string& Uri( ) const { return mUri;  }
    bool CanHandleSubContent( ) const { return mCanHandleSubContent; }

    // Handle the specified request
    virtual void HandleHttpRequest( const IWebRequest& request, IWebResponse& response ) = 0;

    // Handle timer event
    virtual void HandleTimer( IWebResponse& ) { };

private:
    std::string mUri;
    bool        mCanHandleSubContent;
};

/* ================================================================= */
/* Definition of embedded content                                    */
/* ================================================================= */
typedef struct
{
    uint32_t       Length;
    const char*    Type;
    const uint8_t* Body;
}
XEmbeddedContent;

/* ================================================================= */
/* Web request handler serving embedded content                      */
/* ================================================================= */
class XEmbeddedContentHandler : public IWebRequestHandler
{
public:
    XEmbeddedContentHandler( const std::string& uri, const XEmbeddedContent* content );

    // Handle request providing given embedded content
    void HandleHttpRequest( const IWebRequest& request, IWebResponse& response );

private:
    const XEmbeddedContent* mContent;
};

/* ================================================================= */
/* Web server class                                                  */
/* ================================================================= */
class XWebServer : private Uncopyable
{
public:
    XWebServer( const std::string& documentRoot = "", uint16_t port = 8000 );
    ~XWebServer( );

    /* ================================================================= */
    /* All of the below configuration must be done before starting       */
    /* an instance of a Web server. Setting those while it is running    */
    /* has no effect until it is restarted.                              */
    /* ================================================================= */

    // Get/Set document root
    std::string DocumentRoot( ) const;
    XWebServer& SetDocumentRoot( const std::string& documentRoot );

    // Get/Set authentication domain
    std::string AuthDomain( ) const;
    XWebServer& SetAuthDomain( const std::string& authDomain );

    // Get/Set authentication method (default Digest)
    Authentication AuthenticationMethod( ) const;
    XWebServer& SetAuthenticationMethod( Authentication authMethod );

    // Get/Set port
    uint16_t Port( ) const;
    XWebServer& SetPort( uint16_t port );

    // Add/Remove web handler
    XWebServer& AddHandler( const std::shared_ptr<IWebRequestHandler>& handler, UserGroup allowedUserGroup = UserGroup::Anyone );
    void RemoveHandler( const std::shared_ptr<IWebRequestHandler>& handler );

    // Remove all handlers
    void ClearHandlers( );

    // ==============================================================

    // Start/Stop the Web server
    bool Start( );
    void Stop( );

    // Get time of the last access/request to the web server
    std::chrono::steady_clock::time_point LastAccessTime( bool* pWasAccessed = nullptr );
    // Get time of the last access/request to the specified handler
    std::chrono::steady_clock::time_point LastAccessTime( const std::string& handlerUri, bool* pWasAccessed = nullptr );

    // Add/Remove user to access protected request handlers
    XWebServer& AddUser( const std::string& name, const std::string& digestHa1, UserGroup group );
    void RemoveUser( const std::string& name );

    // Load users from file having "htdigest" format (returns number of loaded users)
    uint32_t LoadUsersFromFile( const std::string& fileName );

    // Clear the list of users who can access the web server
    void ClearUsers( );

public:
    // Calculate HA1 as defined by Digest authentication algorithm, MD5(user:domain:pass).
    static std::string CalculateDigestAuthHa1( const std::string& user, const std::string& domain, const std::string& pass );

private:
    Private::XWebServerData* mData;
};

#endif // XWEB_SERVER_HPP
