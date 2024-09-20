import pyarrow as pa
import pyarrow.ipc as ipc
import os
import ctypes
import numpy as np

SHM_NAME = "/dev/shm/arrow_shm"

class Metadata(ctypes.Structure):
    _fields_ = [
        ("num_tables", ctypes.c_int),
        ("table_size_bytes", ctypes.c_int64 * 1)  # Flexible array member
    ]

def read_table_from_buffer(buffer):
    with pa.ipc.open_stream(buffer) as reader:
        return reader.read_all()

def read_metadata(mmap):
    # Read the number of tables
    num_tables = np.frombuffer(mmap.read(ctypes.sizeof(ctypes.c_int)), dtype=np.int32)[0]
    
    # Define the full metadata structure
    class FullMetadata(ctypes.Structure):
        _fields_ = [
            ("num_tables", ctypes.c_int),
            ("table_size_bytes", ctypes.c_int64 * num_tables)
        ]
    
    # Calculate the full metadata size
    metadata_size = ctypes.sizeof(FullMetadata)
    
    # Seek back to the beginning and read the full metadata
    mmap.seek(0)
    metadata_buffer = mmap.read(metadata_size)
    
    return FullMetadata.from_buffer_copy(metadata_buffer)

def read_tables_from_shm():
    if not os.path.exists(SHM_NAME):
        raise FileNotFoundError(f"Shared memory segment {SHM_NAME} not found")

    with pa.memory_map(SHM_NAME, mode='r') as mmap:
        metadata = read_metadata(mmap)
        
        tables = []
        offset = ctypes.sizeof(Metadata) + (metadata.num_tables - 1) * ctypes.sizeof(ctypes.c_int64)
        
        for size in metadata.table_size_bytes:
            if size > 0:
                mmap.seek(offset)
                table_buffer = mmap.read(size)
                table = read_table_from_buffer(pa.py_buffer(table_buffer))
                tables.append(table)
                offset += size

    return tables

def print_table_info(table):
    print(f"Table shape: {table.num_rows} rows x {table.num_columns} columns")
    print("Schema:")
    print(table.schema)
    print("\nAll rows:")
    print(table.to_pandas())
    print("\n" + "="*50 + "\n")

if __name__ == "__main__":
    tables = read_tables_from_shm()
    print(f"Successfully read {len(tables)} tables from shared memory.")
    
    for i, table in enumerate(tables):
        print(f"Table {i+1}:")
        print_table_info(table)