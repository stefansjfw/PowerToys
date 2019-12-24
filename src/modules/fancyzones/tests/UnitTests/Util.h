#pragma once

namespace CustomAssert
{
    static void AreEqual(const RECT& r1, const RECT& r2)
    {
        const bool equal = ((r1.left == r2.left) && (r1.right == r2.right) && (r1.top == r2.top) && (r1.bottom == r2.bottom));
        Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(equal);
    }

    static void AreEqual(GUID g1, GUID g2)
    {
        Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(g1 == g2);
    }

    static void AreEqual(WORD w1, WORD w2)
    {
        Microsoft::VisualStudio::CppUnitTestFramework::Assert::IsTrue(w1 == w2);
    }
}

namespace Mocks
{
    static HWND Window()
    {
        static UINT_PTR s_nextWindow = 0;
        return reinterpret_cast<HWND>(++s_nextWindow);
    }

    static HMONITOR Monitor()
    {
        static UINT_PTR s_nextMonitor = 0;
        return reinterpret_cast<HMONITOR>(++s_nextMonitor);
    }

    static HINSTANCE Instance()
    {
        static UINT_PTR s_nextInstance = 0;
        return reinterpret_cast<HINSTANCE>(++s_nextInstance);
    }

    class HwndCreator
    {
    public:
        HwndCreator(const std::wstring& title = L"");

        ~HwndCreator();

        HWND operator()(HINSTANCE hInst);

        void setHwnd(HWND val);
        void setCondition(bool cond);

        inline HINSTANCE getHInstance() const { return m_hInst; }
        inline const std::wstring& getTitle() const { return m_windowTitle; }
        inline const std::wstring& getWindowClassName() const { return m_windowClassName; }

    private:
        std::wstring m_windowTitle;
        std::wstring m_windowClassName;

        std::mutex m_mutex;
        std::condition_variable m_conditionVar;
        bool m_conditionFlag;
        HANDLE m_thread;

        HINSTANCE m_hInst;
        HWND m_hWnd;
    };
}

