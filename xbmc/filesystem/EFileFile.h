#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "filesystem/OverrideFile.h"

namespace XFILE
{
class CEFileFile : public COverrideFile
{
public:
  CEFileFile(void);
  virtual ~CEFileFile(void);
  virtual bool Exists(const CURL& url);
  static bool ExistsOriginal(const CURL& url);
  static std::string GetOriginalPath(const CURL& url);
  static void GetMessages(const CURL& url, std::string& title, std::string& message);
  static std::string GetTranslatedPath(const CURL& url);

protected:
  static std::string CEFileFile::GetXMLString(const CURL& url, std::string strXMLTag);
  static bool CEFileFile::GetXMLString(const CURL& url, std::string strXMLTag, std::string& strValue);
  virtual std::string TranslatePath(const CURL& url);
};
}
