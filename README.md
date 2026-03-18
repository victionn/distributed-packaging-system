# BitTorrent-Inspired Distributed Package System (C)

## Overview
This project implements a peer-to-peer package distribution system in C, inspired by BitTorrent-style file sharing. It supports:

- Parsing custom `.bpkg` package files  
- Building and querying **Merkle Trees** for data integrity  
- Managing peers over a network  
- Handling package distribution and verification  
- Multi-threaded client/server communication  

The system is split into two major components:
- **Part 1:** Package parsing + Merkle tree validation  
- **Part 2:** Peer-to-peer networking + package management  

---

## Features

### Package & Data Integrity (Part 1)
- Custom `.bpkg` file parsing
- Construction of **Merkle Trees** for chunk verification
- Efficient hash-based validation of file integrity
- Query support:
  - Minimum completed hashes
  - Chunk hash retrieval
  - Tree traversal operations

### Peer-to-Peer Networking (Part 2)
- Multi-threaded server using sockets
- Configurable networking via `.cfg` files
- Peer connection management (`CONNECT`)
- Thread-safe peer list using **mutex locks**
- Package distribution and synchronization

### Testing
- Comprehensive test suite covering:
  - Edge cases
  - Invalid inputs
  - Missing/corrupted data files
  - Peer interaction scenarios
- Automated test binaries included
