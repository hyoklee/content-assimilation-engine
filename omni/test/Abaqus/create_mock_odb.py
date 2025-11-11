#!/usr/bin/env python3
"""
Create mock ODB structures for testing without Abaqus installation.
This script creates mock objects that mimic the Abaqus ODB API structure.
"""

import pickle
import numpy as np
from mock_odb_classes import *


def create_indentation_odb():
    """Create mock ODB for indentation_axi1.odb"""
    print("Creating mock indentation_axi1.odb...")

    odb = MockOdb("indentation_axi1")

    # Create nodes for indenter (400 nodes as expected by script)
    nodes = []
    for i in range(400):
        # Generate simple coordinates in a grid pattern
        x = (i % 20) * 0.5
        y = (i // 20) * 0.5
        z = 0.0
        nodes.append(MockNode(i, [x, y, z]))

    # Create elements (400 elements with 4 nodes each - CAX4 elements)
    elements = []
    for i in range(400):
        # Simple quad connectivity pattern
        n1 = i
        n2 = (i + 1) % 400
        n3 = (i + 21) % 400
        n4 = (i + 20) % 400
        elements.append(MockElement(i, [n1, n2, n3, n4]))

    # Create instance
    instance = MockInstance("I_INDENTER")
    instance.add_node_set("ALL_NODES", nodes)
    instance.add_element_set("ALL_ELEMENTS", elements)

    # Add instance to assembly
    odb.rootAssembly.add_instance("I_INDENTER", instance)

    # Create step and frame
    step = MockStep("LOADING")
    frame = MockFrame()

    # Add displacement field output (U)
    displacement_values = []
    for i in range(400):
        # Generate some mock displacement values
        magnitude = np.random.uniform(0.0, 1.0)
        displacement_values.append(MockFieldValue(magnitude))

    frame.add_field_output("U", displacement_values)
    step.add_frame(frame)
    odb.add_step("LOADING", step)

    # Save as pickle file
    with open('indentation_axi1.odb', 'wb') as f:
        pickle.dump(odb, f)

    print(f"Created indentation_axi1.odb with {len(nodes)} nodes and {len(elements)} elements")
    return odb


def create_discbrake_odb():
    """Create mock ODB for discbrake_sst_axi.odb"""
    print("Creating mock discbrake_sst_axi.odb...")

    odb = MockOdb("discbrake_sst_axi")

    # Create material FONTE
    material = MockMaterial("FONTE")

    # Elastic properties (40 rows, 3 columns as expected)
    elastic_table = []
    for i in range(40):
        E = 200e9  # Young's modulus (Pa)
        nu = 0.3   # Poisson's ratio
        temp = 20.0 + i * 10.0  # Temperature
        elastic_table.append((E, nu, temp))
    material.elastic = MockElastic(elastic_table)

    # Plastic properties (40 rows, 3 columns)
    plastic_table = []
    for i in range(40):
        yield_stress = 250e6 + i * 1e6  # Yield stress (Pa)
        plastic_strain = 0.0 + i * 0.001  # Plastic strain
        temp = 20.0 + i * 10.0  # Temperature
        plastic_table.append((yield_stress, plastic_strain, temp))
    material.plastic = MockPlastic(plastic_table)

    # Expansion properties (40 rows, 2 columns)
    expansion_table = []
    for i in range(40):
        alpha = 1.2e-5  # Thermal expansion coefficient
        temp = 20.0 + i * 10.0  # Temperature
        expansion_table.append((alpha, temp))
    material.expansion = MockExpansion(expansion_table)

    odb.add_material("FONTE", material)

    # Save as pickle file
    with open('discbrake_sst_axi.odb', 'wb') as f:
        pickle.dump(odb, f)

    print(f"Created discbrake_sst_axi.odb with material FONTE")
    return odb


if __name__ == "__main__":
    # Create both mock ODB files
    create_indentation_odb()
    create_discbrake_odb()
    print("\nMock ODB files created successfully!")
    print("Note: These are pickle files that mimic the ODB structure.")
    print("The original scripts will need to be modified to use these mock files.")
