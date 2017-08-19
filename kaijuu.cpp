#include "kaijuu.hpp"
#include "extension.cpp"
#include "factory.cpp"

extern "C" BOOL WINAPI DllMain(HINSTANCE hinstance, DWORD dwReason, LPVOID lpReserved) {
  if(dwReason == DLL_PROCESS_ATTACH) {
    module = hinstance;
  }

  if(dwReason == DLL_PROCESS_DETACH) {
  }

  return TRUE;
}

STDAPI DllCanUnloadNow() {
  return referenceCount == 0 ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer() {
  wchar_t fileName[MAX_PATH];
  GetModuleFileNameW(module, fileName, MAX_PATH);
  registry::tree({"HKCR/CLSID/", classID}, classDescription);
  registry::tree({"HKCR/CLSID/", classID, "/shellex/MayChangeDefaultMenu"});
  registry::tree({"HKCR/CLSID/", classID, "/InprocServer32"}, (const char*)utf8_t(fileName));
  registry::leaf({"HKCR/CLSID/", classID, "/InprocServer32/ThreadingModel"}, "Apartment");
  registry::tree({"HKCR/Directory/shellex/ContextMenuHandlers/", classDescription}, classID);
  registry::leaf({"HKLM/Software/Microsoft/Windows/CurrentVersion/Shell Extensions/Approved/", classID}, classDescription);
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
  return S_OK;
}

STDAPI DllUnregisterServer() {
  registry::remove({"HKCR/CLSID/", classID});
  registry::remove({"HKCR/Directory/shellex/ContextMenuHandlers/", classDescription});
  registry::remove({"HKLM/Software/Microsoft/Windows/CurrentVersion/Shell Extensions/Approved/", classID});
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
  return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv) {
  *ppv = NULL;
  if(IsEqualIID(rclsid, CLSID_ShellExtension)) {
    CShellExtClassFactory *pcf = new CShellExtClassFactory;
    return pcf->QueryInterface(riid, ppv);
  }
  return CLASS_E_CLASSNOTAVAILABLE;
}
