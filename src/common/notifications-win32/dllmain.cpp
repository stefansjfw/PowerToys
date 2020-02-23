// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "DesktopNotificationManagerCompat.h"
#include <NotificationActivationCallback.h>
#include <windows.ui.notifications.h>
#include <wrl.h>

using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace Microsoft::WRL;

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

namespace NotificationsWin32
{
    bool RegisterNotificatorSender(const wchar_t* aumid, const GUID& guid)
    {
        // Register AUMID and COM server (for Desktop Bridge apps, this no-ops)
        auto ret = DesktopNotificationManagerCompat::RegisterAumidAndComServer(aumid, guid);
        if (!SUCCEEDED(ret))
        {
            return false;
        }
        auto ret2 = DesktopNotificationManagerCompat::RegisterActivator();
        if (!SUCCEEDED(ret2))
        {
            return false;
        }

        return true;
    }

    void SendToastNotification(const std::wstring& title, const std::wstring& message)
    {
        std::wstring xmlStr = L"<toast><visual><binding template='ToastGeneric'><text>" + title + message + L"</text></binding></visual></toast>";
        // Construct XML
        ComPtr<IXmlDocument> doc;
        auto hr = DesktopNotificationManagerCompat::CreateXmlDocumentFromString(
            xmlStr.c_str(),
            &doc);
        if (SUCCEEDED(hr))
        {
            // See full code sample to learn how to inject dynamic text, buttons, and more

            // Create the notifier
            // Classic Win32 apps MUST use the compat method to create the notifier
            ComPtr<IToastNotifier> notifier;
            hr = DesktopNotificationManagerCompat::CreateToastNotifier(&notifier);
            if (SUCCEEDED(hr))
            {
                // Create the notification itself (using helper method from compat library)
                ComPtr<IToastNotification> toast;
                hr = DesktopNotificationManagerCompat::CreateToastNotification(doc.Get(), &toast);
                if (SUCCEEDED(hr))
                {
                    // And show it!
                    hr = notifier->Show(toast.Get());
                }
            }
        }
    }
}
