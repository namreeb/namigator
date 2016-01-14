#pragma once

#include <Windows.h>
#include <string>
#include <vector>
#include <map>

class CommonControl
{
    private:
        const HINSTANCE m_instance;
        const HWND m_window;
        const HFONT m_labelFont;
        const HFONT m_textBoxFont;

        std::map<int, HWND> m_controls;

    public:
        CommonControl(HWND window);

        // adding controls
        void AddLabel(const std::wstring &text, int x, int y, int width, int height);
        void AddTextBox(int id, const std::wstring &text, int x, int y, int width, int height);
        void AddComboBox(int id, const std::vector<std::wstring> &items, int x, int y, int width, int height);
        void AddButton(int id, const std::wstring &text, int x, int y, int width, int height);
        void AddCheckBox(int id, const std::wstring &text, int x, int y, int width, int height, bool checked);

        const std::string GetText(int id) const;
};