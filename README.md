<<<<<<< HEAD
# Part 1
The implementation of loading bpkg objects required parsing a bpkg file and getting all information stored in the object. The merkle tree is built in conjuction with calculating its computed hashes. This is done by building the tree in a level order traversal, and once reaching the leaf nodes, the recursive functions begin to return after calculating the parents hash. Queries into the object like get_min_completed_hashes, get_all_chunk_hashes_from_hash and get_min_hashes all utilised a merkle tree node queue, that traverses the tree in level order and perform necessary operations to get the query result.

# Part 2
Part 2 required the reading of a config file through simple file parsing. After this, a server needs to be created with the port specified in the config. This will handle incoming connections when another instance of a program uses CONNECT. Likewise, when a program enters CONNECT, a thread will be created for the client and will be detached to allow it to be ran independently. Operations on the peer list requires it to be mutex_locked to prevent data races and unintended changes. To handle packages, a linked list of packages is initialised and whenever a package is added given the bpkg file, a bpkg object and its corresponding merkle tree will be created. This is used to handle further operations such as creating a data file if it doesn't exist, and checking if the file is complete or not from functions provided in part 1. This is done by checking if the roots expected hash matches the computed hash.


# Part 1 Test Implementation
Part 1 test cases rigorously tests the implementation of bpkg and the merkle tree and its operations. A combination of positive, negative and edge test cases ensure the program is running as intended. This includes invalid arguments, cases where data files don't exist and when the bpkg and the data file does not match up.

# To run tests for part 1, type: ./test.exe 
(pkgmain binary file must exist for test cases to work)

WARNING: Test 10 creates binary4.data in the tests directory due to it not existing as referenced in the bpkg file. To pass this test if ran more than once, delete binary4.data from the tests directory

# Part 2 Test Implementation

Part 2 tests handles a range of positive, negative and edge test cases for packages and peer management. Non exisiting data files, invalid arguments as well as all the packages operations were sufficiently tested. Peers testing required the use of tmux to initliase multiple servers, providing input through send-key and its output redirected to its corresponding .out file.

# To run tests for part 2, type ./test2.exe
(btide binary file must exist for test cases to work)



=======
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
>>>>>>> 91eaff9259b958317ba16b8c73d336436050c4df
