#!/bin/bash
# Complete workflow: Create mock ODB files and convert to HDF5

echo "================================================"
echo "Abaqus ODB to HDF5 Conversion - Full Workflow"
echo "================================================"
echo ""

# Step 1: Create mock ODB files
echo "Step 1: Creating mock ODB files..."
python3 create_mock_odb.py
echo ""

# Step 2: Convert first ODB to HDF5
echo "Step 2: Converting indentation_axi1.odb to nava.hdf5..."
python3 odb2h5_1_mock.py
echo ""

# Step 3: Convert second ODB to HDF5
echo "Step 3: Converting discbrake_sst_axi.odb to mat.hdf5..."
python3 odb2h5_2_mock.py
echo ""

# Step 4: Inspect results
echo "Step 4: Inspecting generated HDF5 files..."
python3 inspect_hdf5.py
echo ""

# Summary
echo "================================================"
echo "Summary of Generated Files:"
echo "================================================"
ls -lh *.odb *.hdf5
echo ""
echo "Workflow completed successfully!"
