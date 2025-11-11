"""
Mock ODB classes that mimic the Abaqus ODB API structure.
These classes can be pickled and unpickled properly.
"""

import numpy as np


class MockNode:
    """Mock node object"""
    def __init__(self, label, coordinates):
        self.label = label
        self.coordinates = np.array(coordinates, dtype=float)


class MockElement:
    """Mock element object"""
    def __init__(self, label, connectivity):
        self.label = label
        self.connectivity = connectivity


class MockNodeSet:
    """Mock node set"""
    def __init__(self, name, nodes):
        self.name = name
        self.nodes = nodes


class MockElementSet:
    """Mock element set"""
    def __init__(self, name, elements):
        self.name = name
        self.elements = elements


class MockFieldValue:
    """Mock field output value"""
    def __init__(self, magnitude, data=None):
        self.magnitude = magnitude
        self.data = data if data is not None else [magnitude]


class MockFieldOutput:
    """Mock field output"""
    def __init__(self, name, values):
        self.name = name
        self.values = values


class MockFrame:
    """Mock frame object"""
    def __init__(self):
        self.fieldOutputs = {}

    def add_field_output(self, name, values):
        self.fieldOutputs[name] = MockFieldOutput(name, values)


class MockStep:
    """Mock step object"""
    def __init__(self, name):
        self.name = name
        self.frames = []

    def add_frame(self, frame):
        self.frames.append(frame)


class MockInstance:
    """Mock instance object"""
    def __init__(self, name):
        self.name = name
        self.nodeSets = {}
        self.elementSets = {}

    def add_node_set(self, name, nodes):
        self.nodeSets[name] = MockNodeSet(name, nodes)

    def add_element_set(self, name, elements):
        self.elementSets[name] = MockElementSet(name, elements)


class MockRootAssembly:
    """Mock root assembly"""
    def __init__(self):
        self.instances = {}

    def add_instance(self, name, instance):
        self.instances[name] = instance


class MockMaterial:
    """Mock material object"""
    def __init__(self, name):
        self.name = name
        self.elastic = None
        self.plastic = None
        self.expansion = None


class MockElastic:
    """Mock elastic properties"""
    def __init__(self, table):
        self.table = table


class MockPlastic:
    """Mock plastic properties"""
    def __init__(self, table):
        self.table = table


class MockExpansion:
    """Mock expansion properties"""
    def __init__(self, table):
        self.table = table


class MockOdb:
    """Mock ODB object that mimics Abaqus ODB structure"""
    def __init__(self, name):
        self.name = name
        self.rootAssembly = MockRootAssembly()
        self.steps = {}
        self.materials = {}

    def add_step(self, name, step):
        self.steps[name] = step

    def add_material(self, name, material):
        self.materials[name] = material
