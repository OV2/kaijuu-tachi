#pragma once

namespace ramus {

using namespace nall;

auto expandPath(string path) -> string {
  #if defined(PLATFORM_WINDOWS)
  wchar_t wMyDocuments[PATH_MAX] = L"";
  SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, wMyDocuments);  //deprecated, but SHGetKnownFolderPath won't compile
  string myDocuments = (string{(const char*)utf8_t(wMyDocuments), "\\"}).transform("\\", "/");
  path.replace("%MyDocuments%", myDocuments);  //unofficial environment variable

  wchar_t buffer[PATH_MAX] = L"";
  ExpandEnvironmentStringsW(utf16_t(path), buffer, PATH_MAX);

  string result = (const char*)utf8_t(buffer);
  result.transform("\\", "/");
  #else
  string result = path;
  #endif
  return result;
}

}
