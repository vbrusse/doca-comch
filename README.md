# doca-comch
The purpose of this research project is to investigate the offloading and hw acceleration of the compute-intensive LDPC function from the O-DU High-PHY layer of the 5G NR in a NVIDIA BlueField-3 DPU, according to O-RAN 7.2x split, to enhance the function performance (throughput, latency and energy efficiency).

## Table of Contents
- [Requirements](#requirements)
- [DOCA SDK](#doca-sdk)
    - [Installation Instructions](#installation-instructions)
    - [doca_comch API](#doca_comch-api)
    - [Usage](#usage)
- [ArmRAL](#armral)
    - [Installation Instructions](#installation-instructions-1)
    - [DPU-ArmRAL Integration](#dpu-armral-integration)
- [OpenAirInterface (OAI)](#openairinterface-oai)
    - [Installation Instructions](#installation-instructions-2)
    - [OAI-Host Integration](#oai-host-integration)
    - [Usage](#usage-1)
- [Experiments](#experiments)
- [Contributing](#contributing)

## Requirements
* Hardware
    * Host Intel XEON Gold 6526Y x86_64 with 64 cores
    * NVIDIA BlueField-3 DPU - Arm Cortex-A78AE aarch64 16 cores 

* Software
    * DOCA SDK 3.0.0
    * ArmRAL 25.07
    * OpenAirInterface 

* Arm CPU
    * To use the Cyclic Redundancy Check (CRC) functions, the Gold sequence generator, and the convolutional encoder, the library must run on a core that supports the AArch64 PMULL extension (check in lscpu, /proc/cpuinfo)

* Operating System
    * OAI
        * Linux Low-latency Kernel


## DOCA SDK

Content for DOCA SDK.

![DOCA Architecture Overview](./images/doca-software.jpg)


### Installation Instructions

Installation instructions for both host and BlueField image can be found in the [DOCA Installation Guide for Linux](https://docs.nvidia.com/doca/sdk/DOCA+Installation+Guide+for+Linux).

DOCA shall be installed on the host or on the BlueField-3 DPU, and the DOCA components is found under the /opt/mellanox/doca directory. These include the traditional SDK-related components (libraries, header files, etc.) as well as the DOCA samples, applications and tools.


## Project Structure

```
├── client/
│   └── opt/
|   |   └── mellanox/
│   |       └── doca/
│   |           ├── applications/
│   |           ├── include/
|   |           ├── infrastructure/
|   |           ├── lib/
|   |           ├── samples/
|   |           |   ├── common.c
|   |           |   ├── common.h
|   |           |   ├── .../
|   |           |   ├── doca_comch/
|   |           |   |   ├── comch_ctrl_path_common.c
|   |           |   |   ├── comch_ctrl_path_common.h
|   |           |   |   ├── comch_data_path_high_speed_common.c
|   |           |   |   ├── comch_data_path_high_speed_common.h
|   |           |   |   ├── meson.build
|   |           |   |   ├── nrLDPC_common.c
|   |           |   |   ├── nrLDPC_common.h
|   |           |   ├── nrLDPC_decod_client/
|   |           |   |   ├── meson.build
|   |           |   |   ├── nrLDPC_decod.c
|   |           |   |   └── nrLDPC_decod_client.c
|   |           |   ├── nrLDPC_defs.h
|   |           |   ├── nrLDPC_encod_client/
|   |           |   |   ├── meson.build
|   |           |   |   ├── nrLDPC_encod.c
|   |           |   |   └── nrLDPC_encod_client.c
|   |           |   ├── nrLDPC_init/
|   |           |   |   ├── meson.build
|   |           |   |   └── nrLDPC_initcall.c
|   |           |   ├── nrLDPC_shutdown/
|   |           |   |   ├── meson.build
|   |           |   |   └── nrLDPC_shutdown.c
|   |           |   └── vDU/
|   |           |       ├── meson.build
|   |           |       └── vdu_high_phy_ldpc_codes.c
|   |           └── tools/
├── server/
│   └── opt/
|       └── mellanox/
│           └── doca/
│               ├── applications/
│               ├── include/
|               ├── infrastructure/
|               ├── lib/
|               ├── samples/
|               |   ├── common.c
|               |   ├── common.h
|               |   ├── .../
|               |   ├── doca_comch/
|               |   |   ├──
|               |   |   ├── 
|               └── tools/
├── images/
│   └── doca-software.jpg
└── README.md
```

#### Compilation

To compile and build the doca_comch servers:

##### Host build commands

```bash
cd /opt/mellanox/doca/services/doca_comch
meson /tmp/build
ninja -C /tmp/build
```

##### DPU build commands

* doca_comch server
    
```bash
# For LDPC Decoder Server
cd /opt/mellanox/doca/services/doca_comch/nrLDPC_decod_server
meson /tmp/build
ninja -C /tmp/build

# For LDPC Encoder Server  
cd /opt/mellanox/doca/services/doca_comch/nrLDPC_encod_server
meson /tmp/build
ninja -C /tmp/build
```

The generated servers are located under the /tmp/build/ directory.

* doca_comch client

```bash
# host
cd /tmp/build
ls -la
    compile_commands.json
    .gitignore
    .hgignore
    libldpc_armral.so
    libldpc_armral.so.p
    libldpc_armral_static.a
    libldpc_armral_static.a.p
    meson-info
    meson-logs
    meson-private
    .ninja_deps
    .ninja_log

# DPU
cd /tmp/build
ls -la



```

The generated clients are located under the /tmp/build/ directory.

The library libldpc_armral.so (host side) will be built again when the OAI 5G NR executables are generated.


### doca_comch API

doca_comch content here...


### Running the doca_comch server and client

First start running the server on DPU side:

```bash
# For LDPC Decoder Server
cd /tmp/build
    ./nrLDPC_decod_server -p 03:00.0 -r 03:00.0
```

after that, the client on host side:

```bash
# For LDPC Encoder Server  
cd /tmp/build
    ./nrLDPC_encod_server -p 03:00.0 -r 03:00.0
```

The OAI 5G CN stack shall be running and the servers nrLDPC_decod_server and nrLDPC_encod_server (DPU side) with the Arm LDPC kernels implementation shall be started (this order does not matter), and then the entire OAI 5G NR stack shall be brought up and running (with the gNB and the nrUE). All components will be running on the same host.

The nrUE (nr-uesoftmodem) shall be started with the flag '--loader.ldpc.shlibversion _armral' that indicates the OAI Loader to load and executed the customized 'libldpc_armral.so' instead of the standard ldpc library from OAI. This is the doca_comch shared library that contains the clients nrLDPC_decod_client and nrLDPC_encod_client that implement the OAI interfaces.

When a LDPC decoding (Uplink) or a LDPC encoding (Downlink) function call happens in the OAI stack (DU High-PHY layer) the doca_comch client (host side) will be called to offload the LDPC function on DPU (server) and the function will be run on Arm multicore CPUs properly.


## ArmRAL

ArmRAL is an open-source software library that provides building blocks (functions or kernels) required by RAN L1 that run on CPU for optimized signal processing and related maths functions for enabling 5G Radio Access Network (RAN) deployments. It leverages the efficient vector units available on Arm cores that support the Armv8-a architectures (Neon, SVE, SVE2 …) and SIMD/Vector capabilities, offering an API that can be integrate into L1 stack to accelerate 5G NR signal processing workloads. As functions/kernels include:

* Matrix and vector arithmetic, such as matrix multiplication.
* Fast Fourier Transforms (FFTs).
* Digital modulation and demodulation.
* Cyclic Redundancy Check (CRC).
* Encoding and decoding schemes, including Polar, Low-Density Parity Check (LDPC), and Turbo.
* Compression and decompression.

This project uses the ArmRAL Low-Density Parity Check (LDPC) decoding and encoding functions.

Download ArmRAL from [ArmRAL GitLab](https://gitlab.arm.com/networking/ral) or from [ArmRAL Releases](https://gitlab.arm.com/networking/ral/-/releases).


### Installation Instructions

The tutorial to build and install the Arm RAN Acceleration Library (ArmRAL) can be found in the [Get started with Arm RAN Acceleration Library](https://developer.arm.com/documentation/102249/2504/Tutorials/Get-started-with-Arm-RAN-Acceleration-Library?lang=en).


### DPU-ArmRAL Integration

Content here...

## OpenAirInterface (OAI)

OpenAirInterface (OAI) is an open-source software platform that provides a complete, software-based implementation of 4G (LTE) and 5G (NR) cellular network standards. It essentially allows to run a full mobile network, from the core network to the radio access network (e.g., base station and user equipment) on standard computing hardware.


### Core Components

OpenAirInterface is divided into two main parts that work together to form a cellular network:

* OAI Radio Access Network (OAI-RAN): This part handles the radio communication. It includes the software for both the eNodeB (the 4G base station) and gNodeB (the 5G base station), as well as the User Equipment (UE) or device modem software.

* OAI Core Network (OAI-CN): It implements the Evolved Packet Core (EPC) for 4G and the 5G Core Network (5GC), which manage subscriber authentication, data routing, and mobility.

These functional splits are a core concept of Open RAN (O-RAN) because they allow for interoperability between different vendors. OpenAirInterface is a powerful tool for exploring these splits.


### OAI and Fucntional Splits

OAI implements the two most common splits, O-RAN 7.2x and 3GPP split 2.


#### O-RAN 7.2x Split

This is the most widely adopted and a defining characteristic of O-RAN. The O-RAN 7.2x split divides the Physical Layer (PHY) of the radio protocol stack between the Radio Unit (O-RU) and the Distributed Unit (O-DU).

* O-RU (Radio Unit): Handles the physical RF (Radio Frequency) functions, such as digital-to-analog conversion, as well as the lower-level PHY tasks like Inverse Fast Fourier Transform (IFFT) and Fast Fourier Transform (FFT).

* O-DU (Distributed Unit): Manages the higher-level PHY functions, including channel coding and decoding (Polar and LDPC codes), as well as the Medium Access Control (MAC) layer.

This split is a balance between centralization and fronthaul bandwidth. By keeping some PHY processing in the O-RU, it reduces the amount of data that needs to be sent over the fronthaul (the link between the O-RU and O-DU), making it a more practical choice for a wider range of deployments. The O-RAN Alliance standardized this split to promote vendor interoperability.


#### 3GPP Split 2

This is an alternative, higher-level split defined by the 3GPP standards body. In this split, the entire Physical Layer (PHY) resides in the Distributed Unit (DU), while the Centralized Unit (CU) handles the Radio Resource Control (RRC) and Packet Data Convergence Protocol (PDCP) layers. The interface is between the CU and the DU.

* CU (Centralized Unit): Handles higher-level functions, including RRC and PDCP, which are less time-sensitive.

* DU (Distributed Unit): Handles the entire MAC, Radio Link Control (RLC), and PHY layers.

Compared to O-RAN 7.2x, this split requires less intelligence at the cell site (the DU). However, it places greater demands on the fronthaul network (the link between the DU and the Radio Head), as a large amount of raw baseband data needs to be transmitted. While defined by 3GPP, this split is less commonly used in commercial O-RAN deployments than the 7.2x split due to its stricter transport requirements.


### Installation Instructions

OAI installation content.

### OAI-Host Integration

OAI integration content here...

### Usage

OAI usage content.

## Experiments

Content here...

## Contributing

Contribution guidelines.
