import pyarrow as pa
import pyarrow.ipc as ipc
import mmap
import os
import pandas as pd
import gc
import ctypes

NUM_TABLES = 3
ROWS_PER_TABLE = 10
SHM_NAME = "arrow_shm_ring"

class RingBufferMetadata(ctypes.Structure):
    _fields_ = [
        ("write_index", ctypes.c_int),
        ("read_index", ctypes.c_int),
        ("table_size_bytes", ctypes.c_int64 * NUM_TABLES)
    ]

def read_table_from_buffer(buffer, offset, size):
    reader = pa.ipc.open_stream(buffer[offset:offset+size])
    table = reader.read_all()
    return table

def read_tables_from_shm():
    shm_path = f"/dev/shm/{SHM_NAME}"
    
    if not os.path.exists(shm_path):
        raise FileNotFoundError(f"Shared memory segment {SHM_NAME} not found")

    with open(shm_path, "rb") as f:
        with mmap.mmap(f.fileno(), 0, prot=mmap.PROT_READ) as mm:
            metadata = RingBufferMetadata.from_buffer_copy(mm[:ctypes.sizeof(RingBufferMetadata)])
            buffer = memoryview(mm[ctypes.sizeof(RingBufferMetadata):])

            tables = []
            for i in range(NUM_TABLES):
                offset = i * 1024 * 1024  # Assuming 1MB per table as in C++
                size = metadata.table_size_bytes[i]
                table = read_table_from_buffer(buffer, offset, size)
                tables.append(table)

    return tables

def print_table_info(table):
    print(f"Table shape: {table.num_rows} rows x {table.num_columns} columns")
    print("Schema:")
    print(table.schema)
    print("\nFirst few rows:")
    print(table.to_pandas().head())
    print("\n" + "="*50 + "\n")

if __name__ == "__main__":
    tables = read_tables_from_shm()
    print(f"Successfully read {len(tables)} tables from shared memory ring buffer.")
    
    for i, table in enumerate(tables):
        print(f"Table {i+1}:")
        print_table_info(table)
        

