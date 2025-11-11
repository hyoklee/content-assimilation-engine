#!/usr/bin/env python3
# Modified script to work with mock ODB files (uses pickle instead of Abaqus odbAccess)
# This script exports Element Set, Node Set and Nodal results from mock ODB to HDF5 file.

import pickle
import h5py
import numpy as np
from mock_odb_classes import *

print("Loading mock ODB file: indentation_axi1.odb")

# Open the mock Abaqus ODB (pickle file)
with open("indentation_axi1.odb", "rb") as f:
    odb = pickle.load(f)

print(f"ODB loaded: {odb.name}")

# Create a new hdf5 file
k = h5py.File("nava.hdf5", "w")
print("Created HDF5 file: nava.hdf5")

# Create groups to match the hierarchy of the odb or as desired
grp1k = k.create_group("NodesK")
grp2k = k.create_group("ElementsK")
grp3k = k.create_group("ResultsK")

# Create datasets associated with these groups
dataset1k = grp2k.create_dataset("elemConnK", (400, 4), dtype="i")
dataset2k = grp1k.create_dataset("nodeCoorK", (400, 3), dtype="f")
dataset3k = grp3k.create_dataset("resultsK", (400, 1))

# Get elements and nodes from the mock ODB
elements = odb.rootAssembly.instances["I_INDENTER"].elementSets["ALL_ELEMENTS"].elements
nodes = odb.rootAssembly.instances["I_INDENTER"].nodeSets["ALL_NODES"].nodes

print(f"Processing {len(elements)} elements...")

# Comprehend the Elements connectivity data from ODB into the hdf5 file
for i in range(min(len(elements), 400)):
    for j in range(len(elements[i].connectivity)):
        dataset1k[i, j] = elements[i].connectivity[j]
dataset1k.attrs["title"] = "Connectivity of elements"

print(f"Processing {len(nodes)} nodes...")

# Comprehend the Nodes coordinates data from ODB into the hdf5 file
for g in range(min(len(nodes), 400)):
    for l in range(len(nodes[g].coordinates)):
        dataset2k[g, l] = nodes[g].coordinates[l]
dataset2k.attrs["title"] = "Node coordinates"

print("Processing field outputs...")

# Comprehend displacement field output
displacement_values = odb.steps["LOADING"].frames[-1].fieldOutputs["U"].values
for node in range(min(len(nodes), 400)):
    dataset3k[node] = displacement_values[node].magnitude

dataset3k.attrs["title"] = "Displacement magnitude"

# Close the HDF5 file
k.close()

print("\nSuccess! Data exported to nava.hdf5")
print("Groups created: NodesK, ElementsK, ResultsK")
print(f"- Elements: {len(elements)} with connectivity data")
print(f"- Nodes: {len(nodes)} with coordinate data")
print(f"- Results: Displacement field output")
