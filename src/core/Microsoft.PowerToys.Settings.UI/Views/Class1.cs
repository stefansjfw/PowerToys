// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using Microsoft.UI.Xaml.Automation.Peers;
using Microsoft.UI.Xaml.Controls;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Automation.Peers;

namespace Microsoft.PowerToys.Settings.UI.Views
{
    public class Class1 : DropDownButton
    {

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new MyAutomationPeer(this);
        }

        internal class MyAutomationPeer : DropDownButtonAutomationPeer
        {
            public MyAutomationPeer(DropDownButton button)
                : base(button) { }

            protected override bool IsControlElementCore()
            {
                return Owner.Visibility != Windows.UI.Xaml.Visibility.Collapsed;
            }
        }
    }

}
