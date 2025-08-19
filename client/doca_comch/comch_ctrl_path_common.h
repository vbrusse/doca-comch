/*
 * Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */

/*
 * Original filename: comch_ctrl_path_common.h
 *
 * Filename: comch_ctrl_path_common.h
 *
 * Customized by: Vlademir Brusse
 *
 * Date: 2025/07/23
 *
 */

#ifndef COMCH_COMMON_H_
#define COMCH_COMMON_H_

#include <stdbool.h>

#include <doca_comch.h>
#include <doca_ctx.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_pe.h>



#include <pthread.h>                                    /* VBrusse */
/* #include <nrLDPC_coding_interface.h> */              /* VBrusse - OAI interface definition for 'LDPC slot coding' */
#include <nrLDPC_defs.h>                                /* VBrusse - OAI interface definition for 'LDPC segment coding' */
/* #include "/home/vlademir/openairinterface5g/openair1/PHY/CODING/nrLDPC_defs.h" */    /* VBrusse */



#define MAX_USER_TXT_SIZE 4096               /* Maximum size of user input text */
#define MAX_TXT_SIZE (MAX_USER_TXT_SIZE + 1) /* Maximum size of input text */
#define SLEEP_IN_NANOS (10 * 1000)           /* Sample tasks every 10 microseconds */
#define CC_MAX_MSG_SIZE 4080                 /* Comm Channel maximum message size */



/*
 * For Base Graph 1 (BG1), Kb is typically used for large block sizes (up to 8448 bits).
 * For Base Graph 2 (BG2), Kb is used for smaller block sizes (up to 3840 bits).
 *
 * It has been reserved 10 blocks of 8448 bits (the maximum large block size) due to the coding rate.
 *
 * Ex: Encoded Block Size (N): for code rate R = 1/3, the encoded block size is: N=K/R = 512×3 = 1536 bits.
 *
 * To accommodate 10 blocks of 8448 bits, you first need to convert the total number of bits to bytes:
 *
 * Total bits=10×8448=84480 bits
 *
 * Since 1 byte = 8 bits, the total number of bytes is:
 *
 * Total bytes=84480/8=10560 bytes
 *
 * Therefore, the array length should be 10560 bytes to accommodate 10 blocks of 8448 bits.
 *
 *
 * 1. Determine Code Block Size
 *      For BG1 (K = 8448 bits)
 *              • Output bits: K = 8448 (maximum)
 *              • Bytes: 8448 / 8 = 1056 bytes
 *      For BG2 (K = 3840 bits)
 *              • Output bits: K = 3840 (maximum)
 *              • Bytes: 3840 / 8 = 480 bytes
 *
 * 2. Calculate Based on Lifting Size (Z)
 *      The actual decoded size is:
 *              decoded_bits = (K / Z) * Z_calculated
 *      Where:
 *              • Z = Lifting size (from 3GPP Table 5.3.2-1, e.g., 96 in your logs)
 *              • K = Base graph info bits (8448 for BG1, 3840 for BG2)
 *              • Z_calculated = Effective lifting after rate matching
 * Example
 *      • BG2 + Z = 96:
 *              o Info bits per block: K = 3840 / 96 = 40
 *              o Decoded bits: 40 * 96 = 3840 (same as BG2 max)
 *              o Output bytes: 3840 / 8 = 480 bytes
 *      • BG1 + Z = 96
 *              o Infor bits per block: K = 8448 / 96 = 88
 *              o Decoded bits: 88 * 96 = 8448 (same as BG1 max)
 *              o Output bytes: 8448 / 8 = 1056
 *
 * 3. Match to LLR Input (ex 2352 bytes = OAI Samples)
 *      LLR input is 2352 bytes:
 *              • Likely corresponds to rate-matched length (E) for one segment.
 *              • For BG2 + Z=96, typical rate-matched sizes are multiples of 2*Z = 192 bytes.
 *              • 2352 bytes / 192 = 12.25 → Likely 12 segments of 196 bytes each (adjusted for CRC).
 *
 * For implementation:
 *      • BG2 (most common for small blocks)
 *              uint8_t decoded_output[480];                            // 3840 bits = 480 bytes
 *      • BG1 (if used for larger blocks)
 *              uint8_t decoded_output[1056];                           // 8448 bits = 1056 bytes
 *
 */

/* #define CC_LDPC_BLOCK_LEN 10560                                      Input/Output block length 10560 bytes ~ 10.3125 KB */

/*
 * LLRs buffer size = N = 68 * Z for BG1 and N = 52 * Z for BG2.
 * Let consider the worst case BG1, so if Z = 96 in samples case and 3GPP defines N = 68 * Z (BG1) as the number of bytes in the LDPC codeblock
 *
 * N = 68 * 96 = 6528 bytes, each LLR is stored in 1 byte (int8_t).
 *
 * It will be used the LLR buffer sized defined by OAI:
 *
 *      #define NR_LDPC_MAX_NUM_LLR 27000       - Maximum number of possible input LLR = NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX
 *
 */
#define CC_LDPC_IN_BLOCK_LEN 6528                                       /* Input block length 3072 bytes (1024x3) = 3 KB, to fit the OAI Samples and based on param E of OAI interface */

/*
 * data_out buffer size is the K value
 *
 * In case of samples, K = 904 bits
 *
 * data_out buffer size = 904 / 8 = 113 bytes
 *
 */
#define CC_LDPC_OUT_BLOCK_LEN 1056                                      /* Output block length 1056 bytes - maximum is BG1 Block Size */

/* #define CC_LLR_SIZE 2360 */                                          /* decoding data: LDPC block length - 3072 to fit OAI Samples and based on param E of OAI interface */
#define ARMRAL_CACHE_ALIGN 64                                           /* cache-based alignment */

/* OAI ldpc encoder interface - LDPCencoder.c function input parameters (GitLab) */
/* OAI interface structure */
struct oai_encoder_params_t {                                           /* VBrusse - structure to store the OAI nrLDCP_encod function's data/input parameters */
        /* unsigned char *inputArray;                                   single block iinput as a sequence of bits to be transmitted */
        /* unsigned char *outputArray;                                  ldpc output as a sequence of encoded bits (codeword) */
        uint8_t *inputArray;
        uint8_t *outputArray;
        uint32_t K;                                                     /* Size in bits of the code segments */
        uint32_t Kb;                                                    /* Number of lifting sizes to fit the payload */
        uint32_t Zc;                                                    /* Lifting size */
        uint32_t F;                                                     /* Number of "Filler" bits */
        uint8_t BG;                                                     /* Encoder BG. Selected Base Graph. LDPC_BASE_GRAPH_1 = 0, LDPC_BASE_GRAPH_2 = 1 */
};

/*
struct oai_decoder_params_t {                                           VBrusse - structure to store the OAI nrLDCP_decod function's data/input parameters
        t_nrLDPC_dec_params *p_dec_pars;                                p_decParams
        uint8_t h_pid;                                                  harq_pid
        uint8_t u_id;                                                   ulsch_id
        uint8_t C_par;                                                  C */
        /* int8_t *llr; */                                              /* p_llr */
/*      int8_t llr[CC_LDPC_IN_BLOCK_LEN]; */
        /* int8_t *out; */                                              /* p_out */
/*      int8_t out[CC_LDPC_OUT_BLOCK_LEN];
        t_nrLDPC_time_stats *p_t_stats;                                 p_time_stats
        decode_abort_t *u_ab;                                           ab
};
*/
struct ldpc_encod_params_t {                                            /* VBrusse  - structure to store the lppc encoding data/input parameters */
        uint8_t inputBlock[CC_LDPC_IN_BLOCK_LEN];                       /* Input block <================ Testar shared memory com o host definindo este pointer e o array no host */
        /* uint8_t *inputBlock; */
        uint8_t bg;                                                     /* Base Graph (BG). LDPC_BASE_GRAPH_1 = 0, LDPC_BASE_GRAPH_2 = 1 */
        uint32_t z;                                                     /* Lifting Size (Zc) */
        uint32_t k;                                                     /* Block length (K) */
        uint32_t len_filler_bits;                                       /* Filler bits to pad the input block */
        uint8_t outputBlock[CC_LDPC_OUT_BLOCK_LEN];                     /* Output block - codeword */
};

// =================================================================================
// LDPC Decoder struct - struct to represent the ldpc decoding data/input parameters
// - ArmRAL deals with CRC Early Termination
// - Uses 64-byte alignment for SIMD optimization
// - Follows 3GPP TS 38.212 (5G NR)
// =================================================================================
/*struct ldpc_decod_params_t {*/                                        /* VBrusse: struct to store the ldpc decoding data/input parameters */
        /* const int8_t *llrs; */                                       /* Pointer to the LLRs (soft values) */
        /*int8_t llrs[NR_LDPC_MAX_NUM_LLR] __attribute__((aligned(64)));*/      /* 64-byte aligned LLRs (soft values) buffer - this DPU memory block shall be declared as an array, */
                                                                        /* it can not be a pointer */
        /*uint32_t n;*/                                                     /* p_llr buffer size shall be calculate as length 68 * Z t or BG=1 and 52 * Z for BG=2. */
                                                                        /* Maximum Z (Lifting Factor / Lifting Size) = 384 */
        /*uint32_t kp;*/                                                    /* 'Kprime' is the K' in the standard 3GPP TS 38.212 section 5.2.2. It is the number of the */
                                                                        /* payload bits per uncoded segment. In other word, it is the number of useful bits in the */
                                                                        /* output of the decoder. */
        /*uint8_t bg;*/                                                 /* Base graph type. LDPC_BASE_GRAPH_1 = 0, LDPC_BASE_GRAPH_2 = 1 */
        /*uint32_t z;*/                                                     /* Lifting size */
        /*uint32_t crc_idx;*/                                               /* CRC index */
        /*uint32_t num_its;*/                                               /* Number of iterations */
        /*uint8_t data_out[CC_LDPC_OUT_BLOCK_LEN] __attribute__((aligned(64)));*/   /* 64-byte aligned buffer to store the LDPC decoder data output with the decoded bits */
/*} __attribute__((aligned(64)));*/                                         /* Struct-level alignment ensures 64-byte for all fields. Struct also 64-byte aligned (optional but safe) */

struct ldpc_decod_params_t {                                            /* VBrusse: structure to store the ldpc decoding data/input parameters */
        /* const int8_t *llrs; */                                       /* Pointer to the LLRs (soft values) */
        int8_t llrs[CC_LDPC_IN_BLOCK_LEN];                              /* LLRs (soft values) buffer - this host memory block shall be declared as an array, it can not be a pointer */
        uint32_t n;                                                     /* p_llr buffer size shall be calculate as length 68 * Z t or BG=1 and 52 * Z for BG=2. */
                                                                        /* Maximum Z (Lifting Factor / Lifting Size) = 384 */
        uint32_t kp;                                                    /* 'Kprime' is the K' in the standard 3GPP TS 38.212 section 5.2.2. It is the number of the */
                                                                        /* payload bits per uncoded segment. In other word, it is the number of useful bits in the */
                                                                        /* output of the decoder. */
        uint8_t bg;                                                     /* Base graph type. LDPC_BASE_GRAPH_1 = 0, LDPC_BASE_GRAPH_2 = 1 */
        uint32_t z;                                                     /* Lifting Factor / Lifting Size */
        uint32_t crc_idx;                                               /* CRC index */
        uint32_t num_its;                                               /* Number of iterations */
        uint8_t data_out[CC_LDPC_OUT_BLOCK_LEN];                        /* Buffer to store the LDPC decoder data output with the decoded bits */
};



struct comch_config {
        char comch_dev_pci_addr[DOCA_DEVINFO_PCI_ADDR_SIZE];            /* Comm Channel DOCA device PCI address */
        char comch_dev_rep_pci_addr[DOCA_DEVINFO_REP_PCI_ADDR_SIZE];    /* Comm Channel DOCA device representor PCI address */
        char text[MAX_TXT_SIZE];                                        /* Text to send to Comm Channel server */
        struct ldpc_encod_params_t ldpc_encod_params;                   /* VBrusse - structure with the data/input parameters of the LDPC encoder */
        struct ldpc_decod_params_t ldpc_decod_params;                   /* VBrusse - structure with the data/input parameters of the LDPC decoder */
};

struct comch_ctrl_path_client_cb_config {
        /* User specified callback when task completed successfully */
        doca_comch_task_send_completion_cb_t send_task_comp_cb;
        /* User specified callback when task completed with error */
        doca_comch_task_send_completion_cb_t send_task_comp_err_cb;
        /* User specified callback when a message is received */
        doca_comch_event_msg_recv_cb_t msg_recv_cb;
        /* Whether need to configure data_path related event callback */
        bool data_path_mode;
        /* User specified callback when a new consumer registered */
        doca_comch_event_consumer_cb_t new_consumer_cb;
        /* User specified callback when a consumer expired event occurs */
        doca_comch_event_consumer_cb_t expired_consumer_cb;
        /* User specified context data */
        void *ctx_user_data;
        /* User specified PE context state changed event callback */
        doca_ctx_state_changed_callback_t ctx_state_changed_cb;
};

struct comch_ctrl_path_server_cb_config {
        /* User specified callback when task completed successfully */
        doca_comch_task_send_completion_cb_t send_task_comp_cb;
        /* User specified callback when task completed with error */
        doca_comch_task_send_completion_cb_t send_task_comp_err_cb;
        /* User specified callback when a message is received */
        doca_comch_event_msg_recv_cb_t msg_recv_cb;
        /* User specified callback when server receives a new connection */
        doca_comch_event_connection_status_changed_cb_t server_connection_event_cb;
        /* User specified callback when server finds a disconnected connection */
        doca_comch_event_connection_status_changed_cb_t server_disconnection_event_cb;
        /* Whether need to configure data_path related event callback */
        bool data_path_mode;
        /* User specified callback when a new consumer registered */
        doca_comch_event_consumer_cb_t new_consumer_cb;
        /* User specified callback when a consumer expired event occurs */
        doca_comch_event_consumer_cb_t expired_consumer_cb;
        /* User specified context data */
        void *ctx_user_data;
        /* User specified PE context state changed event callback */
        doca_ctx_state_changed_callback_t ctx_state_changed_cb;
};

/*
 * Register the command line parameters for the DOCA CC samples
 *
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t register_comch_params(void);

/**
 * Clean client and its PE
 *
 * @client [in]: Client object to clean
 * @pe [in]: Client PE object to clean
 */
void clean_comch_ctrl_path_client(struct doca_comch_client *client, struct doca_pe *pe);

/**
 * Initialize a cc client and its PE
 *
 * @server_name [in]: Server name to connect to
 * @hw_dev [in]: Device to use
 * @cb_cfg [in]: Client callback configuration
 * @client [out]: Client object struct to initialize
 * @pe [out]: Client PE object struct to initialize
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t init_comch_ctrl_path_client(const char *server_name,
                                         struct doca_dev *hw_dev,
                                         struct comch_ctrl_path_client_cb_config *cb_cfg,
                                         struct doca_comch_client **client,
                                         struct doca_pe **pe);

/**
 * Clean server and its PE
 *
 * @server [in]: Server object to clean
 * @pe [in]: Server PE object to clean
 */
void clean_comch_ctrl_path_server(struct doca_comch_server *server, struct doca_pe *pe);

/**
 * Initialize a cc server and its PE
 *
 * @server_name [in]: Server name to connect to
 * @hw_dev [in]: Device to use
 * @rep_dev [in]: Representor device to use
 * @cb_cfg [in]: Server callback configuration
 * @server [out]: Server object struct to initialize
 * @pe [out]: Server PE object struct to initialize
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t init_comch_ctrl_path_server(const char *server_name,
                                         struct doca_dev *hw_dev,
                                         struct doca_dev_rep *rep_dev,
                                         struct comch_ctrl_path_server_cb_config *cb_cfg,
                                         struct doca_comch_server **server,
                                         struct doca_pe **pe);

#endif // COMCH_COMMON_H_
