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

#ifndef XOBJECT_CONFIGURATION_REQUEST_HANDLER_HPP
#define XOBJECT_CONFIGURATION_REQUEST_HANDLER_HPP

#include "IObjectConfigurator.hpp"
#include "XWebServer.hpp"

// Web request handler to allow configuration of object's properties
class XObjectConfigurationRequestHandler : public IWebRequestHandler
{
public:
    XObjectConfigurationRequestHandler( const std::string& uri, const std::shared_ptr<IObjectConfigurator>& objectToConfig );

    void HandleHttpRequest( const IWebRequest& request, IWebResponse& response );

private:
    std::shared_ptr<IObjectConfigurator> ObjectToConfig;
};

// Web request handler to provide information about object's properties - read/only
class XObjectInformationRequestHandler : public IWebRequestHandler
{
public:
    XObjectInformationRequestHandler( const std::string& uri, const std::shared_ptr<IObjectInformation>& infoObject );

    void HandleHttpRequest( const IWebRequest& request, IWebResponse& response );

private:
    std::shared_ptr<IObjectInformation> InfoObject;
};

#endif // XOBJECT_CONFIGURATION_REQUEST_HANDLER_HPP
