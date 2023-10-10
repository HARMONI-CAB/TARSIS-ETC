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
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef _CONFIGMANAGER_H
#define _CONFIGMANAGER_H

#include <cstdio>
#include <string>
#include <list>
#include <map>
#include <yaml-cpp/yaml.h>

#define CONFIG_MANAGER_DIRECTORY "config"

class Config {
    std::string m_objectName;
    YAML::Node  m_yamlConfig;

  public:
    Config(std::string const &name);
    virtual ~Config();
    
    virtual bool serialize();
    virtual bool deserialize();

    bool load();
    bool save();
    bool hasKey(std::string const &) const;

    YAML::Node operator[](std::string const &);

    inline YAML::Node &
    yamlNode()
    {
      return m_yamlConfig;
    }

    bool
    deserializeYamlNode(YAML::Node &node)
    {
      m_yamlConfig = node;
      return deserialize();
    }

    template <typename T> inline bool
    deserializeField(T &dest, const char *name)
    {
      auto node = m_yamlConfig[name];
      if (!node.IsDefined())
        return true; // Okay: use default
      
      try {
        dest = node.as<T>();
      } catch (std::runtime_error const &e) {
        fprintf(
          stderr,
          "%s: cannot deserialize key `%s': %s\n",
          m_objectName.c_str(),
          name,
          e.what());
        return false;
      }

      return true;
    }
};

class ConfigManager {
    static ConfigManager *g_instance;

    std::string                      m_configDir;
    std::list<Config *>              m_configList;
    std::map<std::string, Config *>  m_configCache;
    bool                             m_canSaveConfig = false;

    ConfigManager();

  public:
    static ConfigManager *instance();
    bool saveAll();

    std::string getConfigFilePath(std::string const &name, bool write) const;

    template <class T> T *
    getConfig(std::string const &name)
    {
      auto it = m_configCache.find(name);
      T *retVal = nullptr;

      if (it == m_configCache.end()) {
        // Create new config!
        auto newConfig = new T(name);
        
        newConfig->load();

        m_configList.push_back(newConfig);
        m_configCache[name] = newConfig;

        retVal = newConfig;
      } else {
        retVal = static_cast<T *>(it->second);
      }

      return static_cast<T *>(retVal);
    }

    template <class T> static T &
    get(std::string const &name)
    {
      auto cfg = instance()->getConfig<T>(name);

      if (cfg == nullptr)
        throw std::runtime_error("Failed to retrieve config `" + name + "'");
      
      return *cfg;
    }
};

#endif // _CONFIGMANAGER_H
