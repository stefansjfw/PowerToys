#include "pch.h"
#include "Util.h"

static int s_classId = 0;

LRESULT CALLBACK DLLWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL RegisterDLLWindowClass(LPCWSTR szClassName, Mocks::HwndCreator* creator)
{
    if (!creator)
        return false;

    WNDCLASSEX wc;

    wc.hInstance = creator->getHInstance();
    wc.lpszClassName = szClassName;
    wc.lpfnWndProc = DLLWindowProc;
    wc.cbSize = sizeof(WNDCLASSEX);

    wc.style = CS_DBLCLKS;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

    auto regRes = RegisterClassEx(&wc);
    return regRes;
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
    MSG messages;
    Mocks::HwndCreator* creator = reinterpret_cast<Mocks::HwndCreator*>(lpParam);
    if (!creator)
        return -1;

    if (RegisterDLLWindowClass((LPCWSTR)creator->getWindowClassName().c_str(), creator) != 0)
    {
        auto hWnd = CreateWindowEx(0, (LPCWSTR)creator->getWindowClassName().c_str(), (LPCWSTR)creator->getTitle().c_str(), WS_EX_APPWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 10, 10, nullptr, nullptr, creator->getHInstance(), NULL);
        creator->setHwnd(hWnd);
        creator->setCondition(true);

        while (GetMessage(&messages, NULL, 0, 0))
        {
            TranslateMessage(&messages);
            DispatchMessage(&messages);
        }

        creator->setHwnd(hWnd);
    }
    else
    {
        creator->setCondition(true);
    }

    return 1;
}

namespace Mocks
{
    HwndCreator::HwndCreator(const std::wstring& title) :
        m_windowTitle(title), m_windowClassName(std::to_wstring(++s_classId)), m_conditionFlag(false), m_thread(nullptr), m_hInst(HINSTANCE{}), m_hWnd(nullptr)
    {
    }

    HwndCreator::~HwndCreator()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_conditionVar.wait(lock, [this] { return m_conditionFlag; });

        if (m_thread)
        {
            CloseHandle(m_thread);
        }
    }

    HWND HwndCreator::operator()(HINSTANCE hInst)
    {
        m_hInst = hInst;
        m_conditionFlag = false;
        std::unique_lock<std::mutex> lock(m_mutex);

        m_thread = CreateThread(0, NULL, ThreadProc, (LPVOID)this, NULL, NULL);
        m_conditionVar.wait(lock, [this] { return m_conditionFlag; });

        return m_hWnd;
    }

    void HwndCreator::setHwnd(HWND val)
    {
        m_hWnd = val;
    }

    void HwndCreator::setCondition(bool cond)
    {
        m_conditionFlag = cond;
        m_conditionVar.notify_one();
    }

}