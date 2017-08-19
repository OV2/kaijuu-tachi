CShellExtClassFactory::CShellExtClassFactory() {
  instanceCount = 0;
  referenceCount++;
}

CShellExtClassFactory::~CShellExtClassFactory() {
  referenceCount--;
}

auto CShellExtClassFactory::QueryInterface(REFIID riid, LPVOID *ppv) -> STDMETHODIMP {
  *ppv = NULL;
  if(IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
    *ppv = (LPCLASSFACTORY)this;
    AddRef();
    return NOERROR;
  }
  return E_NOINTERFACE;
}

auto CShellExtClassFactory::AddRef() -> STDMETHODIMP_(ULONG) {
  return ++instanceCount;
}

auto CShellExtClassFactory::Release() -> STDMETHODIMP_(ULONG) {
  if(--instanceCount) return instanceCount;
  delete this;
  return 0;
}

auto CShellExtClassFactory::CreateInstance(LPUNKNOWN pUnknownOuter, REFIID riid, LPVOID *ppv) -> STDMETHODIMP {
  *ppv = NULL;
  if(pUnknownOuter) return CLASS_E_NOAGGREGATION;
  CShellExt *pShellExt = new CShellExt();
  if(pShellExt == NULL) return E_OUTOFMEMORY;
  return pShellExt->QueryInterface(riid, ppv);
}

auto CShellExtClassFactory::LockServer(BOOL fLock) -> STDMETHODIMP {
  return NOERROR;
}
