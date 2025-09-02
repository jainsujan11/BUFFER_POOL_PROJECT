# DBMS Group Term Project — Simulation and Analysis of Buffer Pool Manager Strategies

This repository contains our **DBMS group term project** proposal and implementation for simulating and analyzing the performance of various buffer pool management strategies in a database management system.

---

## 🎯 Objective

To simulate and analyze the performance of various buffer management strategies (LRU, MRU, CLOCK, and pinned blocks) in managing database pages for simple Join and Selection queries on small tables. The project will integrate with the SQLite C library for realistic disk-oriented database operations.

---

## 🛠️ Project Goals

- Implement different buffer replacement strategies: **LRU**, **MRU**, **CLOCK**, and **pinned blocks**.
- Integrate the **SQLite C library** for simulating realistic database interactions.
- Perform **comparative analysis** by measuring disk I/O required for join/selection queries.

---

## 🧪 Methodology

1. **Buffer Pool Implementation**  
   - Construct a buffer pool managing fixed-size pages (4 KB each) and simulate frame management.  
   - Ensure thread safety using mutexes (latches).

2. **Replacement Policies**  
   - Implement buffer replacement strategies:  
     - Least Recently Used (LRU)  
     - Most Recently Used (MRU)  
     - CLOCK  
     - Pinned Blocks (non-evictable)

3. **Integration with SQLite**  
   - Utilize SQLite's C library for query processing.  
   - Replace default I/O operations with custom buffer manager calls using a custom VFS.

4. **Performance Evaluation**  
   - Develop test cases involving basic JOIN and Selection queries.  
   - Measure and record disk I/O for each strategy under various workloads.

5. **Comparative Analysis**  
   - Analyze total disk I/O operations across strategies.  
   - Identify efficiency trade-offs between LRU, MRU, CLOCK, and pinned-block strategies.

---

## 📈 Expected Outcomes

- Clear insights into buffer management performance.
- Identification of optimal scenarios for each replacement strategy.
- Practical knowledge of DBMS internals and performance tuning.

---

## 📦 Deliverables

- Fully implemented simulation code.
- Documentation and intermediate progress reports.
- Final comprehensive report with methodology, results, screenshots, comparative analysis, and conclusions.

---

## 🏗️ Implementation Details (Planned Classes)

- `LRUReplacer` — Tracks least recently used frames.
- `MRUReplacer` — Tracks most recently used frames.
- `CLOCKReplacer` — Uses reference bits and a circular pointer.
- `DiskScheduler` — Maintains a queue of disk operations.
- `DiskManager` — Simulates read/write operations and tracks counts.
- `BufferPoolManager` — Coordinates pages, pins, and I/O through custom policies.

---

## 🔗 References

- [SQLite Documentation](https://sqlite.org/index.html)  
- [CMU 15-445/645 Project Reference (BusTub — Buffer Pool Manager)](https://github.com/cmu-db/bustub)

---
