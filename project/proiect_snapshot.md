# RDMA Communication library for sequential all-to-all communication

This project covers the implementation of an sequential all-to-all communication library. The performance of the library will be tested on test applications in an environment with multiple VMs that will communicate using RDMA. Our implementation will be compared to other communication/synchronization libraries such as NCCL and MPI.

## Problem Description

In modern machine learning distributed training ecosystems, efficient node communication represents an important performance bottleneck. With techniques such as model and data parallelism, GPUs on different nodes need to leverage efficient algorithms for coordination and data sharing. Another bottleneck is the network bandwidth, which can be addressed by using advanced network technologies and NICs such as RDMA.

Sequential all-to-all communication patterns emerge as a optimization strategy, particularly in scenarions with:

* Memory-constrained GPU clusters

* Hierarchical network topologies

* Bandwidth-limited interconnects

* Large model synchronization requirements

For this project I will provide a communication library optimized for sequential all-to-all communication. The library will expose an all-to-all primitive for applications to use and synchronize their data. Some of the characteristics of sequentially exchanged data, one pair at a time, in a strict order:

* Data transfer happens in a serialized manner

* Lower peak bandwidth utilization

* Predictable network load

* Lower memory pressure

Compared to other communication strategies (parallel all-to-all, incast communication) it has the following pros:

* Lower peak memory requirements

* Easier to debug

* More predictable network behavior

While there are also several cons:

* Slower overall communication

* Reduced parallelism

* Lower aggregate bandwidth utilization

## Project Plan

There are several steps involved in the creation, validation and comparing the library with other alternatives.

### 1. Development environment setup

Initially I will develop the library and test applications in a virtual environment consisting of several VMs that can communicate through RDMA over SoftRoCE. After configuring the VM administration software and enable NVME on the machines, at least 2 VMs should be able to ping each other using ibverbs primitives. 

### 2. Sequential all-to-all library implementation

The library will be implemented most likely in C++. Here are the key features that it will provide:

* It uses InfiniBand Verbs API for RDMA operations

* Implements connection management and QP setup

* Provides optimized all-to-all primitive

* Handles memory registration and protection domains

* Includes proper resource cleanup

The funcionality will be validated by writing a distributed test application that will use the synchronization primitive.

### 3. Benchmarks/Comparation with other implementations

After I validate that applications can use the library, the performance will be tested by seeing how it behaves for varying workloads and synchronization between multiple nodes. 

My implementation will be compared to an MPI all-to-all implementation, and eventually with a NCCL implementation.

### 4. Test implementation on Fep

If possible, I will redo all the benchmarks and comparations on our univeristy cluster.

## Current Status

### Trying to set up the VM environment

Currently I'm having some issue setting up the dev environment. I'm having trouble enabling RDMA communications on the VMs based on the image provided during laboratory work. This may be due to the Bridge Adapter provided by VirtualBox, so I intend to either try out another VM mangement software, or try to configure manually a bunch of VMs to use SoftRoCE. Directly hosting some NVME-capable VMs on cloud platforms is also an option.

### Initial library implementation

I have implemented an initial draft of the communication library. We will consider a full-mesh environment for simplicity (all nodes can comunicate with each other directly). The all-to-all broadcast is pretty basic: 

- Every node posts Work Requests to the Receive Queue of the Queue Pairs of all the other nodes

- Then, every node posts Work Requests to the Send Queue of the Queue Pairs of all the other nodes

- Then, every node polls the Completion Queue untill the message transfers are done

After setting up the VM environment I will test this initial implementation on a test application.