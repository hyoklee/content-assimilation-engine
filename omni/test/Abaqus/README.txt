The following codes are extracted from

  HDF5: A new approach to Interoperability in Finite Element tools

by Anshuman S Bhadauria1.

1. odb2h5_1.py - This script exports Element Set, Node Set, and Nodal
  results from Abaqus ODB to HDF5. It creates groups for nodes, elements,
  and results, then transfers connectivity data, node coordinates, and field
   output data.
2. odb2h5_2.py - This script transfers Abaqus material (.mat) file data to
  HDF5 containers. It creates a hierarchical group structure for material
  properties (Mechanical → Elastic, Plastic, Expansion) and transfers
  elasticity, hardening, and expansion data.
