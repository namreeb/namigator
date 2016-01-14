#include "CommonControl.hpp"

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <cassert>
#include <cwchar>

CommonControl::CommonControl(HWND window)
    : m_instance((HINSTANCE)GetWindowLong(window, GWL_HINSTANCE)), m_window(window),
      m_labelFont(CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, VARIABLE_PITCH, L"Microsoft Sans Serif")),
      m_textBoxFont(CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                               ANTIALIASED_QUALITY, VARIABLE_PITCH, L"Microsoft Sans Serif"))
{
    auto const handler = [](HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto const ctrl = reinterpret_cast<CommonControl *>(GetWindowLong(hwnd, GWL_USERDATA));

        return ctrl->WindowProc(hwnd, message, wParam, lParam);
    };

    SetWindowLongPtr(window, GWL_USERDATA, (LONG)this);
    SetWindowLongPtr(window, GWL_WNDPROC, reinterpret_cast<LONG_PTR>(static_cast<decltype(&DefWindowProc)>(handler)));
}

LRESULT CommonControl::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // our custom message handler, which will only return if it actually handles something
    switch (message)
    {
        case WM_COMMAND:
        {
            wchar_t className[16];

            auto const control = reinterpret_cast<HWND>(lParam);

            if (GetClassName(control, className, sizeof(className)/sizeof(className[0])) &&
                !wcsncmp(className, L"Button", wcslen(className)))
            {
                auto const style = GetWindowLong(control, GWL_STYLE);

                // if it is a checkbox, find the checkbox handler
                if (!!(style & BS_CHECKBOX))
                {
                    if (IsDlgButtonChecked(hwnd, wParam))
                    {
                        CheckDlgButton(hwnd, wParam, BST_UNCHECKED);

                        auto handler = m_checkboxHandlers.find(static_cast<int>(wParam));

                        if (handler != m_checkboxHandlers.end())
                        {
                            handler->second(false);
                            return TRUE;
                        }
                    }
                    else
                    {
                        CheckDlgButton(hwnd, wParam, BST_CHECKED);

                        auto handler = m_checkboxHandlers.find(static_cast<int>(wParam));

                        if (handler != m_checkboxHandlers.end())
                        {
                            handler->second(true);
                            return TRUE;
                        }
                    }
                }
                // otherwise, assume it is a button!
                else
                {
                    auto handler = m_buttonHandlers.find(static_cast<int>(wParam));

                    if (handler != m_buttonHandlers.end())
                    {
                        handler->second();
                        return TRUE;
                    }
                }
            }
        }
    }

    // if we reach here, we haven't found anything that is any concern of ours.  pass it off to the default handler
    return DefWindowProc(hwnd, message, wParam, lParam);
}

void CommonControl::AddLabel(const std::wstring &text, int x, int y, int width, int height)
{
    auto control = CreateWindow(L"STATIC", text.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, width, height, m_window, nullptr, m_instance, nullptr);

    SendMessage(control, WM_SETFONT, (WPARAM)m_labelFont, MAKELPARAM(TRUE, 0));
}

void CommonControl::AddTextBox(int id, const std::wstring &text, int x, int y, int width, int height)
{
    auto control = CreateWindow(L"EDIT", text.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, x, y, width, height, m_window, nullptr, m_instance, nullptr);

    SendMessage(control, WM_SETFONT, (WPARAM)m_textBoxFont, MAKELPARAM(TRUE, 0));

    m_controls.insert(std::pair<int, HWND>(id, control));
}

void CommonControl::AddComboBox(int id, const std::vector<std::wstring> &items, int x, int y, int width, int height)
{
    auto control = CreateWindow(L"COMBOBOX", nullptr, CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL,
                                x, y, width, height, m_window, nullptr, m_instance, nullptr);

    SendMessage(control, WM_SETFONT, (WPARAM)m_textBoxFont, MAKELPARAM(TRUE, 0));
    
    for (auto &s : items)
        SendMessage(control, CB_ADDSTRING, (WPARAM)0, (LPARAM)s.c_str());

    SendMessage(control, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    m_controls.insert(std::pair<int, HWND>(id, control));
}

void CommonControl::AddButton(int id, const std::wstring &text, int x, int y, int width, int height, std::function<void()> handler)
{
    auto control = CreateWindow(L"BUTTON", text.c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD, x, y, width, height, m_window, (HMENU)id, m_instance, nullptr);

    SendMessage(control, WM_SETFONT, (WPARAM)m_labelFont, MAKELPARAM(TRUE, 0));

    m_buttonHandlers.insert(std::pair<int, decltype(handler)>(id, handler));
}

void CommonControl::AddCheckBox(int id, const std::wstring &text, int x, int y, int width, int height, bool checked, std::function<void (bool)> handler)
{
    auto control = CreateWindow(L"BUTTON", text.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_CHECKBOX, x, y, width, height, m_window, (HMENU)id, m_instance, nullptr);

    SendMessage(control, WM_SETFONT, (WPARAM)m_textBoxFont, MAKELPARAM(TRUE, 0));

    if (checked)
        CheckDlgButton(m_window, id, BST_CHECKED);

    m_checkboxHandlers.insert(std::pair<int, decltype(handler)>(id, handler));
}

const std::string CommonControl::GetText(int id) const
{
    auto control = m_controls.find(id);

    assert(control != m_controls.end());

    std::vector<char> buffer(GetWindowTextLengthA(control->second)+1);
    GetWindowTextA(control->second, &buffer[0], buffer.size());

    return std::string(&buffer[0]);
}