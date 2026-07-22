#pragma once

#include <windows.h>
#include <oleauto.h>

typedef int PROPERTYID;
typedef int PATTERNID;
typedef int CONTROLTYPEID;

struct UiaRect
{
    double left;
    double top;
    double width;
    double height;
};

enum NavigateDirection
{
    NavigateDirection_Parent = 0,
    NavigateDirection_NextSibling = 1,
    NavigateDirection_PreviousSibling = 2,
    NavigateDirection_FirstChild = 3,
    NavigateDirection_LastChild = 4
};

enum ProviderOptions
{
    ProviderOptions_ClientSideProvider = 0x1,
    ProviderOptions_ServerSideProvider = 0x2,
    ProviderOptions_NonClientAreaProvider = 0x4,
    ProviderOptions_OverrideProvider = 0x8,
    ProviderOptions_ProviderOwnsSetFocus = 0x10,
    ProviderOptions_UseComThreading = 0x20,
    ProviderOptions_RefuseNonClientSupport = 0x40,
    ProviderOptions_HasNativeIAccessible = 0x80,
    ProviderOptions_UseClientCoordinates = 0x100
};

enum ToggleState
{
    ToggleState_Off = 0,
    ToggleState_On = 1,
    ToggleState_Indeterminate = 2
};

struct IRawElementProviderFragment;
struct IRawElementProviderFragmentRoot;

MIDL_INTERFACE("d6dd68d1-86fd-4332-8666-9abedea2d24c")
IRawElementProviderSimple : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE get_ProviderOptions(enum ProviderOptions* pRetVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) = 0;
};

MIDL_INTERFACE("f7063da8-8359-439c-9297-bbc5299a7d87")
IRawElementProviderFragment : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Navigate(enum NavigateDirection direction, IRawElementProviderFragment** pRetVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** pRetVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* pRetVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE SetFocus(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) = 0;
};

MIDL_INTERFACE("620ce2a5-ab8f-40a9-86cb-de3c75599b58")
IRawElementProviderFragmentRoot : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** pRetVal) = 0;
};

MIDL_INTERFACE("56d00bd0-c4f4-433c-a836-1a52a57e0892")
IToggleProvider : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE Toggle(void) = 0;
    virtual HRESULT STDMETHODCALLTYPE get_ToggleState(enum ToggleState* pRetVal) = 0;
};

extern "C"
{
    LRESULT WINAPI UiaReturnRawElementProvider(HWND hwnd, WPARAM wParam, LPARAM lParam, IRawElementProviderSimple* el);
    HRESULT WINAPI UiaHostProviderFromHwnd(HWND hwnd, IRawElementProviderSimple** ppProvider);
}

constexpr LONG UiaAppendRuntimeId = 3;
constexpr LONG UiaRootObjectId = -25;

constexpr PROPERTYID UIA_NamePropertyId = 30005;
constexpr PROPERTYID UIA_ControlTypePropertyId = 30003;
constexpr PROPERTYID UIA_IsEnabledPropertyId = 30010;
constexpr PROPERTYID UIA_IsControlElementPropertyId = 30016;
constexpr PROPERTYID UIA_IsContentElementPropertyId = 30017;
constexpr PROPERTYID UIA_IsKeyboardFocusablePropertyId = 30009;
constexpr PROPERTYID UIA_HasKeyboardFocusPropertyId = 30008;
constexpr CONTROLTYPEID UIA_ButtonControlTypeId = 50000;
constexpr CONTROLTYPEID UIA_CheckBoxControlTypeId = 50002;
constexpr CONTROLTYPEID UIA_WindowControlTypeId = 50032;
constexpr PATTERNID UIA_TogglePatternId = 10015;
