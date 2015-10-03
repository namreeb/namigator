#include <string>

#include "CommonControl.hpp"

void CommonControl::AddLabel(const std::wstring &name, const std::wstring &text, int x, int y, int width, int height)
{
    auto label = CreateWindow(L"STATIC", name.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP, x, y, width, height, m_window, NULL, m_instance, NULL);
    SetWindowText(label, text.c_str());
}