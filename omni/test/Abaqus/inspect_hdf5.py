#!/usr/bin/env python3
"""
Inspect the generated HDF5 files to verify their contents.
"""

import h5py
import numpy as np


def inspect_hdf5(filename):
    """Inspect and display HDF5 file structure and data"""
    print(f"\n{'='*60}")
    print(f"Inspecting: {filename}")
    print(f"{'='*60}")

    with h5py.File(filename, 'r') as f:

        def print_structure(name, obj):
            """Recursively print HDF5 structure"""
            indent = "  " * name.count('/')
            if isinstance(obj, h5py.Group):
                print(f"{indent}📁 Group: {name}")
            elif isinstance(obj, h5py.Dataset):
                print(f"{indent}📄 Dataset: {name}")
                print(f"{indent}   Shape: {obj.shape}, Dtype: {obj.dtype}")
                if 'title' in obj.attrs:
                    print(f"{indent}   Title: {obj.attrs['title']}")
                # Show sample data for small datasets
                if obj.size <= 10:
                    print(f"{indent}   Data: {obj[...]}")
                elif len(obj.shape) == 2 and obj.shape[0] <= 5:
                    print(f"{indent}   Sample (first 5 rows):")
                    for i, row in enumerate(obj[:5]):
                        print(f"{indent}     Row {i}: {row}")

        print("\nFile Structure:")
        f.visititems(print_structure)

        print(f"\n{'='*60}\n")


if __name__ == "__main__":
    inspect_hdf5("nava.hdf5")
    inspect_hdf5("mat.hdf5")
