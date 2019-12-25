﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.ComponentModel;
using System.Collections;
using System.Collections.ObjectModel;
using System.Globalization;
using FancyZonesEditor.Models;
using System.Windows.Documents;
using System.Windows;
using System.Windows.Controls;
using Microsoft.Win32;

namespace FancyZonesEditor
{
    //
    // Settings
    //  These are the configuration settings used by the rest of the editor
    //  Other UIs in the editor will subscribe to change events on the properties to stay up to date as these properties change
    //
    public class Settings : INotifyPropertyChanged
    {
        public bool IsCustomLayoutActive
        {
            get
            {
                foreach (LayoutModel model in CustomModels)
                {
                    if (model.IsSelected)
                    {
                        return true;
                    }
                }
                return false;
            }
        }

        public Settings()
        {
            ParseCommandLineArgs();

            // Initialize the five default layout models: Focus, Columns, Rows, Grid, and PriorityGrid
            _defaultModels = new List<LayoutModel>(5);
            _focusModel = new CanvasLayoutModel("Focus", c_focusModelId, (int)_workArea.Width, (int)_workArea.Height);
            _defaultModels.Add(_focusModel);

            _columnsModel = new GridLayoutModel("Columns", c_columnsModelId);
            _columnsModel.Rows = 1;
            _columnsModel.RowPercents = new int[1] { c_multiplier };
            _defaultModels.Add(_columnsModel);

            _rowsModel = new GridLayoutModel("Rows", c_rowsModelId);
            _rowsModel.Columns = 1;
            _rowsModel.ColumnPercents = new int[1] { c_multiplier };
            _defaultModels.Add(_rowsModel);

            _gridModel = new GridLayoutModel("Grid", c_gridModelId);
            _defaultModels.Add(_gridModel);

            _priorityGridModel = new GridLayoutModel("Priority Grid", c_priorityGridModelId);
            _defaultModels.Add(_priorityGridModel);

            _blankCustomModel = new CanvasLayoutModel("Create new custom", c_blankCustomModelId, (int)_workArea.Width, (int)_workArea.Height);

            _settingsToPersist = new SettingsToPersist(_showSpacing, _spacing, _zoneCount);

            UpdateLayoutModels();
        }

        // ZoneCount - number of zones selected in the picker window
        public int ZoneCount
        {
            get { return _zoneCount; }
            set
            {
                if (_zoneCount != value)
                {
                    _zoneCount = value;
                    _settingsToPersist.ZoneCount = value;
                    UpdateLayoutModels();
                    FirePropertyChanged("ZoneCount");
                }
            }
        }
        private int _zoneCount;

        // Spacing - how much space in between zones of the grid do you want
        public int Spacing
        {
            get { return _spacing; }
            set
            {
                if (_spacing != value)
                {
                    _spacing = value;
                    _settingsToPersist.Spacing = value;
                    FirePropertyChanged("Spacing");
                }
            }
        }
        private int _spacing;

        // ShowSpacing - is the Spacing value used or ignored?
        public bool ShowSpacing
        {
            get { return _showSpacing; }
            set
            {
                if (_showSpacing != value)
                {
                    _showSpacing = value;
                    _settingsToPersist.ShowSpacing = value;
                    FirePropertyChanged("ShowSpacing");
                }
            }
        }
        private bool _showSpacing;

        public class SettingsToPersist
        {
            public SettingsToPersist(bool showSpacing, int spacing, int zoneCount)
            {
                _showSpacing = showSpacing;
                _spacing = spacing;
                _zoneCount = zoneCount;
            }
            private bool _showSpacing;
            public bool ShowSpacing
            {
                get { return _showSpacing; }
                set { _showSpacing = value; }
            }

            private int _spacing;
            public int Spacing
            {
                get { return _spacing; }
                set { _spacing = value; }
            }

            private int _zoneCount;
            public int ZoneCount
            {
                get { return _zoneCount; }
                set { _zoneCount = value; }
            }
        }
        public static SettingsToPersist _settingsToPersist;

        // IsShiftKeyPressed - is the shift key currently being held down
        public bool IsShiftKeyPressed
        {
            get { return _isShiftKeyPressed; }
            set
            {
                if (_isShiftKeyPressed != value)
                {
                    _isShiftKeyPressed = value;
                    FirePropertyChanged("IsShiftKeyPressed");
                }
            }
        }
        private bool _isShiftKeyPressed;

        // IsCtrlKeyPressed - is the ctrl key currently being held down
        public bool IsCtrlKeyPressed
        {
            get { return _isCtrlKeyPressed; }
            set
            {
                if (_isCtrlKeyPressed != value)
                {
                    _isCtrlKeyPressed = value;
                    FirePropertyChanged("IsCtrlKeyPressed");
                }
            }
        }
        private bool _isCtrlKeyPressed;

        public Rect WorkArea
        {
            get { return _workArea; }
        }
        private Rect _workArea;

        public static uint Monitor
        {
            get { return _monitor; }
        }
        private static uint _monitor;

        public static String UniqueKey
        {
            get { return _uniqueKey; }
        }
        private static String _uniqueKey;
        private String _uniqueRegistryPath;

        public static String WorkAreaKey
        {
            get { return _workAreaKey; }
        }
        private static String _workAreaKey;

        public static float Dpi
        {
            get { return _dpi; }
        }
        private static float _dpi;
        public static String ActiveZoneSetTmpFile
        {
            get { return _activeZoneSetTmpFile; }
        }
        private static String _activeZoneSetTmpFile;

        public static String AppliedZoneSetTmpFile
        {
            get { return _appliedZoneSetTmpFile; }
        }
        private static String _appliedZoneSetTmpFile;

        public static String CustomZoneSetsTmpFile
        {
            get { return _customZoneSetsTmpFile; }
        }
        private static String _customZoneSetsTmpFile;

        // UpdateLayoutModels
        //  Update the five default layouts based on the new ZoneCount
        private void UpdateLayoutModels()
        {
            int previousZoneCount = _focusModel.Zones.Count;

            // Update the "Focus" Default Layout
            _focusModel.Zones.Clear();

            Int32Rect focusZoneRect = new Int32Rect((int)(_focusModel.ReferenceWidth * 0.1), (int)(_focusModel.ReferenceHeight * 0.1), (int)(_focusModel.ReferenceWidth * 0.6), (int)(_focusModel.ReferenceHeight * 0.6));
            int focusRectXIncrement = (ZoneCount <= 1) ? 0 : (int)(_focusModel.ReferenceWidth * 0.2) / (ZoneCount - 1);
            int focusRectYIncrement = (ZoneCount <= 1) ? 0 : (int)(_focusModel.ReferenceHeight * 0.2) / (ZoneCount - 1);

            for (int i = 0; i < ZoneCount; i++)
            {
                _focusModel.Zones.Add(focusZoneRect);
                focusZoneRect.X += focusRectXIncrement;
                focusZoneRect.Y += focusRectYIncrement;
            }

            // Update the "Rows" and "Columns" Default Layouts
            // They can share their model, just transposed
            _rowsModel.CellChildMap = new int[ZoneCount, 1];
            _columnsModel.CellChildMap = new int[1, ZoneCount];
            _rowsModel.Rows = _columnsModel.Columns = ZoneCount;
            _rowsModel.RowPercents = _columnsModel.ColumnPercents = new int[ZoneCount];

            for (int i = 0; i < ZoneCount; i++)
            {
                _rowsModel.CellChildMap[i, 0] = i;
                _columnsModel.CellChildMap[0, i] = i;
                _rowsModel.RowPercents[i] = c_multiplier / ZoneCount; // _columnsModel is sharing the same array
            }

            // Update the "Grid" Default Layout
            int rows = 1;
            int cols = 1;
            int mergeCount = 0;
            while (ZoneCount / rows >= rows)
            {
                rows++;
            }
            rows--;
            cols = ZoneCount / rows;
            if (ZoneCount % rows == 0)
            {
                // even grid
            }
            else
            {
                cols++;
                mergeCount = rows - (ZoneCount % rows);
            }
            _gridModel.Rows = rows;
            _gridModel.Columns = cols;
            _gridModel.RowPercents = new int[rows];
            _gridModel.ColumnPercents = new int[cols];
            _gridModel.CellChildMap = new int[rows, cols];

            for (int row = 0; row < rows; row++)
            {
                _gridModel.RowPercents[row] = c_multiplier / rows;
            }

            for (int col = 0; col < cols; col++)
            {
                _gridModel.ColumnPercents[col] = c_multiplier / cols;
            }

            int index = 0;
            for (int col = cols - 1; col >= 0; col--)
            {
                for (int row = rows - 1; row >= 0; row--)
                {
                    _gridModel.CellChildMap[row, col] = index++;
                    if (index == ZoneCount)
                    {
                        index--;
                    }

                }
            }

            // Update the "Priority Grid" Default Layout
            if (ZoneCount <= s_priorityData.Length)
            {
                _priorityGridModel.Reload(s_priorityData[ZoneCount - 1]);
            }
            else
            {
                // same as grid;
                _priorityGridModel.Rows = _gridModel.Rows;
                _priorityGridModel.Columns = _gridModel.Columns;
                _priorityGridModel.RowPercents = _gridModel.RowPercents;
                _priorityGridModel.ColumnPercents = _gridModel.ColumnPercents;
                _priorityGridModel.CellChildMap = _gridModel.CellChildMap;
            }
        }

        private void ParseCommandLineArgs()
        {
            _workArea = System.Windows.SystemParameters.WorkArea;
            _monitor = 0;
            _uniqueRegistryPath = FullRegistryPath;
            _uniqueKey = "";
            _dpi = 1;

            string[] args = Environment.GetCommandLineArgs();
            if (args.Length == 13)
            {
                // 1 = unique key for per-monitor settings
                // 2 = layoutid used to generate current layout (used to pick the default layout to show)
                // 3 = handle to monitor (passed back to engine to persist data)
                // 4 = X_Y_Width_Height in a dpi-scaled-but-unaware coords (where EditorOverlay shows up)
                // 5 = resolution key (passed back to engine to persist data)
                // 6 = monitor DPI (float)
                // 7 = temp file for active zone set
                // 8 = showSpacing value
                // 9 = spacing value
                // 10 = zoneCount value
                // 11 = temp file for applied zone set
                // 12 = FancyZones peristed data json file path

                _uniqueKey = args[1];
                _uniqueRegistryPath += "\\" + _uniqueKey;

                var parsedLocation = args[4].Split('_');
                var x = int.Parse(parsedLocation[0]);
                var y = int.Parse(parsedLocation[1]);
                var width = int.Parse(parsedLocation[2]);
                var height = int.Parse(parsedLocation[3]);

                _workAreaKey = args[5];

                // Try invariant culture first, caller likely uses invariant i.e. "C" locale to construct parameters
                foreach (var cultureInfo in new[] { CultureInfo.InvariantCulture, CultureInfo.CurrentCulture, CultureInfo.CurrentUICulture })
                {
                    try
                    {
                        _dpi = float.Parse(args[6], cultureInfo);
                        break;
                    }
                    catch (FormatException)
                    {
                    }
                }

                _activeZoneSetTmpFile = args[7];

                _showSpacing = int.Parse(args[8]) == 1;
                _spacing = int.Parse(args[9]);
                _zoneCount = int.Parse(args[10]);

                _appliedZoneSetTmpFile = args[11];
                _customZoneSetsTmpFile = args[12];

                _workArea = new Rect(x, y, width, height);

                uint monitor = 0;
                if (uint.TryParse(args[4], out monitor))
                {
                    _monitor = monitor;
                }
            }
        }


        public IList<LayoutModel> DefaultModels { get { return _defaultModels; } }
        public static ObservableCollection<LayoutModel> CustomModels
        {
            get
            {
                if (_customModels == null)
                {
                    _customModels = LayoutModel.LoadCustomModels();
                    _customModels.Insert(0, _blankCustomModel);
                }
                return _customModels;
            }
        }
        private static ObservableCollection<LayoutModel> _customModels;

        public static readonly string RegistryPath = "SOFTWARE\\SuperFancyZones";
        public static readonly string FullRegistryPath = "HKEY_CURRENT_USER\\" + RegistryPath;

        public static bool IsPredefinedLayout(LayoutModel model)
        {
            return (model.Id >= c_lastPrefinedId);
        }

        // implementation of INotifyProeprtyChanged
        public event PropertyChangedEventHandler PropertyChanged;

        // FirePropertyChanged -- wrapper that calls INPC.PropertyChanged
        protected virtual void FirePropertyChanged(string propertyName)
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null) handler(this, new PropertyChangedEventArgs(propertyName));
        }

        // storage for Default Layout Models
        private IList<LayoutModel> _defaultModels;
        private CanvasLayoutModel _focusModel; 
        private GridLayoutModel _rowsModel;   
        private GridLayoutModel _columnsModel;
        private GridLayoutModel _gridModel;
        private GridLayoutModel _priorityGridModel;
        private static CanvasLayoutModel _blankCustomModel;

        public const ushort c_focusModelId = 0xFFFF;
        public const ushort c_rowsModelId = 0xFFFE;
        public const ushort c_columnsModelId = 0xFFFD;
        public const ushort c_gridModelId = 0xFFFC;
        public const ushort c_priorityGridModelId = 0xFFFB;
        public const ushort c_blankCustomModelId = 0xFFFA;
        public const ushort c_lastPrefinedId = c_blankCustomModelId;

        // hard coded data for all the "Priority Grid" configurations that are unique to "Grid"
        private static byte[][] s_priorityData = new byte[][]
        {
            new byte[] { 0, 0, 0, 0, 0, 1, 1, 39, 16, 39, 16, 0 },
            new byte[] { 0, 0, 0, 0, 0, 1, 2, 39, 16, 26, 11, 13, 5, 0, 1 },
            new byte[] { 0, 0, 0, 0, 0, 1, 3, 39, 16, 9, 196, 19, 136, 9, 196, 0, 1, 2 },
            new byte[] { 0, 0, 0, 0, 0, 2, 3, 19, 136, 19, 136, 9, 196, 19, 136, 9, 196, 0, 1, 2, 0, 1, 3 },
            new byte[] { 0, 0, 0, 0, 0, 2, 3, 19, 136, 19, 136, 9, 196, 19, 136, 9, 196, 0, 1, 2, 3, 1, 4 },
            new byte[] { 0, 0, 0, 0, 0, 3, 3, 13, 5, 13, 6, 13, 5, 9, 196, 19, 136, 9, 196, 0, 1, 2, 0, 1, 3, 4, 1, 5 },
            new byte[] { 0, 0, 0, 0, 0, 3, 3, 13, 5, 13, 6, 13, 5, 9, 196, 19, 136, 9, 196, 0, 1, 2, 3, 1, 4, 5, 1, 6 },
            new byte[] { 0, 0, 0, 0, 0, 3, 4, 13, 5, 13, 6, 13, 5, 9, 196, 9, 196, 9, 196, 9, 196, 0, 1, 2, 3, 4, 1, 2, 5, 6, 1, 2, 7 },
            new byte[] { 0, 0, 0, 0, 0, 3, 4, 13, 5, 13, 6, 13, 5, 9, 196, 9, 196, 9, 196, 9, 196, 0, 1, 2, 3, 4, 1, 2, 5, 6, 1, 7, 8 },
            new byte[] { 0, 0, 0, 0, 0, 3, 4, 13, 5, 13, 6, 13, 5, 9, 196, 9, 196, 9, 196, 9, 196, 0, 1, 2, 3, 4, 1, 5, 6, 7, 1, 8, 9 },
            new byte[] { 0, 0, 0, 0, 0, 3, 4, 13, 5, 13, 6, 13, 5, 9, 196, 9, 196, 9, 196, 9, 196, 0, 1, 2, 3, 4, 1, 5, 6, 7, 8, 9, 10 }
        };

        private const int c_multiplier = 10000;
    }
}
