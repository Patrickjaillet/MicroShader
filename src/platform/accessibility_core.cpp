#include "accessibility_core.h"

#include "uia_minimal.h"

#include <atomic>
#include <mutex>
#include <string>
#include <vector>

#include "utf8.h"

namespace
{
    struct AccessibleElementData
    {
        std::wstring name;
        AccessibleRole role = AccessibleRole::Button;
        RECT screen_rect{};
        bool enabled = true;
        bool has_toggle_state = false;
        bool toggled = false;
    };

    long control_type_id(AccessibleRole role)
    {
        switch (role)
        {
            case AccessibleRole::CheckBox:
                return UIA_CheckBoxControlTypeId;
            case AccessibleRole::Button:
            default:
                return UIA_ButtonControlTypeId;
        }
    }

    std::mutex g_mutex;
    std::vector<AccessibleElementData> g_pending;
    std::vector<AccessibleElementData> g_committed;
    HWND g_hwnd = nullptr;
    WNDPROC g_prev_wndproc = nullptr;
    bool g_com_owned = false;

    std::vector<AccessibleElementData> snapshot()
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        return g_committed;
    }

    class RootProvider;

    class ElementProvider : public IRawElementProviderSimple, public IRawElementProviderFragment, public IToggleProvider
    {
    public:
        ElementProvider(RootProvider* root, size_t index, AccessibleElementData data);
        virtual ~ElementProvider();

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* pRetVal) override;
        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal) override;
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) override;
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) override;

        HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) override;
        HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** pRetVal) override;
        HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* pRetVal) override;
        HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) override;
        HRESULT STDMETHODCALLTYPE SetFocus() override;
        HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) override;

        HRESULT STDMETHODCALLTYPE Toggle() override;
        HRESULT STDMETHODCALLTYPE get_ToggleState(ToggleState* pRetVal) override;

    private:
        std::atomic<ULONG> ref_count_;
        RootProvider* root_;
        size_t index_;
        AccessibleElementData data_;
    };

    class RootProvider : public IRawElementProviderSimple, public IRawElementProviderFragment, public IRawElementProviderFragmentRoot
    {
    public:
        explicit RootProvider(HWND hwnd) : ref_count_(1), hwnd_(hwnd) {}

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override
        {
            if (ppv == nullptr)
            {
                return E_POINTER;
            }
            if (riid == __uuidof(IUnknown) || riid == __uuidof(IRawElementProviderSimple))
            {
                *ppv = static_cast<IRawElementProviderSimple*>(this);
            }
            else if (riid == __uuidof(IRawElementProviderFragment))
            {
                *ppv = static_cast<IRawElementProviderFragment*>(this);
            }
            else if (riid == __uuidof(IRawElementProviderFragmentRoot))
            {
                *ppv = static_cast<IRawElementProviderFragmentRoot*>(this);
            }
            else
            {
                *ppv = nullptr;
                return E_NOINTERFACE;
            }
            AddRef();
            return S_OK;
        }

        ULONG STDMETHODCALLTYPE AddRef() override
        {
            return ++ref_count_;
        }

        ULONG STDMETHODCALLTYPE Release() override
        {
            ULONG count = --ref_count_;
            if (count == 0)
            {
                delete this;
            }
            return count;
        }

        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* pRetVal) override
        {
            *pRetVal = ProviderOptions_ServerSideProvider;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID, IUnknown** pRetVal) override
        {
            *pRetVal = nullptr;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) override
        {
            VariantInit(pRetVal);
            if (propertyId == UIA_NamePropertyId)
            {
                pRetVal->vt = VT_BSTR;
                pRetVal->bstrVal = SysAllocString(L"uShader");
            }
            else if (propertyId == UIA_ControlTypePropertyId)
            {
                pRetVal->vt = VT_I4;
                pRetVal->lVal = UIA_WindowControlTypeId;
            }
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) override
        {
            return UiaHostProviderFromHwnd(hwnd_, pRetVal);
        }

        HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) override;

        HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** pRetVal) override
        {
            *pRetVal = nullptr;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* pRetVal) override
        {
            RECT rect{};
            GetWindowRect(hwnd_, &rect);
            pRetVal->left = rect.left;
            pRetVal->top = rect.top;
            pRetVal->width = rect.right - rect.left;
            pRetVal->height = rect.bottom - rect.top;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) override
        {
            *pRetVal = nullptr;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE SetFocus() override
        {
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) override
        {
            AddRef();
            *pRetVal = this;
            return S_OK;
        }

        HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal) override;

        HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** pRetVal) override
        {
            *pRetVal = nullptr;
            return S_OK;
        }

    private:
        std::atomic<ULONG> ref_count_;
        HWND hwnd_;
    };

    ElementProvider::ElementProvider(RootProvider* root, size_t index, AccessibleElementData data)
        : ref_count_(1), root_(root), index_(index), data_(std::move(data))
    {
        root_->AddRef();
    }

    ElementProvider::~ElementProvider()
    {
        root_->Release();
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::QueryInterface(REFIID riid, void** ppv)
    {
        if (ppv == nullptr)
        {
            return E_POINTER;
        }
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IRawElementProviderSimple))
        {
            *ppv = static_cast<IRawElementProviderSimple*>(this);
        }
        else if (riid == __uuidof(IRawElementProviderFragment))
        {
            *ppv = static_cast<IRawElementProviderFragment*>(this);
        }
        else if (riid == __uuidof(IToggleProvider) && data_.has_toggle_state)
        {
            *ppv = static_cast<IToggleProvider*>(this);
        }
        else
        {
            *ppv = nullptr;
            return E_NOINTERFACE;
        }
        AddRef();
        return S_OK;
    }

    ULONG STDMETHODCALLTYPE ElementProvider::AddRef()
    {
        return ++ref_count_;
    }

    ULONG STDMETHODCALLTYPE ElementProvider::Release()
    {
        ULONG count = --ref_count_;
        if (count == 0)
        {
            delete this;
        }
        return count;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::get_ProviderOptions(ProviderOptions* pRetVal)
    {
        *pRetVal = ProviderOptions_ServerSideProvider;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal)
    {
        *pRetVal = nullptr;
        if (patternId == UIA_TogglePatternId && data_.has_toggle_state)
        {
            AddRef();
            *pRetVal = static_cast<IToggleProvider*>(this);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal)
    {
        VariantInit(pRetVal);
        if (propertyId == UIA_NamePropertyId)
        {
            pRetVal->vt = VT_BSTR;
            pRetVal->bstrVal = SysAllocString(data_.name.c_str());
        }
        else if (propertyId == UIA_ControlTypePropertyId)
        {
            pRetVal->vt = VT_I4;
            pRetVal->lVal = control_type_id(data_.role);
        }
        else if (propertyId == UIA_IsEnabledPropertyId)
        {
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = data_.enabled ? VARIANT_TRUE : VARIANT_FALSE;
        }
        else if (propertyId == UIA_IsControlElementPropertyId || propertyId == UIA_IsContentElementPropertyId)
        {
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_TRUE;
        }
        else if (propertyId == UIA_IsKeyboardFocusablePropertyId || propertyId == UIA_HasKeyboardFocusPropertyId)
        {
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = VARIANT_FALSE;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal)
    {
        *pRetVal = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal)
    {
        *pRetVal = nullptr;
        if (direction == NavigateDirection_Parent)
        {
            root_->AddRef();
            *pRetVal = root_;
            return S_OK;
        }
        std::vector<AccessibleElementData> elements = snapshot();
        if (direction == NavigateDirection_NextSibling && index_ + 1 < elements.size())
        {
            *pRetVal = new ElementProvider(root_, index_ + 1, elements[index_ + 1]);
        }
        else if (direction == NavigateDirection_PreviousSibling && index_ > 0 && index_ - 1 < elements.size())
        {
            *pRetVal = new ElementProvider(root_, index_ - 1, elements[index_ - 1]);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::GetRuntimeId(SAFEARRAY** pRetVal)
    {
        LONG runtime_id[] = {static_cast<LONG>(UiaAppendRuntimeId), static_cast<LONG>(index_)};
        SAFEARRAY* array = SafeArrayCreateVector(VT_I4, 0, 2);
        if (array == nullptr)
        {
            *pRetVal = nullptr;
            return S_OK;
        }
        for (LONG i = 0; i < 2; ++i)
        {
            SafeArrayPutElement(array, &i, &runtime_id[i]);
        }
        *pRetVal = array;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::get_BoundingRectangle(UiaRect* pRetVal)
    {
        pRetVal->left = data_.screen_rect.left;
        pRetVal->top = data_.screen_rect.top;
        pRetVal->width = data_.screen_rect.right - data_.screen_rect.left;
        pRetVal->height = data_.screen_rect.bottom - data_.screen_rect.top;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal)
    {
        *pRetVal = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::SetFocus()
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal)
    {
        root_->AddRef();
        *pRetVal = root_;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::Toggle()
    {
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE ElementProvider::get_ToggleState(ToggleState* pRetVal)
    {
        *pRetVal = data_.toggled ? ToggleState_On : ToggleState_Off;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RootProvider::Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal)
    {
        *pRetVal = nullptr;
        std::vector<AccessibleElementData> elements = snapshot();
        if (elements.empty())
        {
            return S_OK;
        }
        if (direction == NavigateDirection_FirstChild)
        {
            *pRetVal = new ElementProvider(this, 0, elements.front());
        }
        else if (direction == NavigateDirection_LastChild)
        {
            *pRetVal = new ElementProvider(this, elements.size() - 1, elements.back());
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE RootProvider::ElementProviderFromPoint(double x, double y, IRawElementProviderFragment** pRetVal)
    {
        *pRetVal = nullptr;
        std::vector<AccessibleElementData> elements = snapshot();
        for (size_t i = elements.size(); i > 0; --i)
        {
            size_t index = i - 1;
            const RECT& rect = elements[index].screen_rect;
            if (x >= rect.left && x < rect.right && y >= rect.top && y < rect.bottom)
            {
                *pRetVal = new ElementProvider(this, index, elements[index]);
                break;
            }
        }
        return S_OK;
    }

    RootProvider* g_root = nullptr;

    LRESULT CALLBACK accessibility_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        if (msg == WM_GETOBJECT && g_root != nullptr
            && (static_cast<long>(lparam) == static_cast<long>(OBJID_CLIENT) || static_cast<long>(lparam) == UiaRootObjectId))
        {
            LRESULT result = UiaReturnRawElementProvider(hwnd, wparam, lparam, g_root);
            if (result != 0)
            {
                return result;
            }
        }
        WNDPROC prev = g_prev_wndproc;
        if (prev != nullptr)
        {
            return CallWindowProcW(prev, hwnd, msg, wparam, lparam);
        }
        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

void accessibility_init_hwnd(HWND hwnd)
{
    if (hwnd == nullptr)
    {
        return;
    }
    g_hwnd = hwnd;

    HRESULT com_result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    g_com_owned = SUCCEEDED(com_result) && com_result != S_FALSE;

    g_root = new RootProvider(g_hwnd);

    LONG_PTR prev = SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&accessibility_wndproc));
    g_prev_wndproc = reinterpret_cast<WNDPROC>(prev);
}

void accessibility_shutdown()
{
    if (g_hwnd == nullptr)
    {
        return;
    }
    if (g_prev_wndproc != nullptr)
    {
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_prev_wndproc));
    }
    UiaReturnRawElementProvider(g_hwnd, 0, 0, nullptr);
    if (g_root != nullptr)
    {
        g_root->Release();
        g_root = nullptr;
    }
    if (g_com_owned)
    {
        CoUninitialize();
        g_com_owned = false;
    }
    g_hwnd = nullptr;
    g_prev_wndproc = nullptr;
}

void accessibility_begin_frame()
{
    g_pending.clear();
}

namespace
{
    void accessibility_register_impl(const char* name, AccessibleRole role, float client_x, float client_y, float width, float height, bool enabled, bool has_toggle_state, bool toggled)
    {
        if (g_hwnd == nullptr)
        {
            return;
        }
        AccessibleElementData element;
        element.name = utf8_to_wide(name);
        element.role = role;
        element.enabled = enabled;
        element.has_toggle_state = has_toggle_state;
        element.toggled = toggled;

        POINT top_left{static_cast<LONG>(client_x), static_cast<LONG>(client_y)};
        POINT bottom_right{static_cast<LONG>(client_x + width), static_cast<LONG>(client_y + height)};
        ClientToScreen(g_hwnd, &top_left);
        ClientToScreen(g_hwnd, &bottom_right);
        element.screen_rect = RECT{top_left.x, top_left.y, bottom_right.x, bottom_right.y};

        g_pending.push_back(std::move(element));
    }
}

void accessibility_register(const char* name, AccessibleRole role, float client_x, float client_y, float width, float height, bool enabled)
{
    accessibility_register_impl(name, role, client_x, client_y, width, height, enabled, false, false);
}

void accessibility_register_toggle(const char* name, AccessibleRole role, float client_x, float client_y, float width, float height, bool enabled, bool toggled)
{
    accessibility_register_impl(name, role, client_x, client_y, width, height, enabled, true, toggled);
}

void accessibility_end_frame()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_committed = g_pending;
}
