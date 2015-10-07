#pragma once

#include <Windows.h>
#include <string>
#include <vector>

class CommonControl
{
    private:
        const HINSTANCE m_instance;
        const HWND m_window;
        const HFONT m_labelFont;
        const HFONT m_textBoxFont;

    public:
        CommonControl(HWND window);

        // adding controls
        void AddLabel(const std::wstring &text, int x, int y, int width, int height);
        void AddTextBox(const std::wstring &name, const std::wstring &text, int x, int y, int width, int height);
        void AddComboBox(const std::wstring &name, const std::vector<std::wstring> &items, int x, int y, int width, int height);
        void AddButton(const std::wstring &text, int id, int x, int y, int width, int height);
};