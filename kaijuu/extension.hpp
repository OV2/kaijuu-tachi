struct CShellExt : IContextMenu, IShellExtInit {
protected:
  uint instanceCount;
  string_vector fileList;

public:
  CShellExt();
  ~CShellExt();

  auto QueryInterface(REFIID, LPVOID*) -> STDMETHODIMP;
  auto AddRef() -> STDMETHODIMP_(ULONG);
  auto Release() -> STDMETHODIMP_(ULONG);

  auto QueryContextMenu(HMENU, UINT, UINT, UINT, UINT) -> STDMETHODIMP;
  auto InvokeCommand(LPCMINVOKECOMMANDINFO) -> STDMETHODIMP;
  auto GetCommandString(UINT_PTR, UINT, UINT FAR*, LPSTR, UINT) -> STDMETHODIMP;
  auto Initialize(LPCITEMIDLIST, LPDATAOBJECT, HKEY) -> STDMETHODIMP;

private:
  vector<uint> matchedRules();
};

typedef CShellExt *LPCSHELLEXT;