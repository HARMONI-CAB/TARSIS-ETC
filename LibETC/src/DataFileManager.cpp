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

#include <DataFileManager.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <stdexcept>
#include <filesystem>

DataFileManager *DataFileManager::g_instance = nullptr;

#if defined(__APPLE__)
#  include <CoreFoundation/CoreFoundation.h>

static const char *
get_resource_bundle_path(CFStringRef relpath)
{
  CFBundleRef main_bundle = NULL;
  CFURLRef dir_url = NULL;
  CFStringRef dir_path = NULL;
  CFStringEncoding encmethod;
  const char *path = NULL;

  if ((main_bundle = CFBundleGetMainBundle()) != NULL) {
    dir_url = CFBundleCopyResourceURL(
                  main_bundle,
                  relpath,
                  NULL, /* resourceType */
                  NULL /* dirName */);

    /* Not an error */
    if (dir_url == NULL)
      goto done;

    dir_path = CFURLCopyFileSystemPath(dir_url, kCFURLPOSIXPathStyle);
    if (dir_path == nullptr)
      goto done;

    encmethod = CFStringGetSystemEncoding();
    path = CFStringGetCStringPtr(dir_path, encmethod);
  }

done:
  return path;
}


#endif

DataFileManager::DataFileManager()
{
  const char *extraPath = getenv("TARSIS_ETC_DATA_DIR");

  addSearchPath(std::filesystem::current_path().string());
  if (extraPath != nullptr)
    addSearchPath(extraPath);

#if defined(__APPLE__)
  const char *rsrcPath = get_resource_bundle_path(CFSTR("TARSISETC"));
  if (rsrcPath != nullptr)
    addSearchPath(rsrcPath + std::string("/data"));
#endif
}

DataFileManager *
DataFileManager::instance()
{
  if (g_instance == nullptr)
    g_instance = new DataFileManager();
  
  return g_instance;
}

bool
DataFileManager::addSearchPath(std::string const &path)
{
  struct stat sbuf;

  if (stat(path.c_str(), &sbuf) == -1) {
    fprintf(
      stderr,
      "warning: cannot stat `%s': %s\n",
      path.c_str(),
      strerror(errno));
    return false;
  }

  if (!S_ISDIR(sbuf.st_mode)) {
    fprintf(
      stderr,
      "warning: file `%s' is not a directory\n",
      path.c_str());
    return false;
  }

  m_paths.push_front(path);
  return true;
}

std::string
DataFileManager::find(std::string const &path, int flags)
{
  if (path.empty())
    return path;
  
  if (path[0] == '/') {
    if (access(path.c_str(), flags) != -1)
      return path;

    // Write access: check if we can create a file there
    if (flags & W_OK && errno == ENOENT) {
      char *copy = strdup(path.c_str());
      if (copy == nullptr)
        throw std::runtime_error("Memory exhausted");
      char *dir = dirname(copy);
      auto ret = access(dir, W_OK | X_OK);
      free(copy);

      if (ret != -1)
        return path;
    }
  } else {
    for (auto const &p : m_paths) {
      std::string fullPath = p + "/" + path;
      if (access(fullPath.c_str(), flags) != -1)
        return fullPath;

      // Write access:: check if we can create a file here
      if (flags & W_OK && errno == ENOENT)
        if (access(p.c_str(), W_OK | X_OK) != -1)
          return fullPath;
    }
  }

  return std::string();
}

std::string
DataFileManager::resolve(std::string const &path)
{
  return find(path, R_OK);
}

std::string
DataFileManager::suggest(std::string const &path)
{
  return find(path, W_OK);
}

std::list<std::string> const &
DataFileManager::searchPaths() const
{
  return m_paths;
}

