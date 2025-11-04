# This script exports Element Set, Node Set and Nodal results from ODB to HDF5 file.
from odb import *
import h5py
import numpy as np

# Open an existing Abaqus Odb
odb = openOdb("indentation_axi1.odb")

# Create a new hdf5 file
k = h5py.File("nava.hdf5", "w")

# Create groups to match the hierarchy of the odb or as desired
grp1k = k.create_group("NodesK")
grp2k = k.create_group("ElementsK")
grp3k = k.create_group("ResultsK")

# Create datasets associated with these groups
dataset1k = grp2k.create_dataset("elemConnK", (400, 4), dtype="i")
dataset2k = grp1k.create_dataset("nodeCoorK", (400, 3), dtype="f")
dataset3k = grp3k.create_dataset("resultsK", (400, 1))

# Comprehend the Elements connectivity data from Odb into the hdf5 file
elements = odb.rootAssembly.instances["I_INDENTER"].elementSets["ALL_ELEMENTS"].elements
for i in range(0, np.size(elements)):
    for j in range(np.size(elements[i].connectivity)):
        dataset1k[i, j] = elements[i].connectivity[j]
    dataset1k.attrs["title"] = "Connectivity of elements"

# Comprehend the Nodes coordinates data from Odb into the hdf5 file
nodes = odb.rootAssembly.instances["I_INDENTER"].nodeSets["ALL_NODES"].nodes
for g in range(np.size(nodes)):
    for l in range(0, np.size(nodes[l].coordinates)):
        dataset2k[g, l] = nodes[g].coordinates[l]

# Comprehend a field output request, using a list comprehension because
# Abaqus repository is not iterable.
requestedOutputs = []
requestedOutputs = odb.steps["LOADING"].frames[-1].fieldOutputs.keys()
# for key in range(len(requestedOutputs)):
for node in range(np.size(nodes)):
    dataset3k[node] = (
        odb.steps["LOADING"].frames[-1].fieldOutputs["U"].values[node].magnitude
    )
