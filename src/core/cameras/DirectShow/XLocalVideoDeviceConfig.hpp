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

#ifndef XLOCAL_VIDEO_DEVICE_CONFIG_HPP
#define XLOCAL_VIDEO_DEVICE_CONFIG_HPP

#include "IObjectConfigurator.hpp"
#include "XLocalVideoDevice.hpp"

class XLocalVideoDeviceConfig : public IObjectConfigurator
{
public:
    XLocalVideoDeviceConfig( const std::shared_ptr<XLocalVideoDevice>& camera,
                             const XDeviceName& deviceName, 
                             const XDeviceCapabilities& deviceCapabilities = XDeviceCapabilities( ) );

    XError SetProperty( const std::string& propertyName, const std::string& value );
    XError GetProperty( const std::string& propertyName, std::string& value ) const;

    std::map<std::string, std::string> GetAllProperties( ) const;

private:
    std::shared_ptr<XLocalVideoDevice> mCamera;
    XDeviceName                        mDeviceName;
    XDeviceCapabilities                mDeviceCapabilities;
};

#endif // XLOCAL_VIDEO_DEVICE_CONFIG_HPP
