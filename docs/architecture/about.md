# High-Performance Log Ingestion & Analytics Engine

## Architecture Design Document (Version 1.0)


### Table of Contents

1. Introduction
2. System Goals and Core Design Principles
3. High-Level Architecture
4. Detailed Module Specifications
   - 4.1 Log Message (Data Model)
   - 4.2 Network Protocol
   - 4.3 Collector
   - 4.4 Queue
   - 4.5 Worker Pool
   - 4.6 Processing Engine
   - 4.7 Storage Engine
   - 4.8 Index Engine
   - 4.9 Metrics Engine
   - 4.10 Query Engine
   - 4.11 Dashboard
   - 4.12 Configuration Manager
   - 4.13 Internal Logger
5. Data Flow and Integration
6. Non-Functional Requirements
7. Future Extensions
8. Appendices

---

## 1. Introduction

### 1.1 Project Vision

Design and implement a high‑performance, modular, and extensible telemetry platform capable of receiving log events from thousands of distributed applications, processing those events in real time, storing them efficiently, generating operational metrics, and serving analytical queries with low latency.

The system emphasises:

- **High throughput:** handle millions of events per second.
- **Reliability:** no data loss, graceful degradation.
- **Scalability:** scale horizontally by adding workers or storage nodes.
- **Maintainability:** each component is isolated and replaceable.
- **Observability:** expose internal metrics for monitoring.
- **Modern C++:** leverage C++17/20 for performance and safety.

The architecture intentionally mirrors modern observability systems (e.g., Loki, Elasticsearch, Datadog) while remaining small enough for a single engineer to build incrementally.

### 1.2 Problem Statement

Modern software systems emit a continuous stream of operational events – user logins, database timeouts, memory warnings, sensor readings, network anomalies, etc. Without centralised collection and analysis, these events are siloed, making diagnosis, monitoring, and performance optimisation extremely difficult.

This system serves as the **central nervous system** for distributed applications, providing:

- Real‑time ingestion and normalisation.
- Durable storage for historical analysis.
- Fast search and filtering.
- Live dashboards and alerting.

### 1.3 Example Scenario

Imagine a smart manufacturing facility with diverse machines – robot arms, packaging lines, vision systems, temperature controllers, AGVs, and power units. Each device emits structured telemetry.

**Robot Arm**:

```json
{
  "service": "robot_arm",
  "level": "INFO",
  "message": "Cycle Completed",
  "duration_ms": 82
}
```

**Temperature Controller**:

```json
{
  "service": "temperature",
  "level": "WARN",
  "message": "Temperature High",
  "temperature": 91
}
```

**Warehouse AGV**:

```json
{
  "service": "agv",
  "level": "ERROR",
  "message": "Obstacle Detected"
}
```

All telemetry flows into the engine, where it is processed, stored, indexed, and made queryable. An operator can then search for all errors from the AGV in the last hour, view real‑time throughput, or set an alert when the temperature exceeds a threshold.

---

## 2. System Goals and Core Design Principles

### 2.1 System Goals

The system shall:

✔ Accept network connections from clients (TCP).  
✔ Receive structured log events (JSON or binary).  
✔ Validate and normalise incoming messages.  
✔ Queue incoming events to decouple ingestion from processing.  
✔ Process events asynchronously via a worker pool.  
✔ Store events durably on disk with rotation.  
✔ Index searchable fields (e.g., service, level) for fast lookups.  
✔ Generate real‑time metrics (counts, latency, service breakdown).  
✔ Answer search queries with filters and sorting.  
✔ Stream live updates to dashboards.  
✔ Provide a web‑based or CLI dashboard.

### 2.2 Core Design Principles

**Single Responsibility** – Every module performs exactly one task.  
**Loose Coupling** – Modules communicate only through well‑defined interfaces; implementation details are hidden.  
**Replaceable Components** – The Queue, Storage, or Index can be swapped with alternative implementations without impacting other modules.  
**Independent Testability** – Every module can be unit‑tested in isolation using mocks or stubs.  
**Observability by Design** – Each module exposes internal metrics (queue depth, latency, error counts) for monitoring.

---

## 3. High-Level Architecture

The system is structured as a linear pipeline with supporting services.

```text
+--------------+      +------------+      +-----------+      +----------------+
| Applications | ---> | Collector  | ---> |  Queue    | ---> | Worker Pool    |
+--------------+      +------------+      +-----------+      +----------------+
                                                                    |
                                                                    v
                                                         +-------------------+
                                                         | Processing Engine |
                                                         +-------------------+
                                                                    |
                                      +---------------------------+---------------------------+
                                      |                           |                           |
                                      v                           v                           v
                               +--------------+          +-------------+          +-------------------+
                               | Storage      |          | Index       |          | Metrics Engine    |
                               | Engine       |          | Engine      |          |                   |
                               +--------------+          +-------------+          +-------------------+
                                      |                           |                           |
                                      +---------------------------+---------------------------+
                                                                    |
                                                                    v
                                                         +-------------------+
                                                         |  Query Engine     |
                                                         +-------------------+
                                                                    |
                                                                    v
                                                         +-------------------+
                                                         |   Dashboard / CLI |
                                                         +-------------------+
```

**Supporting modules** (not shown): Configuration Manager and Internal Logger, which are used by all components.

---

## 4. Detailed Module Specifications

Each module is described with:

- **Purpose** – its core responsibility.
- **Stage** – where it fits in the pipeline.
- **Data Handling** – how it transforms, stores, or forwards data.
- **Importance** – why this module is essential.
- **Interfaces** – public methods, inputs, outputs, and error conditions.
- **Scenarios** – typical usage, edge cases, and performance considerations.

---

### 4.1 Log Message (Data Model)

**Purpose**  
Represents an immutable telemetry event. It is the **currency** of the entire system – every module either produces, consumes, or transforms a `LogMessage`.

**Stage**  
Foundation – used throughout the pipeline.

**Data Handling**  
- Holds raw and parsed fields (timestamp, service, level, message, optional metadata).
- Provides serialisation to/from JSON and binary formats.
- Performs basic validation (required fields, type checking).

**Importance**  
A consistent data model ensures that all modules agree on the structure of a log event. Changes to the model are isolated to this module, minimising ripple effects.

**Interfaces** (C++‑style):

```cpp
class LogMessage {
public:
    // Constructors
    LogMessage() = default;
    LogMessage(std::string service, std::string level, std::string message,
               std::chrono::system_clock::time_point timestamp,
               std::unordered_map<std::string, std::string> metadata = {});

    // Serialisation
    std::string toJSON() const;
    static LogMessage fromJSON(const std::string& json);

    // Binary serialisation (for efficient internal transfer)
    std::vector<uint8_t> toBinary() const;
    static LogMessage fromBinary(const std::vector<uint8_t>& data);

    // Validation
    bool isValid() const; // checks required fields and types

    // Getters (immutable)
    const std::string& service() const;
    const std::string& level() const;
    const std::string& message() const;
    auto timestamp() const;
    const auto& metadata() const;

private:
    std::string service_;
    std::string level_;
    std::string message_;
    std::chrono::system_clock::time_point timestamp_;
    std::unordered_map<std::string, std::string> metadata_;
};
```

**Scenarios**:

- **Normal flow**: A client sends a JSON log; the Collector calls `LogMessage::fromJSON()` to create an object, then passes it downstream.
- **Validation failure**: If `isValid()` returns false, the message is rejected and an error is logged.
- **Binary efficiency**: For high‑throughput internal queues, the Collector may serialise to binary to reduce copying.

---

### 4.2 Network Protocol

**Purpose**  
Defines the wire format and handshake used between clients and the Collector. Abstracts the details of the network layer so that the Collector can support multiple protocols (JSON, Protocol Buffers, etc.) without changing its internal logic.

**Stage**  
Ingestion boundary (between applications and Collector).

**Data Handling**  
- Parses incoming byte streams into complete log messages (e.g., newline‑delimited JSON).
- Optionally handles framing (e.g., length‑prefixed messages).
- May support authentication or compression in the future.

**Importance**  
Decouples the Collector from specific wire formats, allowing the system to evolve its protocol independently. It also provides a single point for protocol upgrades.

**Interfaces**:

```cpp
class NetworkProtocol {
public:
    virtual ~NetworkProtocol() = default;

    // Parse a chunk of bytes; returns a vector of complete LogMessage objects
    // (incomplete data is buffered internally)
    virtual std::vector<LogMessage> parse(const std::vector<uint8_t>& data) = 0;

    // Reset internal state (e.g., on disconnect)
    virtual void reset() = 0;

    // Optionally, generate a response to the client (e.g., acknowledgment)
    virtual std::vector<uint8_t> makeAck(const LogMessage& msg) = 0;
};
```

Concrete implementations: `JSONProtocol`, `ProtobufProtocol`, etc.

**Scenarios**:

- **JSON newline‑delimited**: The parser splits on newline characters, deserialises each line to JSON, and returns `LogMessage` objects.
- **Incomplete data**: If a partial JSON object is received, the parser buffers it and waits for more data.
- **Large messages**: The protocol may enforce a maximum size to prevent DoS.

---

### 4.3 Collector

**Purpose**  
Accept incoming TCP connections, read data from them, and convert raw bytes into `LogMessage` objects using the configured `NetworkProtocol`. It then pushes valid messages into the `Queue`.

**Stage**  
Ingestion entry point (first stage after network).

**Data Handling**  
- Listens on a TCP port.
- For each connection, reads data asynchronously (non‑blocking or using an event loop).
- Passes each chunk to the `NetworkProtocol` to extract complete messages.
- Validates each message (calls `LogMessage::isValid()`); discards invalid ones.
- Pushes valid messages into the `Queue` (non‑blocking, with backpressure handling).

**Importance**  
The Collector is the face of the system to external clients. It must be robust, handle many concurrent connections, and never block the pipeline.

**Interfaces**:

```cpp
class Collector {
public:
    Collector(Queue& queue, std::unique_ptr<NetworkProtocol> protocol,
              const Config& config);
    ~Collector();

    // Start listening and accepting connections
    void start();

    // Gracefully shut down (close listening socket and all active connections)
    void stop();

private:
    void acceptLoop();
    void handleConnection(int client_fd);
    void processData(int client_fd, const std::vector<uint8_t>& data);
};
```

**Scenarios**:

- **Normal high‑load**: Thousands of clients connect and send logs continuously. The Collector uses epoll (Linux) or IOCP (Windows) to handle many connections efficiently.
- **Client disconnect**: The connection is closed gracefully; any partially received data is discarded.
- **Queue full**: The Collector may block or drop messages (configurable) to avoid overwhelming the system.
- **Malformed data**: The protocol parser may throw; the Collector logs the error and continues.

---

### 4.4 Queue

**Purpose**  
A thread‑safe FIFO buffer that decouples the fast network ingestion from the slower processing pipeline. It provides backpressure and allows the system to absorb bursts.

**Stage**  
Between Collector and Worker Pool.

**Data Handling**  
- Stores `LogMessage` objects (or binary representations).
- Supports non‑blocking push (with timeout) and blocking pop (for workers).
- Tracks queue depth and exposes it as a metric.
- May support multiple queues (e.g., by priority or service) for quality‑of‑service.

**Importance**  
The Queue is the **shock absorber** of the system. Without it, a spike in traffic would overwhelm the processing stage, leading to data loss. It also enables the Collector to operate independently of worker availability.

**Interfaces**:

```cpp
class Queue {
public:
    Queue(size_t max_size);

    // Push a message; returns true if successful, false if queue is full (non‑blocking)
    bool push(const LogMessage& msg);
    bool push(LogMessage&& msg); // move version

    // Pop a message (blocks until one is available or timeout)
    bool pop(LogMessage& out, std::chrono::milliseconds timeout = std::chrono::milliseconds::max());

    // Number of messages currently in the queue
    size_t size() const;

    // Whether the queue is empty
    bool empty() const;

    // Clear the queue (e.g., on shutdown)
    void clear();

private:
    std::queue<LogMessage> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t max_size_;
};
```

**Scenarios**:

- **Normal operation**: Workers pop messages as soon as they are pushed, keeping the queue shallow.
- **Burst handling**: During a sudden spike, the queue fills up. The Collector may start to block or drop, but no messages are lost inside the system.
- **Shutdown**: When the system is stopped, the queue is drained gracefully.
- **High contention**: The implementation may use a lock‑free queue for better performance.

---

### 4.5 Worker Pool

**Purpose**  
Manages a fixed number of worker threads that consume messages from the `Queue` and pass them to the `Processing Engine`. It ensures parallel processing and efficient CPU utilisation.

**Stage**  
Between Queue and Processing Engine.

**Data Handling**  
- Spawns N threads on startup.
- Each thread continuously pops a message from the queue.
- For each message, it invokes the `ProcessingEngine::process()` method.
- Handles errors (e.g., if processing throws, the worker logs and continues).
- Shuts down cleanly: waits for all enqueued messages to be processed before exiting.

**Importance**  
The Worker Pool converts the asynchronous ingestion into parallel, CPU‑bound work. By controlling the number of workers, we can tune the system for different hardware (e.g., number of cores).

**Interfaces**:

```cpp
class WorkerPool {
public:
    WorkerPool(Queue& queue, ProcessingEngine& engine, size_t num_workers);
    ~WorkerPool();

    // Start all worker threads
    void start();

    // Request all workers to finish after processing remaining messages
    void stop();

    // Wait for all workers to terminate
    void join();

private:
    void workerLoop();
    std::vector<std::thread> workers_;
    std::atomic<bool> running_;
};
```

**Scenarios**:

- **Normal load**: Workers are busy processing, queue stays balanced.
- **Idle**: If the queue is empty, workers block on `pop()`, consuming no CPU.
- **Worker failure**: If a worker throws an unhandled exception, the pool may restart it or log and continue.
- **Graceful shutdown**: `stop()` signals workers to finish the current message and then exit. The main thread calls `join()` to wait.

---

### 4.6 Processing Engine

**Purpose**  
Apply transformations, validation, enrichment, and normalisation to each log message before it is stored or forwarded.

**Stage**  
After the worker pool, before distribution to Storage, Index, and Metrics.

**Data Handling**  
- Accepts a raw `LogMessage`.
- Applies a configurable pipeline of processors (e.g., timestamp normalisation, field type conversion, hostname lookup, field extraction).
- Produces a `ProcessedLog` which may have additional fields (e.g., `normalized_level`, `host`, `geoip`) and a standardised timestamp.
- Passes the processed log to the next stages: Storage, Index, Metrics.

**Importance**  
Raw logs often contain inconsistent fields, missing timestamps, or non‑standard severity levels. The Processing Engine cleans and enriches them so that downstream modules (query, metrics) work on a consistent dataset.

**Interfaces**:

```cpp
class ProcessingEngine {
public:
    ProcessingEngine(const Config& config);

    // Process a single log; returns a ProcessedLog
    ProcessedLog process(const LogMessage& raw);

    // Register custom processors
    void addProcessor(std::unique_ptr<Processor> processor);

private:
    std::vector<std::unique_ptr<Processor>> processors_;
};

// Example processor interface
class Processor {
public:
    virtual ~Processor() = default;
    virtual ProcessedLog apply(const LogMessage& raw, ProcessedLog& processed) = 0;
};
```

`ProcessedLog` is an extension of `LogMessage` with extra fields (e.g., `normalized_level`, `hostname`, `geolocation`).

**Scenarios**:

- **Timestamp normalisation**: Convert various date formats (ISO8601, UNIX epoch) to a common UTC timestamp.
- **Severity mapping**: Map "WARN" to 3, "ERROR" to 4, etc., for metric aggregation.
- **Hostname resolution**: Extract source IP and perform a reverse DNS lookup (cached) to populate `host`.
- **Field extraction**: Parse a JSON‑formatted `message` field to extract additional structured data (e.g., `duration_ms`).

---

### 4.7 Storage Engine

**Purpose**  
Persist processed log events durably to disk. Manage log rotation, compression, and retrieval of raw logs by offset.

**Stage**  
After processing; receives `ProcessedLog` objects.

**Data Handling**  
- Appends each log to an active segment file (e.g., `.log`).
- Rotates files based on size or time (e.g., every 1 GB or hourly).
- Optionally compresses old segments (gzip, LZ4).
- Maintains a **segment manifest** that maps segment files to their time range and offsets.
- Provides a method to read a log entry given a segment ID and offset.

**Importance**  
Persistent storage ensures that no data is lost and enables historical queries. Without it, the system would only be a live streaming tool.

**Interfaces**:

```cpp
class StorageEngine {
public:
    StorageEngine(const std::string& data_dir, const Config& config);

    // Append a processed log; returns a StorageLocation (segment_id, offset)
    StorageLocation append(const ProcessedLog& log);

    // Read a log by its location
    ProcessedLog read(const StorageLocation& loc);

    // Force flush to disk
    void flush();

    // Rotate segments manually (or automatically)
    void rotate();

    // List all segments (for recovery)
    std::vector<SegmentInfo> listSegments() const;

private:
    std::string data_dir_;
    std::ofstream current_file_;
    uint64_t current_segment_id_;
    size_t current_offset_;
};
```

`StorageLocation` is a lightweight struct { uint64_t segment_id; size_t offset; }.

**Scenarios**:

- **Normal writes**: Logs are appended quickly; `append()` returns a location that the Index Engine can store.
- **Rotation**: When the current segment reaches size limit, it is closed, optionally compressed, and a new segment is opened.
- **Recovery**: On startup, the engine scans the data directory to recover existing segments and find the next available offset.
- **Read**: The Query Engine calls `read(loc)` to load the raw log for display.

---

### 4.8 Index Engine

**Purpose**  
Maintain inverted indexes over log fields to accelerate search queries. For each indexed field (e.g., `service`, `level`), it builds a map from field value to a list of `StorageLocation`s.

**Stage**  
After processing; receives `ProcessedLog` objects. Also used by Query Engine for lookups.

**Data Handling**  
- On receiving a `ProcessedLog`, it extracts configured fields (e.g., `service`, `level`, `host`).
- For each field‑value pair, it appends the log's `StorageLocation` to the corresponding posting list.
- Posting lists are stored in memory (for speed) and periodically flushed to disk (for durability).
- Supports range queries (e.g., timestamp) via time‑partitioned indexes.

**Importance**  
Searching through billions of logs without an index would require scanning all storage, which is impractical. The Index Engine makes queries **fast** (O(log N) or O(1) for exact matches).

**Interfaces**:

```cpp
class IndexEngine {
public:
    IndexEngine(const Config& config);

    // Index a new log
    void index(const ProcessedLog& log, const StorageLocation& loc);

    // Search for logs matching a field value
    std::vector<StorageLocation> search(const std::string& field, const std::string& value) const;

    // Advanced: combine multiple conditions (AND/OR)
    std::vector<StorageLocation> search(const Query& query) const;

    // Flush in‑memory index to disk (periodic)
    void flush();

    // Rebuild index from storage (recovery)
    void rebuild(const StorageEngine& storage);

private:
    // In‑memory posting lists: field -> value -> vector<StorageLocation>
    std::unordered_map<std::string, std::unordered_map<std::string, std::vector<StorageLocation>>> index_;
};
```

**Scenarios**:

- **Indexing**: As logs are processed, the Index Engine is updated. This happens asynchronously to not block the main pipeline.
- **Exact match search**: Query "service=robot_arm" returns all locations of robot arm logs.
- **Range query**: "timestamp > now - 1h" can be handled by combining index with time‑partitioned segments.
- **Memory pressure**: To limit memory usage, the index may only keep recent logs in memory; older logs are indexed on disk (using a separate disk‑based index).

---

### 4.9 Metrics Engine

**Purpose**  
Continuously compute operational statistics from the log stream. These include throughput, error rates, service‑level aggregations, and latency percentiles.

**Stage**  
After processing; receives `ProcessedLog` objects.

**Data Handling**  
- Maintains a set of counters, gauges, and histograms.
- For each log, it updates metrics based on fields (e.g., increment counter for level, service).
- Periodically (e.g., every 5 seconds) generates snapshots that can be exported to monitoring systems (Prometheus, Graphite) or displayed on the Dashboard.
- Supports resetting (for counters) and sliding windows.

**Importance**  
Metrics provide real‑time visibility into system health and performance. They are essential for alerting and capacity planning.

**Interfaces**:

```cpp
class MetricsEngine {
public:
    MetricsEngine(const Config& config);

    // Update metrics from a processed log
    void update(const ProcessedLog& log);

    // Snapshot all metrics (for export or display)
    MetricsSnapshot snapshot() const;

    // Reset counters (e.g., for rate calculations)
    void reset();

private:
    std::unordered_map<std::string, std::atomic<uint64_t>> counters_;
    std::unordered_map<std::string, std::atomic<double>> gauges_;
    // Histograms: service latency, etc.
};
```

**Scenarios**:

- **Counting errors**: Each ERROR log increments the `errors.total` counter and the `errors.by_service[service]` counter.
- **Throughput**: A gauge tracks the number of logs per second (computed over a sliding window).
- **Latency**: If a log contains `duration_ms`, the Metrics Engine updates a histogram to compute p50, p95, p99.
- **Export**: Every 10 seconds, the snapshot is pushed to a Prometheus endpoint.

---

### 4.10 Query Engine

**Purpose**  
Answer user search queries by combining indexes and raw storage. It handles filters, sorting, and pagination.

**Stage**  
After storage and indexing; serves the Dashboard/CLI.

**Data Handling**  
- Accepts a `Query` object specifying fields, values, time range, etc.
- Uses the Index Engine to obtain candidate `StorageLocation`s.
- For each location, reads the full log from the Storage Engine.
- Applies additional filters (e.g., regex on message) that weren't indexed.
- Sorts the results (by timestamp) and returns a page of results.

**Importance**  
The Query Engine is the user‑facing component that makes the stored data useful. Its performance directly impacts user experience.

**Interfaces**:

```cpp
class QueryEngine {
public:
    QueryEngine(const StorageEngine& storage, const IndexEngine& index);

    // Execute a query; returns a page of results
    QueryResult search(const Query& query);

    // Stream live logs (tail) – discussed later
    void tail(std::function<void(const ProcessedLog&)> callback);

private:
    const StorageEngine& storage_;
    const IndexEngine& index_;
};

struct Query {
    std::unordered_map<std::string, std::string> field_eq; // exact matches
    std::string time_range; // e.g., "last 1h"
    std::string message_filter; // regex
    size_t limit;
    size_t offset;
    SortOrder sort; // asc/desc by timestamp
};

struct QueryResult {
    std::vector<ProcessedLog> logs;
    size_t total_hits; // for pagination
};
```

**Scenarios**:

- **Simple search**: `service=robot_arm` returns all logs from that service.
- **Combined search**: `level=ERROR AND service=agv` – the engine first gets intersection of posting lists.
- **Full‑text search**: `message contains "timeout"` – after filtering by index, it scans the message field (costly, but acceptable if the candidate set is small).
- **Pagination**: The query includes `offset` and `limit`; the engine fetches only the required slice.
- **Live tail**: The Query Engine subscribes to new logs from the processing pipeline and streams them to the Dashboard.

---

### 4.11 Dashboard

**Purpose**  
Provide a graphical user interface (web or terminal) for visualizing metrics, searching logs, and viewing live streams.

**Stage**  
User interface layer; uses both Query Engine and Metrics Engine.

**Data Handling**  
- Fetches metrics snapshots periodically to update charts.
- Sends search requests to the Query Engine and displays results in a table.
- Opens a WebSocket or SSE stream to receive live logs.
- Allows users to filter by service, level, time, etc.

**Importance**  
The Dashboard is the primary interaction point for operators. It makes the system's data accessible and actionable.

**Interfaces** (simplified):

```cpp
class Dashboard {
public:
    Dashboard(QueryEngine& query_engine, MetricsEngine& metrics_engine);

    // Start the web server (or terminal UI)
    void start();

    // Render the current view
    void render();

    // Handle user input (search, filter)
    void handleInput(const std::string& command);

private:
    void updateMetrics();
    void updateLiveLogs();
};
```

In practice, the Dashboard would be a separate process (e.g., a Node.js server serving React, or a terminal UI using ncurses). The interface here is conceptual.

**Scenarios**:

- **Live view**: The Dashboard shows a real‑time stream of logs, color‑coded by level.
- **Metrics charts**: Displays throughput, error rate, top services over time.
- **Search**: User types "level=ERROR", Dashboard sends query, displays results.
- **Alerting**: If error rate exceeds threshold, the Dashboard highlights it or sends a notification.

---

### 4.12 Configuration Manager

**Purpose**  
Load and provide runtime configuration to all modules from a central file (`config.json`) or environment variables.

**Stage**  
Initialisation; used by all modules.

**Data Handling**  
- Parses a JSON configuration file.
- Provides getter methods for various settings (port, queue size, storage path, etc.).
- Supports hot‑reload (optional) – modules can listen for configuration changes.

**Importance**  
Centralises configuration, making it easy to adjust system behaviour without recompilation. It also helps avoid hard‑coded values.

**Interfaces**:

```cpp
class ConfigManager {
public:
    ConfigManager(const std::string& config_path);

    // Getters
    int collector_port() const;
    size_t queue_max_size() const;
    size_t worker_count() const;
    std::string storage_dir() const;
    std::vector<std::string> index_fields() const;
    // ... many more
};
```

**Scenarios**:

- **Startup**: The main function loads the configuration and passes it to each module's constructor.
- **Reconfiguration**: If the config file changes, the ConfigManager can notify listeners to apply new values (e.g., adjust queue size).

---

### 4.13 Internal Logger

**Purpose**  
Log internal events (startup, errors, warnings) from all modules for debugging and diagnostics. **The logging system needs its own logging** – a humorous but essential requirement.

**Stage**  
All stages – used by every module.

**Data Handling**  
- Provides a simple logging interface (info, warning, error, debug).
- Writes to a separate log file (or stdout) with timestamps.
- Supports different log levels (e.g., INFO, DEBUG).
- Thread‑safe.

**Importance**  
Without internal logging, diagnosing issues in the ingestion system itself would be nearly impossible. It is the system's **self‑observability**.

**Interfaces**:

```cpp
class Logger {
public:
    static Logger& getInstance(); // singleton

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator(Logger&&) = delete;

    void setLevel(Level level);

    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void debug(const std::string& msg);

private:
    void log(Level level, const std::string& msg);
};
```

**Scenarios**:

- **Startup**: Each module logs "Starting..." with its configuration.
- **Error**: If the Collector cannot bind to a port, it logs an error and exits.
- **Debug**: During development, debug logs detail every message received.
- **Crash**: If an unhandled exception occurs, the logger writes the stack trace (if available) to the log file.

---

## 5. Data Flow and Integration

The following sequence illustrates a complete end‑to‑end flow:

1. **Client** sends a JSON log over TCP.
2. **Collector** accepts the connection, reads data, and uses the **Network Protocol** to parse the JSON into a `LogMessage`.
3. The `LogMessage` is validated; if invalid, it is discarded (logged).
4. The Collector pushes the `LogMessage` into the **Queue**.
5. A **Worker** from the **Worker Pool** pops the message.
6. The Worker passes it to the **Processing Engine**, which normalises and enriches it, producing a `ProcessedLog`.
7. The `ProcessedLog` is sent concurrently to:
   - **Storage Engine** – appended to a segment file, returning a `StorageLocation`.
   - **Index Engine** – indexed by relevant fields, storing the location.
   - **Metrics Engine** – updates internal counters and histograms.
8. The **Query Engine** can later search using the Index and retrieve full logs from Storage.
9. The **Dashboard** queries the Query Engine and Metrics Engine to display data.
10. The **Internal Logger** records all significant events throughout.

All modules are loosely coupled through their interfaces; the main application composes them together.

---

## 6. Non-Functional Requirements

- **Throughput**: Must handle at least 1 million logs/sec on a commodity server (with proper tuning).
- **Latency**: End‑to‑end latency (from ingestion to storage) should be < 100 ms for 99th percentile.
- **Durability**: Once a log is acknowledged to the client (if we implement acks), it must be safely stored on disk.
- **Availability**: The system should be designed for 99.9% uptime (single node) with graceful degradation under overload.
- **Scalability**: Horizontal scaling by adding more worker nodes or sharding storage.
- **Security**: Support for TLS, authentication, and authorisation (future).

---

## 7. Future Extensions

- **Distributed deployment**: Split Collector, Queue (e.g., Kafka), and storage across multiple nodes.
- **Streaming analytics**: Complex event processing (CEP) for anomaly detection.
- **Alerting engine**: Rule‑based alerts triggered by metrics or log patterns.
- **More storage backends**: S3, HDFS, or a time‑series database.
- **Multi‑tenancy**: Isolate data and queries for different teams.

---

## 8. Appendices

### Appendix A: Data Structures

**LogMessage** – as defined in 4.1.

**ProcessedLog** – extends LogMessage with:
- `normalized_level` (int)
- `hostname` (string)
- `geoip` (optional)
- Extracted fields (e.g., `duration_ms`).

**StorageLocation** – { segment_id, offset }.

**Query** – as defined in 4.10.

**MetricsSnapshot** – a set of key‑value pairs and histograms.

### Appendix B: Configuration Example (`config.json`)

```json
{
  "collector": {
    "port": 8080,
    "max_connections": 10000
  },
  "queue": {
    "max_size": 500000
  },
  "workers": 8,
  "storage": {
    "data_dir": "/var/Zeus/data",
    "segment_size_mb": 1024,
    "compress": true
  },
  "index": {
    "fields": ["service", "level", "host"]
  },
  "metrics": {
    "snapshot_interval_sec": 10
  },
  "logging": {
    "level": "info",
    "file": "/var/Zeus/Zeus.log"
  }
}
```

---

*This document serves as the complete architectural blueprint for the Zeus system. All modules, interfaces, and flows are defined to enable independent implementation, testing, and evolution.*