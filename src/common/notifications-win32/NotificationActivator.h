#pragma once

#include "DesktopNotificationManagerCompat.h"
#include <NotificationActivationCallback.h>
#include <windows.ui.notifications.h>
#include <wrl.h>

using namespace ABI::Windows::Data::Xml::Dom;
using namespace ABI::Windows::UI::Notifications;
using namespace Microsoft::WRL;

namespace NotificationsWin32
{
    // The UUID CLSID must be unique to your app. Create a new GUID if copying this code.
    class DECLSPEC_UUID("DD5CACDA-7C2E-4997-A62A-04A597B58F76") NotificationActivator WrlSealed WrlFinal : public RuntimeClass<RuntimeClassFlags<ClassicCom>, INotificationActivationCallback>
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Activate(
            _In_ LPCWSTR appUserModelId,
            _In_ LPCWSTR invokedArgs,
            _In_reads_(dataCount) const NOTIFICATION_USER_INPUT_DATA* data,
            ULONG dataCount) override
        {
            return 1L;
        }
    };

    // Flag class as COM creatable
    CoCreatableClass(NotificationActivator);

}