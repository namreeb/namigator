#pragma once

#include <Windows.h>
#include <string>

class CommonControl
{
    private:
        const HINSTANCE m_instance;
        const HWND m_window;

    public:
        CommonControl(HINSTANCE instance, HWND window) : m_instance(instance), m_window(window) {}

        // adding controls
        void AddLabel(const std::wstring &name, const std::wstring &text, int x, int y, int width, int height);
};