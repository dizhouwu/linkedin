#include <arrow/api.h>
#include <arrow/io/api.h>
#include <arrow/ipc/api.h>
#include <arrow/result.h>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>
#include <cstring>
#include <vector>
#include <random>
#include <chrono>

const char* SHM_NAME = "arrow_shm";

struct Metadata {
    int num_tables;
    int64_t table_size_bytes[1];  // Flexible array member
};

void handle_status(const arrow::Status& status) {
    if (!status.ok()) {
        std::cerr << "Arrow error: " << status.ToString() << std::endl;
        exit(1);
    }
}

std::shared_ptr<arrow::Table> create_table(int num_rows, int64_t start_time) {
    arrow::Int64Builder timestamp_builder;
    arrow::DoubleBuilder price_builder;
    arrow::DoubleBuilder volume_builder;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(100.0, 200.0);
    std::uniform_real_distribution<> volume_dist(1000.0, 10000.0);

    for (int i = 0; i < num_rows; ++i) {
        handle_status(timestamp_builder.Append(start_time + i * 1000000)); // Increment by 1ms
        handle_status(price_builder.Append(price_dist(gen)));
        handle_status(volume_builder.Append(volume_dist(gen)));
    }

    std::shared_ptr<arrow::Array> timestamp_array;
    std::shared_ptr<arrow::Array> price_array;
    std::shared_ptr<arrow::Array> volume_array;
    handle_status(timestamp_builder.Finish(&timestamp_array));
    handle_status(price_builder.Finish(&price_array));
    handle_status(volume_builder.Finish(&volume_array));

    auto schema = arrow::schema({
        arrow::field("timestamp_nano", arrow::int64()),
        arrow::field("price", arrow::float64()),
        arrow::field("volume", arrow::float64())
    });

    return arrow::Table::Make(schema, {timestamp_array, price_array, volume_array});
}

void write_table_to_buffer(const std::shared_ptr<arrow::Table>& table, char* buffer, int64_t& offset) {
    arrow::Result<std::shared_ptr<arrow::io::BufferOutputStream>> maybe_out_stream = 
        arrow::io::BufferOutputStream::Create(1024 * 1024, arrow::default_memory_pool());
    handle_status(maybe_out_stream.status());
    std::shared_ptr<arrow::io::BufferOutputStream> out_stream = maybe_out_stream.ValueOrDie();

    arrow::ipc::IpcWriteOptions options = arrow::ipc::IpcWriteOptions::Defaults();
    arrow::Result<std::shared_ptr<arrow::ipc::RecordBatchWriter>> maybe_writer = 
        arrow::ipc::MakeStreamWriter(out_stream.get(), table->schema(), options);
    handle_status(maybe_writer.status());
    std::shared_ptr<arrow::ipc::RecordBatchWriter> writer = maybe_writer.ValueOrDie();

    handle_status(writer->WriteTable(*table));
    handle_status(writer->Close());

    arrow::Result<std::shared_ptr<arrow::Buffer>> maybe_buf = out_stream->Finish();
    handle_status(maybe_buf.status());
    std::shared_ptr<arrow::Buffer> buf = maybe_buf.ValueOrDie();

    memcpy(buffer + offset, buf->data(), buf->size());
    offset += buf->size();
}

int main() {
    using namespace boost::interprocess;

    const int num_tables = 3;
    const std::vector<int> rows_per_table = {3, 4, 5};  // Variable number of rows

    // Calculate total size needed
    int64_t metadata_size = sizeof(Metadata) + (num_tables - 1) * sizeof(int64_t);
    int64_t data_size = num_tables * 1024 * 1024;  // Assume max 1MB per table
    int64_t total_size = metadata_size + data_size;

    // Create shared memory
    shared_memory_object::remove(SHM_NAME);
    shared_memory_object shm(create_only, SHM_NAME, read_write);
    shm.truncate(total_size);
    mapped_region region(shm, read_write);
    char* mem = static_cast<char*>(region.get_address());

    // Initialize metadata
    Metadata* metadata = reinterpret_cast<Metadata*>(mem);
    metadata->num_tables = num_tables;

    // Write tables
    char* data_buffer = mem + metadata_size;
    int64_t offset = 0;
    int64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    for (int i = 0; i < num_tables; ++i) {
        auto table = create_table(rows_per_table[i], current_time + i * 1000000000);
        int64_t start_offset = offset;
        write_table_to_buffer(table, data_buffer, offset);
        metadata->table_size_bytes[i] = offset - start_offset;
        std::cout << "Table " << i+1 << " size: " << metadata->table_size_bytes[i] << " bytes" << std::endl;
    }

    std::cout << "Wrote " << num_tables << " tables to shared memory." << std::endl;
    for (int i = 0; i < num_tables; ++i) {
        std::cout << "Table " << i+1 << " has " << rows_per_table[i] << " rows." << std::endl;
    }

    return 0;
}