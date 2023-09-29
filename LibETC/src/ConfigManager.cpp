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

#include <DataFileManager.h>
#include <ConfigManager.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <cstdio>

ConfigManager *ConfigManager::g_instance = nullptr;

Config::Config(std::string const &name)
{
  if (name.empty())
    throw std::runtime_error("Empty config names are not allowed\n");

  m_objectName = name;

  auto p = m_yamlConfig["a"];
}

Config::~Config()
{

}

bool
Config::serialize()
{
  return true;
}

bool
Config::deserialize()
{
  return true;
}

YAML::Node
Config::operator[](std::string const &key)
{
  return m_yamlConfig[key];
}

bool
Config::load()
{
  std::string path = ConfigManager::instance()->getConfigFilePath(m_objectName);

  // File exists!
  if (access(path.c_str(), R_OK) != -1) {
    try {
      m_yamlConfig = YAML::LoadFile(path);
    } catch (std::runtime_error const &e) {
      fprintf(
        stderr,
        "%s: cannot load file config file from `%s': %s. Using defaults.\n",
        m_objectName.c_str(),
        path.c_str(),
        e.what());
    }
  }

  return deserialize();
}

bool
Config::hasKey(std::string const &name) const
{
  return m_yamlConfig[name].IsDefined();
}

bool
Config::save()
{
  std::string path = ConfigManager::instance()->getConfigFilePath(m_objectName);

  if (!serialize())
    return false;

  YAML::Emitter out;
  out << m_yamlConfig;

  FILE *fp = fopen(path.c_str(), "wb");
  if (fp == nullptr) {
    fprintf(
        stderr,
        "%s: cannot save config to `%s': %s.\n",
        m_objectName.c_str(),
        path.c_str(),
        strerror(errno));

    return false;
  }

  fwrite(out.c_str(), out.size(), 1, fp);
  fclose(fp);

  return true;
}

ConfigManager::ConfigManager()
{
  struct stat sbuf;

  m_configDir = DataFileManager::instance()->suggest("config");
  if (m_configDir.empty())
    throw std::runtime_error("No config directory location available");

  if (stat(m_configDir.c_str(), &sbuf) == -1) {
    if (errno == ENOENT) {
      if (mkdir(m_configDir.c_str(), 0700) == -1)
        throw std::runtime_error("Failed to create config directory");
    } else {
      throw std::runtime_error(
        "Config directory `" + m_configDir + "' inaccessible: " + strerror(errno));
    }
  } else if (!S_ISDIR(sbuf.st_mode)) {
    throw std::runtime_error(
      "Config directory `" + m_configDir + "' is not a directory");
  }
}

bool
ConfigManager::saveAll()
{
  bool ok = true;

  for (auto p : m_configList)
    ok = p->save() && ok;

  return ok;
}

std::string
ConfigManager::getConfigFilePath(std::string const &name) const
{
  return m_configDir + "/" + name + ".yaml";
}

ConfigManager *
ConfigManager::instance()
{
  if (g_instance == nullptr)
    g_instance = new ConfigManager();
  
  return g_instance;
}