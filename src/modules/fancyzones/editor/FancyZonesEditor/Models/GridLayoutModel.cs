﻿using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.Json;
using System.Threading.Tasks;
using System.Windows.Documents;

namespace FancyZonesEditor.Models
{
    // GridLayoutModel 
    //  Grid-styled Layout Model, which specifies rows, columns, percentage sizes, and row/column spans
    public class GridLayoutModel : LayoutModel
    {
        public GridLayoutModel() : base() { }
        public GridLayoutModel(string name) : base(name) { }
        public GridLayoutModel(string name, ushort id) : base(name, id) { }
        public GridLayoutModel(ushort version, string name, ushort id, byte[] data) : base(name, id)
        {
            if (version == c_latestVersion)
            {
                Reload(data);
            }
        }
        // Rows - number of rows in the Grid
        public int Rows
        {
            get { return _rows; }
            set
            {
                if (_rows != value)
                {
                    _rows = value;
                    FirePropertyChanged("Rows");
                }
            }
        }
        private int _rows = 1;

        // Columns - number of columns in the Grid
        public int Columns
        {
            get { return _cols; }
            set
            {
                if (_cols != value)
                {
                    _cols = value;
                    FirePropertyChanged("Columns");
                }
            }
        }
        private int _cols = 1;

        // CellChildMap - represents which "children" belong in which grid cells; 
        //  shows spanning children by the same index appearing in adjacent cells
        //  TODO: ideally no setter here - this means moving logic like "split" over to model
        public int[,] CellChildMap { get { return _cellChildMap; } set { _cellChildMap = value; } }
        private int[,] _cellChildMap;

        // RowPercents - represents the %age height of each row in the grid
        public int[] RowPercents { get { return _rowPercents; } set { _rowPercents = value; } }
        private int[] _rowPercents;

        // ColumnPercents - represents the %age width of each column in the grid
        public int[] ColumnPercents { get { return _colPercents; } set { _colPercents = value; } }
        private int[] _colPercents;

        // FreeZones (not persisted) - used to keep track of child indices that are no longer in use in the CellChildMap,
        //  making them candidates for re-use when it's needed to add another child
        //  TODO: do I need FreeZones on the data model?  - I think I do
        public IList<int> FreeZones { get { return _freeZones; } } 
        private IList<int> _freeZones = new List<int>();

        public void Reload(byte[] data)
        {
            // Skip version (2 bytes), id (2 bytes), and type (1 bytes)
            int i = 5;

            Rows = data[i++];
            Columns = data[i++];

            _rowPercents = new int[Rows];
            for (int row = 0; row < Rows; row++)
            {
                _rowPercents[row] = data[i++]*256 + data[i++];
            }

            _colPercents = new int[Columns];
            for (int col = 0; col < Columns; col++)
            {
                _colPercents[col] = data[i++]*256 + data[i++];
            }

            _cellChildMap = new int[Rows, Columns];
            for (int row = 0; row < Rows; row++)
            {
                for (int col = 0; col < Columns; col++)
                {
                    _cellChildMap[row, col] = data[i++];
                }
            }
        }

        // Clone
        //  Implements the LayoutModel.Clone abstract method
        //  Clones the data from this GridLayoutModel to a new GridLayoutModel
        public override LayoutModel Clone()
        {
            GridLayoutModel layout = new GridLayoutModel(Name);
            int rows = Rows;
            int cols = Columns;

            layout.Rows = rows;
            layout.Columns = cols;

            int[,] cellChildMap = new int[rows, cols];
            for (int row = 0; row < rows; row++)
            {
                for (int col = 0; col < cols; col++)
                {
                    cellChildMap[row, col] = CellChildMap[row, col];
                }
            }
            layout.CellChildMap = cellChildMap;

            int[] rowPercents = new int[rows];
            for (int row = 0; row < rows; row++)
            {
                rowPercents[row] = RowPercents[row];
            }
            layout.RowPercents = rowPercents;

            int[] colPercents = new int[cols];
            for (int col = 0; col < cols; col++)
            {
                colPercents[col] = ColumnPercents[col];
            }
            layout.ColumnPercents = colPercents;

            return layout;
        }

        // GetPersistData
        //  Implements the LayoutModel.GetPersistData abstract method
        //  Returns the state of this GridLayoutModel in persisted format
        protected override void PersistData()
        {
            int rows = Rows;
            int cols = Columns;

            int[,] cellChildMap;

            if (_freeZones.Count == 0)
            {
                // no unused indices -- so we can just use the _cellChildMap as is
                cellChildMap = _cellChildMap;
            }
            else
            {
                // compress cellChildMap to not have gaps for unused child indices;
                List<int> mapping = new List<int>();

                cellChildMap = new int[rows, cols];

                for (int row = 0; row < rows; row++)
                {
                    for (int col = 0; col < cols; col++)
                    {
                        int source = _cellChildMap[row, col];

                        int index = mapping.IndexOf(source);
                        if (index == -1)
                        {
                            index = mapping.Count;
                            mapping.Add(source);
                        }
                        cellChildMap[row, col] = index;
                    }
                }
            }

            FileStream outputStream = File.Open(Settings.AppliedZoneSetTmpFile, FileMode.Create);
            using (var writer = new Utf8JsonWriter(outputStream, options: default))
            {
                writer.WriteStartObject();
                writer.WriteString("uuid", Id.ToString());
                writer.WriteString("name", Name);

                writer.WriteString("type", "grid");
                
                writer.WriteStartObject("info");
                
                writer.WriteNumber("rows", rows);
                writer.WriteNumber("columns", cols);
                
                writer.WriteStartArray("rows-percentage");
                for (int row = 0; row < Rows; row++)
                {
                    writer.WriteNumberValue(_rowPercents[row]);
                }
                writer.WriteEndArray();

                writer.WriteStartArray("columns-percentage");
                for (int col = 0; col < Columns; col++)
                {
                    writer.WriteNumberValue(_colPercents[col]);
                }
                writer.WriteEndArray();

                writer.WriteStartArray("cell-child-map");
                for (int row = 0; row < Rows; row++)
                {
                    writer.WriteStartArray();
                    for (int col = 0; col < Columns; col++)
                    {
                        writer.WriteNumberValue(_cellChildMap[row, col]);
                    }
                    writer.WriteEndArray();
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

        private static ushort c_latestVersion = 0;
    }
}
