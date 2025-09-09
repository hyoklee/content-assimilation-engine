import numpy as np
import pandas as pd
import sys
import os
from multiprocessing import shared_memory

def process_shared_memory_to_parquet(shm_name, output_path, shape=None, dtype=np.float64):
    try:
        # Attach to the existing shared memory block
        shm = shared_memory.SharedMemory(name=shm_name)
        
        # Convert the buffer to a numpy array with the specified shape and dtype
        buffer = np.ndarray(shape=shape, dtype=dtype, buffer=shm.buf)
        
        # Create a copy of the data to avoid shared memory issues
        data = buffer.copy()
        
        # Convert to DataFrame (reshape if necessary for 1D arrays)
        if len(shape) == 1:
            df = pd.DataFrame(data, columns=['value'])
        else:
            df = pd.DataFrame(data)
        
        # Write DataFrame to Parquet file
        df.to_parquet(output_path, engine='pyarrow', index=False)
        
        # Close shared memory
        shm.close()
        
        return f"Successfully wrote {data.size} float64 values from buffer '{shm_name}' to '{output_path}'"
    
    except Exception as e:
        return f"Error: {str(e)}"
    finally:
        # Ensure shared memory is closed
        if 'shm' in locals():
            shm.close()

def main(input_file, output_file, shape=None, dtype=np.float64):
    try:
        # Derive shared memory name from input file (use base name without extension)
        shm_name = os.path.splitext(os.path.basename(input_file))[0]
        
        if output_file.startswith('s3://'):
            # Split by '/' and take the last part as the file name
            output_file_name = output_file.split('/')[-1]
        else:
            output_file_name = output_file
        
        # Read binary data from input file
        with open(input_file, 'rb') as f:
            binary_data = f.read()
        
        # Calculate number of elements based on data type size
        num_elements = len(binary_data) // np.dtype(dtype).itemsize
        
        # If shape is not provided, assume 1D array
        if shape is None:
            shape = (num_elements,)
        
        # Create shared memory block with the exact size needed
        shm = shared_memory.SharedMemory(
            create=True, 
            size=len(binary_data), 
            name=shm_name
        )
        
        # Copy binary data to shared memory
        shm.buf[:len(binary_data)] = binary_data
        
        # Process the shared memory data to Parquet
        result = process_shared_memory_to_parquet(
            shm_name=shm_name,
            output_path=output_file_name,
            shape=shape,
            dtype=dtype
        )
        print(result)
        
    except Exception as e:
        print(f"Error: {str(e)}", file=sys.stderr)
        sys.exit(1)
    finally:
        # Clean up shared memory
        if 'shm' in locals():
            shm.close()
            shm.unlink()

# Command-line execution
if __name__ == "__main__":
    if len(sys.argv) < 3 or len(sys.argv) > 5:
        print("Usage: python hdf5.py <input_binary_file> <output_parquet_file> [shape] [dtype]")
        print("  shape: Comma-separated dimensions (e.g., '1000,1000' for 1000x1000 matrix)")
        print("  dtype: Data type (default: float64)")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    
    # Parse shape if provided
    shape = None
    if len(sys.argv) >= 4 and sys.argv[3]:
        try:
            shape = tuple(map(int, sys.argv[3].split(',')))
        except ValueError:
            print(f"Error: Invalid shape format: {sys.argv[3]}")
            sys.exit(1)
    
    # Parse dtype if provided
    dtype = np.float64
    if len(sys.argv) >= 5:
        try:
            dtype = getattr(np, sys.argv[4])
        except AttributeError:
            print(f"Error: Unsupported data type: {sys.argv[4]}")
            sys.exit(1)
    
    main(input_file, output_file, shape, dtype)
