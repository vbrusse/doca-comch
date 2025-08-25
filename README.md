# doca-comch
The purpose of this research project is to investigate the offloading and hw acceleration of the compute-intensive LDPC function from the O-DU High-PHY layer of the 5G NR in a NVIDIA BlueField-3 DPU, according to O-RAN 7.2x split, to enhance this function performance (throughput, latency and energy efficiency).

## Table of Contents
- [DOCA SDK](#doca-sdk)
    - [Installation Instructions](#installation-instructions)
    - [doca_comch API](#doca_comch-api)
    - [Usage](#usage)
- [ArmRAL](#armral)
    - [Installation Instructions](#installation-instructions-1)
    - [Library Creation](#library-creation)
    - [DPU-ArmRAL Integration](#dpu-armral-integration)
- [OpenAirInterface (OAI)](#openairinterface-oai)
    - [Installation Instructions](#installation-instructions-2)
    - [OAI-Host Integration](#oai-host-integration)
    - [Usage](#usage-1)
- [Experiments](#experiments)
- [Contributing](#contributing)

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
The library libldpc_armral.so (host side) will be built again when the OAI 5G NR executables are generated.

#### Running the doca_comch server and client
First start running the server on DPU side:
```bash
# For LDPC Decoder Server
cd /tmp/build
    ./nrLDPC_decod_server -p 03:00.0 -r 03:00.0

# For LDPC Encoder Server  
cd /tmp/build
    ./nrLDPC_encod_server -p 03:00.0 -r 03:00.0
```
The OAI 5G CN stack shall be running and the servers nrLDPC_decod_server and nrLDPC_encod_server (DPU side) with the Arm LDPC kernels implementation shall be started (this order does not matter), and then the entire OAI 5G NR stack shall be brought up and running (with the gNB and the nrUE). All components will be running on the same host.

The nrUE (nr-uesoftmodem) shall be started with the flag '--loader.ldpc.shlibversion _armral' that indicates the OAI Loader to load and executed the customized 'libldpc_armral.so' instead of the standard ldpc library from OAI. This is the doca_comch shared library that contains the clients nrLDPC_decod_client and nrLDPC_encod_client that implement the OAI interfaces.

When a LDPC decoding (Uplink) or a LDPC encoding (Downlink) function call happens in the OAI stack (DU High-PHY layer) the doca_comch client (host side) will be called to offload the LDPC function on DPU (server) and the function will be run on Arm multicore CPUs properly.


### doca_comch API

doca_comch content here...

### Usage

DOCA usage content.

## ArmRAL

Content here...

### Installation Instructions

The tutorial to build and install the Arm RAN Acceleration Library (ArmRAL) can be found in the [Get started with Arm RAN Acceleration Library](https://developer.arm.com/documentation/102249/2401/Tutorials/Get-started-with-Arm-RAN-Acceleration-Library--ArmRAL-).

### DPU-ArmRAL Integration

Content here...

## OpenAirInterface (OAI)

Content for OAI.

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
