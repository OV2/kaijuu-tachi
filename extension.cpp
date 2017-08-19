CShellExt::CShellExt() {
  instanceCount = 0;
  referenceCount++;
}

CShellExt::~CShellExt() {
  referenceCount--;
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID *ppv) {
  *ppv = nullptr;

  if(IsEqualIID(riid, IID_IShellExtInit) || IsEqualIID(riid, IID_IUnknown)) {
    *ppv = (IShellExtInit*)this;
  } else if(IsEqualIID(riid, IID_IContextMenu)) {
    *ppv = (IContextMenu*)this;
  }

  if(*ppv) {
    AddRef();
    return NOERROR;
  }

  return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CShellExt::AddRef() {
  return ++instanceCount;
}

STDMETHODIMP_(ULONG) CShellExt::Release() {
  if(--instanceCount) return instanceCount;
  delete this;
  return 0;
}

STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder, IDataObject *pDataObject, HKEY hRegKey) {
  fileList.reset();

  if(pDataObject) {
    wchar_t filename[PATH_MAX];
    STGMEDIUM medium;
    FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    HDROP hDrop;

    if(FAILED(pDataObject->GetData(&fe, &medium))) return E_INVALIDARG;
    hDrop = (HDROP)GlobalLock(medium.hGlobal);
    if(hDrop == NULL) return E_INVALIDARG;

    fileList.reset();
    unsigned count = DragQueryFileW(hDrop, 0xffffffff, NULL, 0);
    for(unsigned i = 0; i < count; i++) {
      DragQueryFileW((HDROP)medium.hGlobal, i, filename, PATH_MAX);
      string name = (const char*)utf8_t(filename);
      name.transform("/", "\\");
      if(name.endswith("\\")) name.rtrim<1>("\\");
      fileList.append(name);
    }
    GlobalUnlock(medium.hGlobal);
    ReleaseStgMedium(&medium);
  }

  return S_OK;
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
  settings.load();
  unsigned idCmd = idCmdFirst;
  bool firstDefault = true;

  auto ruleIDs = matchedRules();
  for(auto &ruleID : ruleIDs) {
    auto &rule = settings.rules(ruleID);
    if(idCmd < idCmdLast) {
      InsertMenuW(hMenu, indexMenu, MF_STRING | MF_BYPOSITION, idCmd++, (const wchar_t*)utf16_t(rule.name));
      if(rule.defaultAction && firstDefault) {
        firstDefault = false;  //there can be only one default menu item
        SetMenuDefaultItem(hMenu, indexMenu, TRUE);
      }
      indexMenu++;
    }
  }

  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(idCmd - idCmdFirst));
}

STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd, UINT uFlags, LPUINT lpReserved, LPSTR pszName, UINT uMaxNameLen) {
  if(idCmd < settings.rules.size()) return S_OK;
  return E_INVALIDARG;
}

STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi) {
  if(HIWORD(lpcmi->lpVerb) != 0) return E_INVALIDARG;  //ignore actual string verbs

  settings.load();
  unsigned id = LOWORD(lpcmi->lpVerb);
  auto ruleIDs = matchedRules();
  auto &rule = settings.rules(ruleIDs(id));

  string filename = fileList(0);
  string pathname = filename;
  if(directory::exists(pathname) == false) pathname = dir(pathname);
  if(pathname.endswith("\\")) pathname.rtrim<1>("\\");

  string filenames, pathnames;
  for(auto &filename : fileList) {
    string pathname = filename;
    if(directory::exists(pathname) == false) pathname = dir(pathname);
    if(pathname.endswith("\\")) pathname.rtrim<1>("\\");
    filenames.append("\"", filename, "\" ");
    pathnames.append("\"", pathname, "\" ");
  }
  filenames.rtrim<1>(" ");
  pathnames.rtrim<1>(" ");

  lstring params = rule.command.qsplit<1>(" ");
  params(1).replace("{ufile}", filename);
  params(1).replace("{upath}", pathname);
  params(1).replace("{file}", string{"\"", filename, "\""});
  params(1).replace("{path}", string{"\"", pathname, "\""});
  params(1).replace("{files}", filenames);
  params(1).replace("{paths}", pathnames);

  if((intptr_t)ShellExecuteW(NULL, L"open", utf16_t(params(0)), utf16_t(params(1)), utf16_t(dir(filename)), SW_SHOWNORMAL) <= 32) {
    MessageBoxW(0, L"Error opening associated program.", L"kaijuu", MB_OK);
  }

  return S_OK;
}

vector<unsigned> CShellExt::matchedRules() {
  vector<unsigned> matched;
  if(fileList.size() == 0) return matched;

  for(unsigned id = 0; id < settings.rules.size(); id++) {
    auto &rule = settings.rules(id);
    if(rule.multiSelection == false && fileList.size() > 1) continue;
    bool proceed = true;
    for(auto &filename : fileList) {
      string name = filename;
      if(directory::exists(name)) {
        if(rule.matchFolders == false) {
          proceed = false;
          break;
        }
      } else {
        if(rule.matchFiles == false) {
          proceed = false;
          break;
        }
      }
      name = notdir(name);

      lstring patternList = rule.pattern.split(";");
      bool found = false;
      for(auto &pattern : patternList) {
        if(name.wildcard(pattern)) {
          found = true;
          break;
        }
      }
      if(found == false) {
        proceed = false;
        break;
      }
    }
    if(proceed == false) continue;
    matched.append(id);
  }

  return matched;
}
