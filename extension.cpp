CShellExt::CShellExt() {
  instanceCount = 0;
  referenceCount++;
}

CShellExt::~CShellExt() {
  referenceCount--;
}

STDMETHODIMP CShellExt::QueryInterface(REFIID riid, LPVOID *ppv) {
  *ppv = NULL;

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
  if(pDataObject) getFileNamesFromPdata(pDataObject);
  return S_OK;
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
  if(fileList.size() == 1) {
    string filename = fileList(0);
    lstring rules = registry::contents("HKCU/Software/Kaijuu/");
    for(auto &rule : rules) {
      string path = {"HKCU/Software/Kaijuu/", rule};
      string filter = string{rule}.rtrim<1>("/");
      bool makeDefault = registry::read({path, "Default"}) == "true";
      string association = registry::read({path, "Association"});
      string description = registry::read({path, "Description"});

      if(filename.wildcard(filter) && association.empty() == false) {
        MENUITEMINFOW mii = {0};
        mii.fMask = MIIM_STRING | MIIM_ID;
        mii.cbSize = sizeof(mii);
        mii.wID = idCmdFirst + IDM_CFOPEN;
        utf16_t s(description);
        mii.dwTypeData = s;
        InsertMenuItemW(hMenu, indexMenu, TRUE, &mii);
        if(makeDefault) SetMenuDefaultItem(hMenu, indexMenu, TRUE);
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(1));
      }
    }
  }

  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
}

STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCommand, UINT uFlags, LPUINT lpReserved, LPSTR pszName, UINT uMaxNameLen) {
  HRESULT hr = E_INVALIDARG;

  if(idCommand == IDM_CFOPEN) {
    switch(uFlags) {
    case GCS_HELPTEXTA:
      hr = StringCbCopyA(pszName, uMaxNameLen, "Opens folder with kaijuu");
      break;
    case GCS_HELPTEXTW:
      hr = StringCbCopyW((LPWSTR)pszName, uMaxNameLen, L"Opens folder with kaijuu");
      break;
    case GCS_VERBA:
      hr = StringCbCopyA(pszName, uMaxNameLen, "CFOPEN");
      break;
    case GCS_VERBW:
      hr = StringCbCopyW((LPWSTR)pszName, uMaxNameLen, L"CFOPEN");
      break;
    default:
      hr = S_OK;
      break;
    }
  }

  return hr;
}

STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi) {
  BOOL fEx = FALSE;
  BOOL fUnicode = FALSE;

  if(lpcmi->cbSize == sizeof(CMINVOKECOMMANDINFOEX)) {
    fEx = TRUE;
    if((lpcmi->fMask & CMIC_MASK_UNICODE)) fUnicode = TRUE;
  }

  if(!fUnicode && HIWORD(lpcmi->lpVerb)) {
    if(!lstrcmpiA(lpcmi->lpVerb, "CFOPEN")) cartridgeFolderOpen();
    else return E_FAIL;
  }

  else if(fUnicode && HIWORD(((CMINVOKECOMMANDINFOEX*)lpcmi)->lpVerbW)) {
    if(!lstrcmpiW(((CMINVOKECOMMANDINFOEX*)lpcmi)->lpVerbW, L"CFOPEN")) cartridgeFolderOpen();
    else return E_FAIL;
  }

  else if(LOWORD(lpcmi->lpVerb) == IDM_CFOPEN) {
    cartridgeFolderOpen();
  }

  else {
    return E_FAIL;
  }

  return S_OK;
}

void CShellExt::getFileNamesFromPdata(LPDATAOBJECT pDataObject) {
  wchar_t filename[MAX_PATH];
  STGMEDIUM medium;
  FORMATETC fe = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
  HDROP hDrop;

  if(FAILED(pDataObject->GetData(&fe, &medium))) return;
  hDrop = (HDROP)GlobalLock(medium.hGlobal);
  if(hDrop == NULL) return;

  fileList.reset();
  unsigned count = DragQueryFileW(hDrop, 0xffffffff, NULL, 0);
  for(unsigned i = 0; i < count; i++) {
    DragQueryFileW((HDROP)medium.hGlobal, i, filename, MAX_PATH);
    fileList.append((const char*)utf8_t(filename));
  }
  GlobalUnlock(medium.hGlobal);
  ReleaseStgMedium(&medium);
}

void CShellExt::cartridgeFolderOpen() {
  string filename = fileList(0);
  lstring rules = registry::contents("HKCU/Software/Kaijuu/");
  for(auto &rule : rules) {
    string path = {"HKCU/Software/Kaijuu/", rule};
    string filter = string{rule}.rtrim<1>("/");

    if(filename.wildcard(filter)) {
      string program = registry::read({path, "Association"});
      lstring params = program.qsplit<1>(" ");
      params(1).replace("{path}", fileList(0));

      if((intptr_t)ShellExecuteW(NULL, L"open", utf16_t(params(0)), utf16_t(params(1)), NULL, SW_SHOWNORMAL) <= 32) {
        MessageBoxW(0, L"Error opening associated program.", L"kaijuu", MB_OK);
      }
      return;
    }
  }
}
