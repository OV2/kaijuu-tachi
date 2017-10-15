#include <nall/platform.hpp>
#include <nall/directory.hpp>
#include <nall/file.hpp>
#include <nall/string.hpp>
#include <nall/vector.hpp>
#include <nall/windows/registry.hpp>
using namespace nall;

#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <strsafe.h>
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <uxtheme.h>
#include <list>
#include <string>
#include <map>
#define IDM_CFOPEN 0

HINSTANCE module = NULL;
uint referenceCount = 0;

#include "guid.hpp"
#include "settings.hpp"
#include "iconloader.hpp"
#include "extension.hpp"
#include "factory.hpp"
