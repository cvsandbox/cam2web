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

#ifndef XOBJECT_CONFIGURATOR_REQUEST_HANDLER_HPP
#define XOBJECT_CONFIGURATOR_REQUEST_HANDLER_HPP

#include "IObjectConfigurator.hpp"
#include "XWebServer.hpp"

class XObjectConfiguratorRequestHandler : public IWebRequestHandler
{
public:
    XObjectConfiguratorRequestHandler( const std::string& uri, const std::shared_ptr<IObjectConfigurator>& objectToConfig );

    void HandleHttpRequest( const IWebRequest& request, IWebResponse& response );

private:
    void HandleGet( const std::string& varsToGet, IWebResponse& response );
    void HandlePost( const std::string& body, IWebResponse& response );

private:
    std::shared_ptr<IObjectConfigurator> ObjectToConfig;
};

#endif // XOBJECT_CONFIGURATOR_REQUEST_HANDLER_HPP
