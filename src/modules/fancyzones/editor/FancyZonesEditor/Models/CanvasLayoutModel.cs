// Copyright (c) Microsoft Corporation
// The Microsoft Corporation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Text.Json;
using System.Windows;

namespace FancyZonesEditor.Models
{
    // CanvasLayoutModel
    //  Free form Layout Model, which specifies independent zone rects
    public class CanvasLayoutModel : LayoutModel
    {
        public CanvasLayoutModel(string uuid, string name, int referenceWidth, int referenceHeight, IList<Int32Rect> zones) : base(uuid, name)
        {
            _referenceWidth = referenceWidth;
            _referenceHeight = referenceHeight;
            Zones = zones;
        }
        
        public CanvasLayoutModel(string name, ushort id, int referenceWidth, int referenceHeight)
        : base(name, id)
        {
            // Initialize Reference Size
            _referenceWidth = referenceWidth;
            _referenceHeight = referenceHeight;
        }

        public CanvasLayoutModel(Guid guid, string name) : base(guid.ToString(), name)
        {
        }

        public CanvasLayoutModel(string name, ushort id)
            : base(name, id)
        {
        }

        public CanvasLayoutModel(string name)
            : base(name)
        {
        }

        public CanvasLayoutModel()
            : base()
        {
        }

        // ReferenceWidth - the reference width for the layout rect that all Zones are relative to
        public int ReferenceWidth
        {
            get
            {
                return _referenceWidth;
            }

            set
            {
                if (_referenceWidth != value)
                {
                    _referenceWidth = value;
                    FirePropertyChanged("ReferenceWidth");
                }
            }
        }

        private int _referenceWidth;

        // ReferenceHeight - the reference height for the layout rect that all Zones are relative to
        public int ReferenceHeight
        {
            get
            {
                return _referenceHeight;
            }

            set
            {
                if (_referenceHeight != value)
                {
                    _referenceHeight = value;
                    FirePropertyChanged("ReferenceHeight");
                }
            }
        }

        private int _referenceHeight;

        // Zones - the list of all zones in this layout, described as independent rectangles
        public IList<Int32Rect> Zones { get; } = new List<Int32Rect>();

        // RemoveZoneAt
        //  Removes the specified index from the Zones list, and fires a property changed notification for the Zones property
        public void RemoveZoneAt(int index)
        {
            Zones.RemoveAt(index);
            FirePropertyChanged("Zones");
        }

        // AddZone
        //  Adds the specified Zone to the end of the Zones list, and fires a property changed notification for the Zones property
        public void AddZone(Int32Rect zone)
        {
            Zones.Add(zone);
            FirePropertyChanged("Zones");
        }

        private void Load(byte[] data)
        {
            // Initialize this CanvasLayoutModel based on the given persistence data
            // Skip version (2 bytes), id (2 bytes), and type (1 bytes)
            int i = 5;
            _referenceWidth = (data[i++] * 256) + data[i++];
            _referenceHeight = (data[i++] * 256) + data[i++];

            int count = data[i++];

            while (count-- > 0)
            {
                Zones.Add(new Int32Rect(
                    (data[i++] * 256) + data[i++],
                    (data[i++] * 256) + data[i++],
                    (data[i++] * 256) + data[i++],
                    (data[i++] * 256) + data[i++]));
            }
        }

        // Clone
        //  Implements the LayoutModel.Clone abstract method
        //  Clones the data from this CanvasLayoutModel to a new CanvasLayoutModel
        public override LayoutModel Clone()
        {
            CanvasLayoutModel layout = new CanvasLayoutModel(Name)
            {
                ReferenceHeight = ReferenceHeight,
                ReferenceWidth = ReferenceWidth,
            };

            foreach (Int32Rect zone in Zones)
            {
                layout.Zones.Add(zone);
            }

            return layout;
        }

        // PersistData
        // Implements the LayoutModel.PersistData abstract method
        protected override void PersistData()
        {
            FileStream outputStream = File.Open(Settings.AppliedZoneSetTmpFile, FileMode.Create);
            JsonWriterOptions writerOptions = new JsonWriterOptions();
            writerOptions.SkipValidation = true;
            using (var writer = new Utf8JsonWriter(outputStream, writerOptions))
            {
                writer.WriteStartObject();
                writer.WriteString("uuid", Guid.ToString());
                writer.WriteString("name", Name);

                writer.WriteString("type", "canvas");

                writer.WriteStartObject("info");

                writer.WriteNumber("ref-width", _referenceWidth);
                writer.WriteNumber("ref-height", _referenceHeight);

                writer.WriteStartArray("zones");
                foreach (Int32Rect rect in Zones)
                {
                    writer.WriteStartObject();
                    writer.WriteNumber("X", rect.X);
                    writer.WriteNumber("Y", rect.Y);
                    writer.WriteNumber("width", rect.Width);
                    writer.WriteNumber("height", rect.Height);
                    writer.WriteEndObject();
                }
                writer.WriteEndArray();
                // end info object
                writer.WriteEndObject();
                // end root object
                writer.WriteEndObject();
                writer.Flush();
            }
            outputStream.Close();
        }
    }
}
