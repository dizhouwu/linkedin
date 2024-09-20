import pyarrow as pa
import pyarrow.ipc as ipc
import mmap
import os
import pandas as pd
import gc
import ctypes

SHM_NAME = "arrow_shm"

class Metadata(ctypes.Structure):
    _fields_ = [
        ("num_tables", ctypes.c_int),
        ("table_size_bytes", ctypes.c_int64 * 1)  # Flexible array member
    ]

def read_table_from_buffer(buffer):
    with pa.ipc.open_stream(buffer) as reader:
        return reader.read_all()

def read_tables_from_shm():
    shm_path = f"/dev/shm/{SHM_NAME}"
    
    if not os.path.exists(shm_path):
        raise FileNotFoundError(f"Shared memory segment {SHM_NAME} not found")

    with open(shm_path, "rb") as f:
        with mmap.mmap(f.fileno(), 0, prot=mmap.PROT_READ) as mm:
            # Read the number of tables
            num_tables = ctypes.c_int.from_buffer_copy(mm[:ctypes.sizeof(ctypes.c_int)]).value

            # Calculate the full metadata size
            metadata_size = ctypes.sizeof(Metadata) + (num_tables - 1) * ctypes.sizeof(ctypes.c_int64)

            # Read the full metadata
            metadata_buffer = mm[:metadata_size]
            
            class FullMetadata(ctypes.Structure):
                _fields_ = [
                    ("num_tables", ctypes.c_int),
                    ("table_size_bytes", ctypes.c_int64 * num_tables)
                ]

            metadata = FullMetadata.from_buffer_copy(metadata_buffer)

            tables = []
            offset = metadata_size
            for i in range(num_tables):
                size = metadata.table_size_bytes[i]
                if size > 0:
                    table_buffer = memoryview(mm[offset:offset+size])
                    table = read_table_from_buffer(table_buffer)
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
