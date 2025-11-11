# Abaqus ODB to HDF5 Converter - Mock Implementation

This directory contains scripts for converting Abaqus ODB files to HDF5 format, along with a mock implementation for testing without Abaqus installation.

## Files

### Original Scripts (require Abaqus)
- `odb2h5_1.py` - Exports Element Set, Node Set, and Nodal results from `indentation_axi1.odb` to HDF5
- `odb2h5_2.py` - Exports material properties from `discbrake_sst_axi.odb` to HDF5

### Mock Implementation (works without Abaqus)
- `mock_odb_classes.py` - Mock classes that mimic the Abaqus ODB API structure
- `create_mock_odb.py` - Creates mock ODB files using pickle for testing
- `odb2h5_1_mock.py` - Modified version of odb2h5_1.py that works with mock ODB files
- `odb2h5_2_mock.py` - Modified version of odb2h5_2.py that works with mock ODB files
- `inspect_hdf5.py` - Utility to inspect generated HDF5 files

### Sample Input Files
- `indentation_axi.inp` - Abaqus input file for axisymmetric indentation simulation
- `indentation_axi1.inp` - Alternative indentation input file

## Usage

### With Mock ODB Files (No Abaqus Required)

1. **Create mock ODB files:**
   ```bash
   python3 create_mock_odb.py
   ```
   This creates:
   - `indentation_axi1.odb` - Mock ODB with 400 nodes, 400 elements, and displacement results
   - `discbrake_sst_axi.odb` - Mock ODB with material properties for FONTE material

2. **Convert to HDF5:**
   ```bash
   python3 odb2h5_1_mock.py  # Creates nava.hdf5
   python3 odb2h5_2_mock.py  # Creates mat.hdf5
   ```

3. **Inspect HDF5 files:**
   ```bash
   python3 inspect_hdf5.py
   ```

### With Real Abaqus ODB Files (Requires Abaqus)

1. Run Abaqus simulation to generate ODB files:
   ```bash
   abaqus job=indentation_axi1 input=indentation_axi1.inp
   abaqus job=discbrake_sst_axi input=discbrake_sst_axi.inp
   ```

2. Run conversion scripts using Abaqus Python:
   ```bash
   abaqus python odb2h5_1.py
   abaqus python odb2h5_2.py
   ```

## Output Files

### nava.hdf5 (from indentation simulation)
- **NodesK/nodeCoorK** - Node coordinates (400 × 3)
- **ElementsK/elemConnK** - Element connectivity (400 × 4)
- **ResultsK/resultsK** - Displacement magnitude results (400 × 1)

### mat.hdf5 (from disc brake simulation)
- **Material/Mechanical/Elastic/elasticity** - Elastic properties (40 × 3)
  - Young's modulus, Poisson's ratio, Temperature
- **Material/Mechanical/Plastic/hardening** - Plastic hardening (40 × 3)
  - Yield stress, Plastic strain, Temperature
- **Material/Mechanical/Expansion/expansion** - Thermal expansion (40 × 2)
  - Thermal expansion coefficient, Temperature

## Dependencies

```bash
pip3 install h5py numpy
```

## Notes

- Mock ODB files are created using Python's pickle module and mimic the Abaqus ODB API structure
- The mock implementation generates synthetic data for testing purposes
- Real ODB files require Abaqus installation and can only be created by running Abaqus simulations
- HDF5 format provides an open, portable alternative to proprietary ODB format
