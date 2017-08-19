struct CShellExtClassFactory : IClassFactory {
protected:
  uint instanceCount;

public:
  CShellExtClassFactory();
  ~CShellExtClassFactory();

  auto QueryInterface(REFIID, LPVOID FAR*) -> STDMETHODIMP;
  auto AddRef() -> STDMETHODIMP_(ULONG);
  auto Release() -> STDMETHODIMP_(ULONG);

  auto CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR*) -> STDMETHODIMP;
  auto LockServer(BOOL) -> STDMETHODIMP;
};

typedef CShellExtClassFactory *LPCSHELLEXTCLASSFACTORY;
