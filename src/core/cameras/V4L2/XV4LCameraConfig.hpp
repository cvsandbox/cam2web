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

#ifndef XV4L_CAMERA_CONFIG_HPP
#define XV4L_CAMERA_CONFIG_HPP

#include "IObjectConfigurator.hpp"
#include "XV4LCamera.hpp"

// The class is to get/set camera properties
class XV4LCameraConfig : public IObjectConfigurator
{
public:
    XV4LCameraConfig( const std::shared_ptr<XV4LCamera>& camera );

    XError SetProperty( const std::string& propertyName, const std::string& value );
    XError GetProperty( const std::string& propertyName, std::string& value ) const;

    std::map<std::string, std::string> GetAllProperties( ) const;

private:
    std::shared_ptr<XV4LCamera> mCamera;
};

// The class is to get/set camera properties information - min, max, default, etc.
class XV4LCameraPropsInfo : public IObjectInformation
{
public:
    XV4LCameraPropsInfo( const std::shared_ptr<XV4LCamera>& camera );

    XError GetProperty( const std::string& propertyName, std::string& value ) const;

    std::map<std::string, std::string> GetAllProperties( ) const;

private:
    std::shared_ptr<XV4LCamera> mCamera;
};

#endif // XV4L_CAMERA_CONFIG_HPP

