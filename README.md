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
|               |   |   ├── comch_ctrl_path_common.c
|               |   |   ├── comch_ctrl_path_common.h
|               |   |   ├── comch_data_path_high_speed_common.c
|               |   |   ├── comch_data_path_high_speed_common.h
|               |   |   ├── meson.build
|               |   |   ├── nrLDPC_common.c
|               |   |   ├── nrLDPC_common.h
|               |   ├── nrLDPC_decod_client/
|               |   |   ├── meson.build
|               |   |   ├── nrLDPC_decod.c
|               |   |   └── nrLDPC_decod_client.c
|               |   ├── nrLDPC_defs.h
|               |   ├── nrLDPC_encod_client/
|               |   |   ├── meson.build
|               |   |   ├── nrLDPC_encod.c
|               |   |   └── nrLDPC_encod_client.c
|               |   ├── nrLDPC_init/
|               |   |   ├── meson.build
|               |   |   └── nrLDPC_initcall.c
|               |   ├── nrLDPC_shutdown/
|               |   |   ├── meson.build
|               |   |   └── nrLDPC_shutdown.c
|               |   └── vDU/
|               |       ├── meson.build
|               |       └── vdu_high_phy_ldpc_codes.c
|               └── tools/
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



### doca_comch API

doca_comch content here...

### Usage

DOCA usage content.

## ArmRAL

Content here...

### Installation Instructions

Content here...

### Library Creation

Content here...

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
