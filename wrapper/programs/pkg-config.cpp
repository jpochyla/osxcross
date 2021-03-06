/***********************************************************************
 *  OSXCross Compiler Wrapper                                          *
 *  Copyright (C) 2014, 2015 by Thomas Poechtrager                     *
 *  t.poechtrager@gmail.com                                            *
 *                                                                     *
 *  This program is free software; you can redistribute it and/or      *
 *  modify it under the terms of the GNU General Public License        *
 *  as published by the Free Software Foundation; either version 2     *
 *  of the License, or (at your option) any later version.             *
 *                                                                     *
 *  This program is distributed in the hope that it will be useful,    *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *  GNU General Public License for more details.                       *
 *                                                                     *
 *  You should have received a copy of the GNU General Public License  *
 *  along with this program; if not, write to the Free Software        *
 *  Foundation, Inc.,                                                  *
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.      *
 ***********************************************************************/

#include "proginc.h"

#ifndef _WIN32
#include <unistd.h>
#endif

extern char **environ;
using namespace tools;

namespace program {
namespace osxcross {

struct envvar {
  std::string name;
  std::string value;
  envvar() {}
  envvar(std::string name, std::string value) : name(name), value(value) {}
};

static envvar &var(const char *p, envvar &evar, const bool skipval = false) {
  const char *value = strchr(p, '=') + 1; // find value offset
  evar.name.assign(p, value - p - 1);
  if (!skipval)
    evar.value = value;
  return evar;
}

int pkg_config(int argc, char **argv, Target &target) {
  (void)argc;

  std::vector<envvar> envvars;
  std::vector<envvar> unset;
  envvar evar;

  if (!getenv("OSXCROSS_PKG_CONFIG_NO_MP_INC")) {
    std::string MacPortsSysRoot;

    if (target.getMacPortsSysRootDir(MacPortsSysRoot)) {
      concatEnvVariable("OSXCROSS_PKG_CONFIG_SYSROOT_DIR", MacPortsSysRoot);
      std::string MacPortsPkgConfigPath;

      if (target.getMacPortsPkgConfigDir(MacPortsPkgConfigPath))
        concatEnvVariable("OSXCROSS_PKG_CONFIG_PATH", MacPortsPkgConfigPath);
    }
  } else {
    unsetenv("OSXCROSS_PKG_CONFIG_NO_MP_INC");
  }

  // Map OSXCROSS_PKG_* to PKG_*.
  for (char **env = environ; *env; ++env) {
    char *p = *env;

    if (!strncmp(p, "OSXCROSS_PKG_CONFIG", 19)) {
      p += 9; // skip OSXCROSS_
      envvars.push_back(var(p, evar));
    } else if (!strncmp(p, "PKG_CONFIG", 10)) {
      // Unset native pkg-config vars.
      unset.push_back(var(p, evar, true));
    }
  }

  if (!envvars.empty()) {
    for (const envvar &evar : unset)
      unsetenv(evar.name.c_str());

    for (const envvar &evar : envvars)
      setenv(evar.name.c_str(), evar.value.c_str(), 1);

    // Prevent pkg-config from looking for *.pc files
    // in pre-defined search paths, such as /usr.
    if (!getenv("PKG_CONFIG_LIBDIR"))
      setenv("PKG_CONFIG_LIBDIR", "", 1);

    if (execvp("pkg-config", argv))
      err << "cannot find or execute pkg-config" << err.endl();
  }

  return 1;
}

} // namespace osxcross
} // namespace program
