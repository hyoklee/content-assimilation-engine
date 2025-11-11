#!/usr/bin/env python3
# Modified script to work with mock ODB files (uses pickle instead of Abaqus odbAccess)
# This script transfers material data from mock Abaqus ODB to HDF5 containers

import pickle
import h5py
import numpy as np
from mock_odb_classes import *

print("Loading mock ODB file: discbrake_sst_axi.odb")

# Open the mock Abaqus ODB (pickle file)
with open("discbrake_sst_axi.odb", "rb") as f:
    odb = pickle.load(f)

print(f"ODB loaded: {odb.name}")

# Create a new hdf5 file
k = h5py.File("mat.hdf5", "w")
print("Created HDF5 file: mat.hdf5")

# Create groups to match the hierarchy of the odb or as desired
grp1 = k.create_group("Material")
grp2 = grp1.create_group("Mechanical")
grp3 = grp2.create_group("Elastic")
grp4 = grp2.create_group("Plastic")
grp5 = grp2.create_group("Expansion")

# Create datasets associated with these groups
dataset1 = grp3.create_dataset("elasticity", (40, 3), dtype="f")
dataset2 = grp4.create_dataset("hardening", (40, 3), dtype="f")
dataset3 = grp5.create_dataset("expansion", (40, 2), dtype="f")

print("Processing material FONTE...")

# Comprehend the Elasticity data from ODB into the hdf5 file
for i, j in enumerate(odb.materials["FONTE"].elastic.table):
    dataset1[i] = j
dataset1.attrs["title"] = "Elasticity model"
print(f"- Elastic properties: {len(odb.materials['FONTE'].elastic.table)} rows")

# Comprehend the Plastic hardening data from ODB into the hdf5 file
for k_idx, l in enumerate(odb.materials["FONTE"].plastic.table):
    dataset2[k_idx] = l
dataset2.attrs["title"] = "Plasticity model"
print(f"- Plastic properties: {len(odb.materials['FONTE'].plastic.table)} rows")

# Comprehend the Expansion data from ODB into the hdf5 file
for m, n in enumerate(odb.materials["FONTE"].expansion.table):
    dataset3[m] = n
dataset3.attrs["title"] = "Expansion model"
print(f"- Expansion properties: {len(odb.materials['FONTE'].expansion.table)} rows")

# Close the HDF5 file
k.close()

print("\nSuccess! Material data exported to mat.hdf5")
print("Groups created: Material/Mechanical/{Elastic, Plastic, Expansion}")
print("- Elasticity: Young's modulus, Poisson's ratio, Temperature")
print("- Plasticity: Yield stress, Plastic strain, Temperature")
print("- Expansion: Thermal expansion coefficient, Temperature")
