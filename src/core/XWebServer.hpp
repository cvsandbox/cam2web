/*
    cam2web - streaming camera to web

    BSD 2-Clause License

    Copyright (c) 2017, cvsandbox, cvsandbox@gmail.com
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
    FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
    DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef XWEB_SERVER_HPP
#define XWEB_SERVER_HPP

#include <stdint.h>
#include <string>
#include <memory>
#include <map>

#include "XInterfaces.hpp"

namespace Private
{
    class XWebServerData;
}

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

    // Get/Set port
    uint16_t Port( ) const;
    XWebServer& SetPort( uint16_t port );

    // Add/Remove web handler
    XWebServer& AddHandler( const std::shared_ptr<IWebRequestHandler>& handler );
    void RemoveHandler( const std::shared_ptr<IWebRequestHandler>& handler );

    // ==============================================================

    // Start/Stop the Web server
    bool Start( );
    void Stop( );

private:
    Private::XWebServerData* mData;
};

#endif // XWEB_SERVER_HPP
