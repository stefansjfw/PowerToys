﻿// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System.Linq;

namespace FancyZonesEditor.Utils
{
    public static class StringUtils
    {
        public static string ToDashCase(this string str)
        {
            if (str.Length == 1)
            {
                return str;
            }

            return string.Concat(str.Select((x, i) => i > 0 && char.IsUpper(x) ? "-" + x.ToString() : x.ToString())).ToLower();
        }
    }
}
