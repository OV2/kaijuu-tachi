#ifndef NALL_WINDOWS_REGISTRY_HPP
#define NALL_WINDOWS_REGISTRY_HPP

#include <nall/platform.hpp>
#include <nall/string.hpp>

#include <shlwapi.h>
#ifndef KEY_WOW64_64KEY
  #define KEY_WOW64_64KEY 0x0100
#endif
#ifndef KEY_WOW64_32KEY
  #define KEY_WOW64_32KEY 0x0200
#endif

namespace nall {

struct registry {
  static void tree(const string &key, const string &data = "") {
    lstring part = key.split("/");
    string path;
    HKEY handle, rootKey = root(part[0]);
    DWORD disposition;
    for(unsigned i = 1; i < part.size(); i++) {
      path.append(part[i]);
      if(RegCreateKeyExW(rootKey, utf16_t(path), 0, NULL, 0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &handle, &disposition) == ERROR_SUCCESS) {
        if(i == part.size() - 1) {
          RegSetValueExW(handle, NULL, 0, REG_SZ, (BYTE*)(wchar_t*)utf16_t(data), (data.length() + 1) * sizeof(wchar_t));
        }
        RegCloseKey(handle);
      }
      path.append("\\");
    }
  }

  static void leaf(const string &key, const string &data = "") {
    lstring part = key.split("/");
    string path;
    HKEY handle, rootKey = root(part[0]);
    DWORD disposition;
    for(unsigned i = 1; i < part.size() - 1; i++) {
      path.append(part[i]);
      if(RegCreateKeyExW(rootKey, utf16_t(path), 0, NULL, 0, KEY_WOW64_64KEY | KEY_ALL_ACCESS, NULL, &handle, &disposition) == ERROR_SUCCESS) {
        if(i == part.size() - 2) {
          RegSetValueExW(handle, utf16_t(part[i + 1]), 0, REG_SZ, (BYTE*)(wchar_t*)utf16_t(data), (data.length() + 1) * sizeof(wchar_t));
        }
        RegCloseKey(handle);
      }
      path.append("\\");
    }
  }

  static string read(const string &key) {
    lstring part = key.split("/");
    HKEY handle, rootKey = root(part[0]);
    part.remove(0);
    string path = part.concatenate("\\");
    if(RegOpenKeyExW(rootKey, utf16_t(path), 0, KEY_WOW64_64KEY | KEY_READ, &handle) == ERROR_SUCCESS) {
      wchar_t data[PATH_MAX] = L"";
      DWORD size = PATH_MAX * sizeof(wchar_t);
      LONG result = RegQueryValueExW(handle, NULL, NULL, NULL, (LPBYTE)&data, (LPDWORD)&size);
      RegCloseKey(handle);
      if(result == ERROR_SUCCESS) return (const char*)utf8_t(data);
    }
    string node = part[part.size() - 1];
    part.remove(part.size() - 1);
    path = part.concatenate("\\");
    if(RegOpenKeyExW(rootKey, utf16_t(path), 0, KEY_WOW64_64KEY | KEY_READ, &handle) == ERROR_SUCCESS) {
      wchar_t data[PATH_MAX] = L"";
      DWORD size = PATH_MAX * sizeof(wchar_t);
      LONG result = RegQueryValueExW(handle, utf16_t(node), NULL, NULL, (LPBYTE)&data, (LPDWORD)&size);
      RegCloseKey(handle);
      if(result == ERROR_SUCCESS) return (const char*)utf8_t(data);
    }
    return "";
  }

  static bool exists(const string &key) {
    lstring part = key.split("/");
    HKEY handle, rootKey = root(part[0]);
    part.remove(0);
    string path = part.concatenate("\\");
    if(RegOpenKeyExW(rootKey, utf16_t(path), 0, KEY_WOW64_64KEY | KEY_READ, &handle) == ERROR_SUCCESS) {
      wchar_t data[PATH_MAX] = L"";
      DWORD size = PATH_MAX * sizeof(wchar_t);
      LONG result = RegQueryValueExW(handle, NULL, NULL, NULL, (LPBYTE)&data, (LPDWORD)&size);
      RegCloseKey(handle);
      if(result == ERROR_SUCCESS) return true;
    }
    string node = part[part.size() - 1];
    part.remove(part.size() - 1);
    path = part.concatenate("\\");
    if(RegOpenKeyExW(rootKey, utf16_t(path), 0, KEY_WOW64_64KEY | KEY_READ, &handle) == ERROR_SUCCESS) {
      wchar_t data[PATH_MAX] = L"";
      DWORD size = PATH_MAX * sizeof(wchar_t);
      LONG result = RegQueryValueExW(handle, utf16_t(node), NULL, NULL, (LPBYTE)&data, (LPDWORD)&size);
      RegCloseKey(handle);
      if(result == ERROR_SUCCESS) return true;
    }
    return false;
  }

  static bool remove(const string &key) {
    lstring part = key.split("/");
    HKEY rootKey = root(part[0]);
    part.remove(0);
    string path = part.concatenate("\\");
    if(SHDeleteKeyW(rootKey, utf16_t(path)) == ERROR_SUCCESS) return true;
    string node = part[part.size() - 1];
    part.remove(part.size() - 1);
    path = part.concatenate("\\");
    if(SHDeleteValueW(rootKey, utf16_t(path), utf16_t(node)) == ERROR_SUCCESS) return true;
    return false;
  }

  static lstring trees(const string &key) {
    lstring part = key.split("/"), result;
    HKEY handle, rootKey = root(part[0]);
    part.remove(0);
    string path = part.concatenate("\\");
    if(RegOpenKeyExW(rootKey, utf16_t(path), 0, KEY_WOW64_64KEY | KEY_READ, &handle) == ERROR_SUCCESS) {
      DWORD trees, leaves;
      RegQueryInfoKey(handle, NULL, NULL, NULL, &trees, NULL, NULL, &leaves, NULL, NULL, NULL, NULL);
      for(unsigned index = 0; index < trees; index++) {
        wchar_t name[MAX_PATH] = L"";
        DWORD size = MAX_PATH * sizeof(wchar_t);
        RegEnumKeyEx(handle, index, (wchar_t*)&name, &size, NULL, NULL, NULL, NULL);
        result.append((const char*)utf8_t(name));
      }
      RegCloseKey(handle);
    }
    return result;
  }

  static lstring leaves(const string &key) {
    lstring part = key.split("/"), result;
    HKEY handle, rootKey = root(part[0]);
    part.remove(0);
    string path = part.concatenate("\\");
    if(RegOpenKeyExW(rootKey, utf16_t(path), 0, KEY_WOW64_64KEY | KEY_READ, &handle) == ERROR_SUCCESS) {
      DWORD trees, leaves;
      RegQueryInfoKey(handle, NULL, NULL, NULL, &trees, NULL, NULL, &leaves, NULL, NULL, NULL, NULL);
      for(unsigned index = 0; index < leaves; index++) {
        wchar_t name[MAX_PATH] = L"";
        DWORD size = MAX_PATH * sizeof(wchar_t);
        RegEnumValueW(handle, index, (wchar_t*)&name, &size, NULL, NULL, NULL, NULL);
        result.append((const char*)utf8_t(name));
      }
      RegCloseKey(handle);
    }
    return result;
  }

private:
  static HKEY root(const string &name) {
    if(name == "HKCR") return HKEY_CLASSES_ROOT;
    if(name == "HKCC") return HKEY_CURRENT_CONFIG;
    if(name == "HKCU") return HKEY_CURRENT_USER;
    if(name == "HKLM") return HKEY_LOCAL_MACHINE;
    if(name == "HKU" ) return HKEY_USERS;
    return NULL;
  }
};

}

#endif
