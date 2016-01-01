#include <string>

#include "CommonControl.hpp"

CommonControl::CommonControl(HWND window)
    : m_instance((HINSTANCE)GetWindowLong(window, GWL_HINSTANCE)), m_window(window),
      m_labelFont(CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                             ANTIALIASED_QUALITY, VARIABLE_PITCH, L"Microsoft Sans Serif")),
      m_textBoxFont(CreateFont(16, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                               ANTIALIASED_QUALITY, VARIABLE_PITCH, L"Microsoft Sans Serif"))
{}

void CommonControl::AddLabel(const std::wstring &text, int x, int y, int width, int height)
{
    auto control = CreateWindow(L"STATIC", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, width, height, m_window, nullptr, m_instance, nullptr);

    SendMessage(control, WM_SETFONT, (WPARAM)m_labelFont, MAKELPARAM(TRUE, 0));
    SetWindowText(control, text.c_str());

}

void CommonControl::AddTextBox(const std::wstring &name, const std::wstring &text, int x, int y, int width, int height)
{
    auto control = CreateWindow(L"EDIT", name.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_BORDER, x, y, width, height, m_window, nullptr, m_instance, nullptr);

    SendMessage(control, WM_SETFONT, (WPARAM)m_textBoxFont, MAKELPARAM(TRUE, 0));
    SetWindowText(control, text.c_str());

    m_controls.insert(std::pair<std::wstring, HWND>(name, control));
}

void CommonControl::AddComboBox(const std::wstring &name, const std::vector<std::wstring> &items, int x, int y, int width, int height)
{
    auto control = CreateWindow(L"COMBOBOX", name.c_str(), CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | WS_VSCROLL,
                                x, y, width, height, m_window, nullptr, m_instance, nullptr);

    SendMessage(control, WM_SETFONT, (WPARAM)m_textBoxFont, MAKELPARAM(TRUE, 0));
    
    for (auto &s : items)
        SendMessage(control, CB_ADDSTRING, (WPARAM)0, (LPARAM)s.c_str());

    SendMessage(control, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    m_controls.insert(std::pair<std::wstring, HWND>(name, control));
}

void CommonControl::AddButton(const std::wstring &text, int id, int x, int y, int width, int height)
{
    auto control = CreateWindow(L"BUTTON", text.c_str(), WS_TABSTOP | WS_VISIBLE | WS_CHILD, x, y, width, height, m_window, (HMENU)id, m_instance, nullptr);

    SendMessage(control, WM_SETFONT, (WPARAM)m_labelFont, MAKELPARAM(TRUE, 0));
    SetWindowText(control, text.c_str());
}

const std::string CommonControl::GetText(const std::wstring &name) const
{
    auto control = m_controls.find(name);

    if (control == m_controls.end())
        throw "control not found";

    std::vector<char> buffer(GetWindowTextLengthA(control->second)+1);
    GetWindowTextA(control->second, &buffer[0], buffer.size());

    return std::string(&buffer[0]);
}