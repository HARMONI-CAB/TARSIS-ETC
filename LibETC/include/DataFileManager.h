//
// DataFileManager.h: Handle data files and defult paths
// Copyright (c) 2023 Gonzalo J. Carracedo <BatchDrake@gmail.com>
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef _DATAFILEMANAGER_H
#define _DATAFILEMANAGER_H

#include <string>
#include <list>
#include <stdexcept>

class DataFileManager {
    static DataFileManager *g_instance;
    std::list<std::string> m_paths;
    std::string find(std::string const &path, int flags);

    DataFileManager();

  public:
    static DataFileManager *instance();

    bool addSearchPath(std::string const &);
    
    std::string resolve(std::string const &);
    std::string suggest(std::string const &);
    std::list<std::string> const &searchPaths() const;
};

static inline std::string
dataFile(std::string const &path)
{
  auto result = DataFileManager::instance()->resolve(path);
  if (result.empty())
    throw std::runtime_error("Required datafile `" + path + "' not found");

  return result;
}

#endif // _DATAFILEMANAGER
