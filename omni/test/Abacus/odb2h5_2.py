# This script transfers an Abaqus .mat file to HDF5 containers
from odb import *
import h5py
import numpy as np

# Open an existing Abaqus Odb
odb = openOdb("discbrake_sst_axi.odb")

# Create a new hdf5 file
k = h5py.File("mat.hdf5", "w")

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

# Comprehend the Elasticity data from Odb into the hdf5 file
for i, j in enumerate(odb.materials["FONTE"].elastic.table):
    dataset1[i] = j
dataset1.attrs["title"] = "Elasticity model"

# Comprehend the Nodes coordinates data from Odb into the hdf5 file
for k, l in enumerate(odb.materials["FONTE"].plastic.table):
    dataset2[k] = l
dataset2.attrs["title"] = "Plasticity model"

# Comprehend a field output request, using a list comprehension because Abaqus
# repository is not iterable.
for m, n in enumerate(odb.materials["FONTE"].expansion.table):
    dataset3[m] = n
dataset3.attrs["title"] = "Expansion model"
