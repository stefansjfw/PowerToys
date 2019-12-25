using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.ComponentModel;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.IO;
using System.Threading.Tasks;
using System.Windows;
using Microsoft.Win32;

namespace FancyZonesEditor.Models
{
    // Base LayoutModel
    //  Manages common properties and base persistence
    public abstract class LayoutModel : INotifyPropertyChanged
    {
        protected LayoutModel() { }

        protected LayoutModel(string name) : this()
        {
            Name = name;
        }

        protected LayoutModel(string uuid, string name) : this()
        {
            _guid = Guid.Parse(uuid);
            Name = name;
        }

        protected LayoutModel(string name, ushort id) : this(name)
        {
            _id = id;
        }

        //   Name - the display name for this layout model - is also used as the key in the registry
        public string Name
        {
            get { return _name; }
            set
            {
                if (_name != value)
                {
                    _name = value;
                    FirePropertyChanged("Name");
                }
            }
        }
        private string _name;

        // Id - the unique ID for this layout model - is used to connect fancy zones' ZonesSets with the editor's Layouts
        //    - note: 0 means this is a new layout, which means it will have its ID auto-assigned on persist
        public ushort Id
        {
            get
            {
                if (_id == 0)
                {
                    _id = ++s_maxId;
                }
                return _id;
            }
        }
        private ushort _id = 0;

        public Guid Guid
        {
            get
            {
                return _guid;
            }
        }
        private Guid _guid;

        // IsSelected (not-persisted) - tracks whether or not this LayoutModel is selected in the picker
        // TODO: once we switch to a picker per monitor, we need to move this state to the view  
        public bool IsSelected
        {
            get { return _isSelected; }
            set
            {
                if (_isSelected != value)
                {
                    _isSelected = value;
                    FirePropertyChanged("IsSelected");
                }
            }
        }
        private bool _isSelected;

        // implementation of INotifyProeprtyChanged
        public event PropertyChangedEventHandler PropertyChanged;

        // FirePropertyChanged -- wrapper that calls INPC.PropertyChanged
        protected virtual void FirePropertyChanged(string propertyName)
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }

        // Removes this Layout from the registry and the loaded CustomModels list
        public void Delete()
        {
            int i = s_customModels.IndexOf(this);
            if (i != -1)
            {
                s_customModels.RemoveAt(i);
            }
        }

        // Loads all the Layouts persisted under the Layouts key in the registry
        public static ObservableCollection<LayoutModel> LoadCustomModels()
        {
            s_customModels = new ObservableCollection<LayoutModel>();

            FileStream inputStream = File.Open(Settings.CustomZoneSetsTmpFile, FileMode.Open);
            var jsonObject = JsonDocument.Parse(inputStream, options: default);
            JsonElement.ArrayEnumerator customZoneSetsEnumerator = jsonObject.RootElement.GetProperty("custom-zone-sets").EnumerateArray();
            while (customZoneSetsEnumerator.MoveNext())
            {
                var current = customZoneSetsEnumerator.Current;
                string name = current.GetProperty("name").GetString();
                string type = current.GetProperty("type").GetString();
                string uuid = current.GetProperty("uuid").GetString();
                var info = current.GetProperty("info");
                if (type.Equals("grid"))
                {
                    int rows = info.GetProperty("rows").GetInt32();
                    int columns = info.GetProperty("columns").GetInt32();
                    int[] rowsPercentage = new int[rows];
                    JsonElement.ArrayEnumerator rowsPercentageEnumerator = info.GetProperty("rows-percentage").EnumerateArray();
                    int i = 0;
                    while (rowsPercentageEnumerator.MoveNext())
                    {
                        rowsPercentage[i++] = rowsPercentageEnumerator.Current.GetInt32();
                    }
                    i = 0;
                    int[] columnsPercentage = new int[columns];
                    JsonElement.ArrayEnumerator columnsPercentageEnumerator = info.GetProperty("columns-percentage").EnumerateArray();
                    while (columnsPercentageEnumerator.MoveNext())
                    {
                        columnsPercentage[i++] = columnsPercentageEnumerator.Current.GetInt32();
                    }
                    i = 0;
                    JsonElement.ArrayEnumerator cellChildMapRows = info.GetProperty("cell-child-map").EnumerateArray();
                    int[,] cellChildMap = new int[rows, columns];
                    while (cellChildMapRows.MoveNext())
                    {
                        int j = 0;
                        JsonElement.ArrayEnumerator cellChildMapRowElems = cellChildMapRows.Current.EnumerateArray();
                        while (cellChildMapRowElems.MoveNext())
                        {
                            cellChildMap[i, j++] = cellChildMapRowElems.Current.GetInt32();
                        }
                        i++;
                    }
                    s_customModels.Add(new GridLayoutModel(uuid, name, rows, columns, rowsPercentage, columnsPercentage, cellChildMap));
                }
                else if (type.Equals("canvas"))
                {
                    int referenceWidth = info.GetProperty("ref-width").GetInt32();
                    int referenceHeight = info.GetProperty("ref-height").GetInt32();
                    JsonElement.ArrayEnumerator zonesEnumerator = info.GetProperty("zones").EnumerateArray();
                    IList<Int32Rect> zones = new List<Int32Rect>();
                    while (zonesEnumerator.MoveNext())
                    {
                        int x = zonesEnumerator.Current.GetProperty("X").GetInt32();
                        int y = zonesEnumerator.Current.GetProperty("Y").GetInt32();
                        int width = zonesEnumerator.Current.GetProperty("width").GetInt32();
                        int height = zonesEnumerator.Current.GetProperty("height").GetInt32();
                        zones.Add(new Int32Rect(x, y, width, height));
                    }
                    s_customModels.Add(new CanvasLayoutModel(uuid, name, referenceWidth, referenceHeight, zones));
                }
            }
            return s_customModels;
        }
        private static ObservableCollection<LayoutModel> s_customModels = null;

        private static ushort s_maxId = 0;

        // Callbacks that the base LayoutModel makes to derived types
        protected abstract void PersistData();
        public abstract LayoutModel Clone();
        
        public void Persist(System.Windows.Int32Rect[] zones)
        {
            PersistData();
            Apply(zones);
        }

        public void Apply(System.Windows.Int32Rect[] zones)
        { 
            int zoneCount = zones.Length;
            FileStream outputStream = File.Open(Settings.ActiveZoneSetTmpFile, FileMode.Create);
            var writer = new Utf8JsonWriter(outputStream, options: default);

            writer.WriteStartObject();
            writer.WriteString("device-id", Settings.UniqueKey);

            writer.WriteStartObject("active-zoneset");
            writer.WriteString("uuid", "{" + Guid.ToString().ToUpper() + "}");
            bool custom = false;
            switch (_id)
            {
                case Settings.c_focusModelId:
                    writer.WriteString("type", "focus");
                    break;
                case Settings.c_rowsModelId:
                    writer.WriteString("type", "rows");
                    break;
                case Settings.c_columnsModelId:
                    writer.WriteString("type", "columns");
                    break;
                case Settings.c_gridModelId:
                    writer.WriteString("type", "grid");
                    break;
                case Settings.c_priorityGridModelId:
                    writer.WriteString("type", "priority-grid");
                    break;
                default:
                    writer.WriteString("type", "custom");
                    custom = true;
                    break;
            }
            if (!custom)
            {
                writer.WriteNumber("zone-count", zoneCount);
            }
            writer.WriteEndObject();

            writer.WriteBoolean("editor-show-spacing", Settings._settingsToPersist.ShowSpacing);
            writer.WriteNumber("editor-spacing", Settings._settingsToPersist.Spacing);
            writer.WriteNumber("editor-zone-count", Settings._settingsToPersist.ZoneCount);
            writer.WriteEndObject();
            writer.Flush();
            outputStream.Close();
        }

        private static readonly string c_registryPath = Settings.RegistryPath + "\\Layouts";
        private static readonly string c_fullRegistryPath = Settings.FullRegistryPath + "\\Layouts";
    }
}
