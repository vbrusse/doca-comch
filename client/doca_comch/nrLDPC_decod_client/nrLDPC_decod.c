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
 * Original filename: comch_data_path_high_speed_client_main.c
 *
 * Filename: nrLDPC_decod.c
 *
 * DOCA Communication Channel Client API customized by: Vlademir Brusse
 *
 * Date: 2025/07/30
 *
 */

#include <stdlib.h>

#include <string.h>                                                     /* VBrusse - used by memset */

#include <doca_argp.h>
#include <doca_dev.h>
#include <doca_log.h>

#include "comch_ctrl_path_common.h"

#define DEFAULT_PCI_ADDR "b1:00.0"
#define DEFAULT_MESSAGE "Message from the client"                       /* VBrusse */



// ALIAS DECLARATION
// LDPCdecoder declared as an alias for nrLDPC_decod
extern int32_t LDPCdecoder(t_nrLDPC_dec_params *p_decParams,
                           uint8_t harq_pid,
                           uint8_t ulsch_id,
                           uint8_t C,
                           int8_t *p_llr,
                           int8_t *p_out,
                           t_nrLDPC_time_stats *p_time_stats,
                           decode_abort_t *ab)
        __attribute__((alias("nrLDPC_decod")));
// END ALIAS



DOCA_LOG_REGISTER(NRLDPC_DECOD_CLIENT::MAIN);

/* DOCA comch client's logic */
doca_error_t start_nrLDPC_decod_client(const char *server_name,
                                       const char *dev_pci_addr,
                                       struct ldpc_decod_params_t *pldpc_decod_params);


/*
 * nrLDPC_decod_offloading - This host function starts/calls the Offloading Service as a task
 * exposured by "nrLDPC_decod_server" to compute the 5G NR LDPC function on the DPU over a
 * PCIe channel established through DOCA Communication Channel API client using the ConnectX
 * adapter to run on DPU the LDPC decoder kernel implemented/provided by ArmRAL.
 * It makes the function offloading and leverages the DPU's hardware acceleration capabilities
 * to efficiently execute the LDPC decoding task, potentially improving performance and reducing
 * the CPU load.
 *
 * @p_decPars [in]:
 * @harq_pid [in]:
 * @ulsch_id [in]:
 * @C [in]:
 * @p_llr [in]:
 * @p_out [out]:
 * @p_time_stats [in]:
 * @ab [in]:
 *
 *
 * (just replaced "encoded/enconding" with "decoded/decoding", needed to collect the decoding comments properly)
 *
 * Information Block Size (K)
 *      K represents the size of the information block, i.e., the number of information bits that
 *      are input to the LDPC decoder (the number of information bits before LDPC decoding).
 *      These bits form the portion of the transmitted codeword that carries actual user data.
 *      For each base graph (BG1 and BG2) in 5G NR, the size of K varies based on the chosen code
 *      rate and the lifting size Z.
 *      Higher values of K result in larger codewords, which can affect the efficiency of error
 *      correction and the system's overall throughput.
 *
 * Base Information Block Size (Kb)
 *      Kb is a parameter related to the base graph (BG) used for LDPC decoding. It defines the
 *      maximum size of the information block before the decoder starts lifting (expanding the
 *      base graph) to create the final codeword.
 *      It is the base graph's block size, which determines the structure of the LDPC code and the
 *      specific base graph to be used.
 *      The value of Kb determines which base graph (BG1 or BG2) is used, as well as the configuration
 *      of the LDPC code.
 *      In 5G NR, two base graphs are defined:
 *              - BG1: Optimized for large block sizes and higher code rates.
 *              - BG2: Optimized for smaller block sizes and lower code rates.
 *      For Base Graph 1 (BG1), Kb is typically used for large block sizes (up to 8448 bits).
 *      For Base Graph 2 (BG2), Kb is used for smaller block sizes (up to 3840 bits).
 *
 *      K and Kb together
 *              The parameter K (the actual information block size) must be less than or equal to Kb.
 *              The 5G NR standard defines values for Kb based on the transport block size and modulation
 *              scheme.
 *              Depending on K and the chosen base graph, the codeword is constructed by lifting the base
 *              graph matrix, leading to the generation of the parity-check matrix used for error correction.
 *
 * Lifting Size / Scaling Factor (Zc)
 *      Zc is the lifting factor that expands the base graph matrix used in LDPC decoding. In 5G NR, two
 *      base graphs (BG1 and BG2) are defined, and Zc determines how the base graph is "lifted" or
 *      "expanded" into the full parity-check matrix.
 *      Each entry in the base graph is expanded into a matrix of size Zc x Zc. The specific value of Zc
 *      depends on the size of the transport block and the Modulation and Coding Scheme (MCS) being used.
 *      The 5G NR standard defines specific valid values for Zc, which can vary based on factors like the
 *      code rate and block size.
 *      Typical values of Zc are powers of 2 and are often in the range from 2 to 384 or more, depending on
 *      the implementation.
 *      Larger values of Zc result in larger codewords, increasing the block size and improving the error
 *      correction capability but also increasing the complexity of the decoding process.
 *
 *      Basically, Zc is the lifting size used to expand the base graph into the final parity-check matrix.
 *      Larger Zc values result in larger block sizes and more powerful error correction, but they also
 *      increase the complexity of decoding.
 *      Zc is chosen based on the transport block size and the code rate, and it plays a key role in determining
 *      the structure of the LDPC codeword in 5G NR.
 *
 *      Role in Base Graph Expansion:
 *              In LDPC decoding, a base graph is selected (BG1 or BG2), which is a small, predefined matrix
 *              that specifies the connections between information bits and parity bits.
 *              The Zc parameter controls how the base graph is "lifted" into a much larger matrix by replacing
 *              each entry in the base graph with either a zero matrix or a circularly shifted identity matrix
 *              of size Zc.
 *              This lifting process determines the final structure of the LDPC codeword, including the number
 *              of parity bits and the overall size of the decoded block.
 *
 *      Relation to Information Block Size (K)
 *              Zc is related to the information block size K. The number of rows and columns in the lifted
 *              matrix will depend on both K and Zc.
 *              For a given K and base graph, Zc helps determine the total size of the codeword that will be
 *              transmitted.
 *
 *      Example in 5G NR
 *              For Base Graph 1 (BG1), the lifting size Zc can range from small values (like 2) for small
 *              block sizes to larger values (like 384) for very large block sizes.
 *              The exact choice of Zc depends on the transport block size, modulation order, and code rate.
 *
 * Filler Bits
 *      In LDPC decoding, filler bits are used to pad the input block when the size of the information block K is smaller
 *      than the required input size for the selected base graph (BG) and lifting size (Zc). The filler bits are typically
 *      set to zeros and are ignored during decoding.
 *
 *      The len_filler_bits parameter adjusts the length of the input block by adding dummy bits at the end of the input message
 *      to fit the block structure. This helps match the block structure for the selected coding rate.
 *
 * Base Graph 1 (BG1)
 *      BG1 is optimized for larger block sizes and higher code rates (typically above 1/3) (less redundancy, higher
 *      throughput).
 *      It is used when the information block size K exceeds a certain threshold (typically around 3840 bits or more).
 *      BG1 is selected when:
 *              - The block size K is large, and
 *              - A higher code rate is needed (which means less redundancy and higher efficiency).
 *      The higher code rate reduces the number of parity bits generated during LDPC decoding, which makes BG1 more
 *      suitable for large blocks with less redundancy needed.
 *
 * Base Graph 2 (BG2)
 *      BG2 is optimized for smaller block sizes and lower code rates (typically below 1/3) (more redundancy, higher
 *      error protection).
 *      It is used when the information block size K is smaller, generally under 3840 bits.
 *      BG2 is selected when:
 *              - The block size K is small, and
 *              - A lower code rate is required (which adds more redundancy for better error correction).
 *      BG2 has a structure that provides more parity bits relative to BG1, which makes it better suited for
 *      applications that require more robust error correction, especially for smaller blocks of data.
 *
 * The choice between BG1 and BG2 in the LDPC decoding process is made based on the size of the transport block and the
 * desired trade-off between error correction and data throughput.
 *
 * Key Differences Between BG1 and BG2
 *      Block Size:
 *              - BG1 is used for larger blocks (above ~3840 bits), while BG2 is used for smaller blocks (below ~3840 bits).
 *      Code Rate:
 *              - BG1 is used for higher code rates (i.e., less redundancy and higher data throughput).
 *              - BG2 is used for lower code rates (i.e., more redundancy and stronger error correction).
 *      Complexity and Use Cases:
 *              - BG1 is more efficient for scenarios where large amounts of data need to be transmitted with less redundancy,
 *                which is typical for high-throughput applications.
 *              - BG2 is more robust and is used when higher error correction capability is needed, which can be beneficial
 *                for more error-prone communication channels, such as control channels or low-throughput, high-reliability
 *                transmissions.
 *
 * Example in 5G NR
 *      - BG1 is typically used for data channels like the PDSCH (Physical Downlink Shared Channel) when the transport block
 *        size is large.
 *      - BG2 is often used for control channels like PDCCH (Physical Downlink Control Channel), where more reliability is
 *        required but the block sizes are smaller.
 *
 * gen_code
 *      In the structure decoder_implemparams_t, the field gen_code typically refers to a generator matrix code or a flag
 *      related to code generation in the context of LDPC decoding.
 *
 * gen_code might represent in this context
 *      Generator Matrix:
 *              In LDPC decoding, the generator matrix (often denoted as G) is used to generate the codewords from the input
 *              information bits. When gen_code is set, it might instruct the decoder to either use a precomputed generator
 *              matrix or dynamically generate one for decoding the information block.
 *      Code Generation Flag:
 *              It could also be a flag that signals whether the implementation should generate the LDPC code for the specific
 *              block being processed. This could be relevant if the decoder needs to dynamically adjust based on the block size,
 *              code rate, or base graph.
 *      Implementation Optimization:
 *              In certain implementations, gen_code might be used for switching between different decoding methods or
 *              optimizations. For example, it might select between using a systematic decoder (where the information bits appear
 *              directly in the codeword) or a non-systematic one.
 *
 * The specific function of gen_code could vary according to the implementation, but it generally relates to code generation logic
 * in the LDPC decoding process.
 *
 * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
 */

        /* VBrusse - OAI interface */
        /*
        Steps to Create the Test Input:

        1. Generate the Information Block:

                Create a K-bit binary sequence (e.g., random 512 bits).

                Input block (512 bits)

        2. LDPC Decoding:

                Apply the LDPC decoding algorithm based on your selected base graph and lifting size (Zc).

                The output decoded block will be 1536 bits for this example (because the code rate is 1/3, the decoded size is
                three times the input size).

        3. Encoded Output:

                After LDPC decoding with rate 1/3, the decoded block will contain the original 512 information bits and 1024
                parity bits.

                Output block (1536 bits)


        Verifying the LDPC Algorithm:
                - Run your LDPC decoder on this input block.

                - Ensure that the output block is exactly 3 times the input size, and validate the decoded bits by running the
                  LDPC decoder (to ensure proper decoding and error correction).


        Focus on K for your LDPC test input rather than Kb.

                - K is the size of the actual information block (i.e., the number of information bits you want to decode).
                - Kb is a scaling factor used to compute the maximum allowed size of the information block for a given base graph (BG)
                  and lifting size (Zc).

        For a fixed input block size of 512 bits, len_filler_bits can indeed be a fixed value, but it depends on the specific base
        graph (BG) and lifting size (Zc) being used. Once these parameters has been choosen, the required block size K' will remain
        constant, and thus the number of filler bits will be the same for all blocks of size 512 bits.

        How the filler bits work for fixed block sizes:

        1. Base Graph (BG) and Lifting Size (Zc):

                - The base graph (BG1 or BG2) determines the scaling factor Kb.
                - The lifting size (Zc) adjusts the total block size.

        2. Fixed Input Block Size (K = 512):

                - If it is being consistently used a 512-bit input block, it only need to calculate the filler bits once, based on Zc and
                  Kb for the base graph chosen.

        Example with Fixed Block Size:

        Let’s assume:
                - Base Graph: BG1
                - Lifting Size (Zc): 128
                - Kb: 22 (a typical value for BG1)

        1. Calculate the required block size K':
                K' = 22×128 = 2816 bits

        2. Calculate the filler bits for a 512-bit input block:
                Filler Bits = K'−K = 2816−512 = 2304 bits

        Thus, for 512-bit blocks, the value of len_filler_bits will always be 2304 for this configuration (BG1, Zc = 128).

        (or)

        For Testing the LDPC Algorithm:

        1. K is the primary value to work with.

                - K represents the number of information bits in the input block. For example, it may be chose K = 512 bits for the test.
                - This is the actual number of bits that are decoding in the LDPC decoder.

        2. Filler Bits:

                - If the required block size (derived from Kb and Zc) is larger than K, it is needed filler bits to pad the input block.
                - The number of filler bits is calculated as:
                        Filler Bits = K' − K
                  where K' is the required input block size, and K is the size of your test input block.

        Example:
                - Let's say K = 512 bits is the test input size.
                - If it is being used the Base Graph 1 (BG1) with Zc = 128 and Kb = 22, the required block size would be:
                        K'= 22×128 = 2816 bits
                - Since the test input K is 512 bits, it would need:
                        Filler Bits = 2816−512 = 2304 bits
                The filler bits will be padded with zeros.
*/

/*      unsigned float coding_rate;                     - this parameter is not required in ArmRAL ldpd encoder signature

        Encoded Block Size (N): for code rate R = 1/3, the encoded block size is: N=K/R = 512x3 = 1536 bits.

        Implicit Coding Rate Calculation:
        The input size and the selected base graph (BG1 or BG2) determine the size of the output block. The ratio of these two
        (output size/input size) will give the coding rate.

        For example, if the input block is 512 bits and the output block is 1536 bits, the coding rate is 512/1536 = 1/3.

        Why It’s Not an Explicit Parameter:
        Since the coding rate is a derived property from the base graph, lifting size, and block lengths, the function does not
       need an explicit coding rate parameter. Instead, the coding rate is controlled by the selection of these parameters.
*/

        /* VBrusse - OAI ldpc decoder interface - LDPCdecoder.c function input parameters (GitLab) */

/*      const unsigned char *inputArray = input[0];
        unsigned char *outputArray = output[0];
        const int Zc = encod_params->Zc;
        const int Kb = encod_params->Kb;
        const short block_length = encod_params->K;
        const short BG = encod_params->BG;
        const uint8_t gen_code = encod_params->gen_code;
        uint8_t c[22 * 384]; //padded input, unpacked, max size
        uint8_t d[68 * 384]; // coded output, unpacked, max size
*/
int32_t nrLDPC_decod_offloading(t_nrLDPC_dec_params *p_decParams,
                                uint8_t harq_pid,
                                uint8_t ulsch_id,
                                uint8_t C,
                                int8_t *p_llr,
                                int8_t *p_out,
                                t_nrLDPC_time_stats *p_time_stats,
                                decode_abort_t *ab)
{
        struct comch_config cfg;
        const char *server_name = "nrLDPC_decod_server";
        doca_error_t result;
        struct doca_log_backend *sdk_log;
        int exit_status = EXIT_FAILURE;


        int argc = 3;                                                   /* VBrusse */
        char *argv[] = {
                "nrLDPC_decod_client",
                "-p",
                "03:00.0"                                               /* VBrusse - PCIe address = "03:00.0", representor address = "b1:00.0" */
        };                                                              /* client connection with -p, server connection with -p and -r */

        /* Calculate the p_llr buffer size according to ArmRAL documentation. i.e. it shall be calculate as length 68 * Z for BG=1 and 52 * Z for BG=2. */
        /* Maximum Z (Lifting Factor / Lifting Size) = 384 */
        int N = 0;
        int Z = p_decParams->Z;

        if (p_decParams->BG == 1 || p_decParams->BG == 2) {
                if (p_decParams->BG == 1)
                        N = 68 * Z;
                if (p_decParams->BG == 2)
                        N = 52 * Z;
        } else {
                printf("\n[nrLDPC_decod] Received a BG value different from 1 or 2.\n");
                goto sample_exit;
        }


        printf("\n[nrLDPC_decod_offloading] *** Start the nrLDPC_decod_offloading function ... ***\n");
        printf("\n");
        printf("BG = %d\n", p_decParams->BG);
        printf("Z = %d\n", p_decParams->Z);
        printf("R = %d\n", p_decParams->R);
        printf("numMaxIter = %d\n", p_decParams->numMaxIter);
        printf("Kprime = %d\n", p_decParams->Kprime);
        printf("outMode = %d\n", p_decParams->outMode);
        printf("crc_type = %d\n", p_decParams->crc_type);
        /* int (*check_crc)(uint8_t* decoded_bytes, uint32_t n, uint8_t crc_type); */ /**< Parity check function */
        printf("\n");
        printf("*** harq_pid = %d\n", harq_pid);
        printf("*** ulsch_id = %d\n", ulsch_id);
        /* printf("*** C = %d\n", C); */                        /* 'C' is not an argument relevant for LDPC decoding */
        printf("*** N = %d\n", N);
        printf("\n");

        printf("[nrLDPC_decod_offloading] ====================> p_llr - LLRs (Log-Likelihood Ratios) - soft values <====================\n");
        for (int i = 0; i < N; i++) {
                printf("%d ", *(p_llr + i));
        }



        // Step 1: Correctly calculate the total memory needed for the array
        // size_t total_size = N * sizeof(int8_t);

        // Step 2: Allocate the memory and assign it to the pointer
        // We're assigning the result of malloc directly to the struct member.
        // cfg.ldpc_decod_params.llrs = (int8_t *)malloc(total_size);

        // It's good practice to check if the allocation was successful
/*      if (cfg.ldpc_decod_params.llrs == NULL) {
                // Handle the allocation error (e.g., print an error and exit)
                goto sample_exit;
        }
*/
        // Step 3: Now the pointer is valid. Use it directly for memset or memcpy.
        // You do NOT need the '&...[0]' part when using a pointer.
        // memset(cfg.ldpc_decod_params.llrs, 0, N);
        // memcpy(cfg.ldpc_decod_params.llrs, p_llr, N);



        /* memset(&cfg.ldpc_decod_params.llrs[0], 0, CC_LDPC_IN_BLOCK_LEN); */          /* Use memset and memcpy instead of loop for eficiency */
        memset(&cfg.ldpc_decod_params.llrs[0], 0, N);
        /* memcpy(&cfg.ldpc_decod_params.llrs[0], p_llr, CC_LDPC_IN_BLOCK_LEN); */
        memcpy(&cfg.ldpc_decod_params.llrs[0], p_llr, N);

        size_t k_bytes = p_decParams->Kprime / 8;               // Kprime is given in bits. The total size of the decoded buffer in bytes.
                                                                // NOTE: This assumes kprime is a multiple of 8.

        if (p_decParams->Kprime % 8 != 0) {                     // Check for non-byte aligned input
                printf("[nrLDPC_decod_offloading] Kprime must be a multiple of 8 bits for byte-aligned access.");
                goto sample_exit;
        }

        // cfg.ldpc_decod_params.llrs = p_llr;

        cfg.ldpc_decod_params.n = N;

        cfg.ldpc_decod_params.kp = k_bytes;                     // kp value is the Kprime value in bytes

        cfg.ldpc_decod_params.bg = p_decParams->BG;

        cfg.ldpc_decod_params.z = p_decParams->Z;

        cfg.ldpc_decod_params.num_its = p_decParams->numMaxIter;

        cfg.ldpc_decod_params.crc_idx = 0;                      /* There is no CRC attached, set this to ARMRAL_LDPC_NO_CRC = 0 */

        printf("\n\n[nrLDPC_decod_offloading] ====================> cfg.ldpc_decod_params <====================\n");
        // for (int i = 0; i < cfg.ldpc_decod_params.n; i++) {
        // for (int i = 0; i < N; i++) {
                // printf("%d ", cfg.ldpc_decod_params.llrs[i]);
                // printf("%d ", *(cfg.ldpc_decod_params.llrs + i));
        // }
        printf("\n\n");
        printf("*** n = %d\n", cfg.ldpc_decod_params.n);
        printf("*** kp = %d\n", cfg.ldpc_decod_params.kp);
        printf("*** bg = %d\n", cfg.ldpc_decod_params.bg);
        printf("*** z = %d\n", cfg.ldpc_decod_params.z);
        printf("*** num_its = %d\n", cfg.ldpc_decod_params.num_its);
        printf("*** crc_idx = %d\n\n", cfg.ldpc_decod_params.crc_idx);



        /* Set the default configuration values, client so no need for the comch_dev_rep_pci_addr field */
        strcpy(cfg.comch_dev_pci_addr, DEFAULT_PCI_ADDR);

        /* Register a logger backend */
        result = doca_log_backend_create_standard();
        if (result != DOCA_SUCCESS)
                goto sample_exit;

        /* Register a logger backend for internal SDK errors and warnings */
        result = doca_log_backend_create_with_file_sdk(stderr, &sdk_log);
        if (result != DOCA_SUCCESS)
                goto sample_exit;
        result = doca_log_backend_set_sdk_level(sdk_log, DOCA_LOG_LEVEL_WARNING);
        if (result != DOCA_SUCCESS)
                goto sample_exit;

        DOCA_LOG_INFO("Starting the sample");

        /* Parse cmdline/json arguments */
        result = doca_argp_init("doca_comch_data_path_client", &cfg);
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to init ARGP resources: %s", doca_error_get_descr(result));
                goto sample_exit;
        }

        result = register_comch_params();
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to register CC client sample parameters: %s", doca_error_get_descr(result));
                goto argp_cleanup;
        }

        result = doca_argp_start(argc, argv);
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to parse sample input: %s", doca_error_get_descr(result));
                goto argp_cleanup;
        }

        /* Start the client */

        result = start_nrLDPC_decod_client(server_name, cfg.comch_dev_pci_addr, &cfg.ldpc_decod_params);
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to run sample: %s", doca_error_get_descr(result));
                goto argp_cleanup;
        }



        // Free the allocated memory to avoid a memory leak
        // free(cfg.ldpc_decod_params.llrs);

        // Return the decoded output in the pointer p_out using memcpy instead of loop for eficiency
        // 'Kprime' is the K' in the standard 3GPP TS 38.212 section 5.2.2. It is the number of the payload bits per uncoded segment.
        // In other word, it is the number of useful bits in the output of the decoder.

        memcpy(p_out, cfg.ldpc_decod_params.data_out, p_decParams->Kprime);
        // memcpy(p_out, cfg.ldpc_decod_params.data_out, CC_LDPC_OUT_BLOCK_LEN);

        exit_status = EXIT_SUCCESS;

argp_cleanup:
        doca_argp_destroy();
sample_exit:
        if (exit_status == EXIT_SUCCESS)
                DOCA_LOG_INFO("Sample finished successfully");
        else
                DOCA_LOG_INFO("Sample finished with errors");
        return exit_status;
}

/*
 * nrLDPC_decod - OpenAirInterfa (OAI) interface function to start/call the DOCA Communication
 * Channel Client API on host and offload the LDPC function on DPU.
 *
 * The function signature is provided by OAI. The function call is executed by OAI 5G gNB in
 * the High-PHY O-DU stack.
 *
 * @p_decParams [in]:
 * @harq_pid [in]:
 * @ulsch_id [in]:
 * @C [in]:
 * @p_llr [in]:
 * @p_out [out]:
 * @p_time_stats [in]:
 * @ab [in]:
 *
 * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
 */

/*
 * There are two interfaces to integrate with OAI 5G NR stack as stated in
 *
 *      https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/develop/openair1/PHY/CODING/DOC/LDPCImplementation.md
 *
 *              - LDPC slot coding
 *              - LDPC segment coding
 *
 * The interface used is the 'LDPC segment coding' defined in
 *
 *      /openairinterface5g/openair1/PHY/CODING/nrLDPC_defs.h
 *
 *      /openairinterface5g/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_types.h'
 *
 *      'Z' (Lifting Factor / Lifting Size) the valid values for 5G NR LDPC codes shall fall within the set of allowed Z
 *      values like {2, 4, 8, ..., 384}.
 *
 *      'Kprime' is the K' in the standard 3GPP TS 38.212 section 5.2.2. It is the number of the payload bits per uncoded segment.
 *      In other word, it is the number of useful bits in the output of the decoder.
 *      The number of LLR samples in the segment input to the decoder, it is N as defined in section 5.3.2 of 3GPP TS 38.212,
 *      OAI community said it is calculated from Z and and BG: N=66*Z if BG=1 and N=50*Z if BG=2.
 *      ArmRAL documentation indicated that it shall be calculate as length 68 * Z for BG=1 and 52 * Z for BG=2.
 *
 *      'harq_pid' is the index of the HARQ process wich is sequence of transmissions and retransmissions to try to successfully
 *      transmit some payload. The first call to the decoder will be with id 0. It can have other values if there are failed
 *      transmission and retransmissions that keep HARQ process 0 busy so that further data should use other HARQ processes.
 *
 *      'ulsch_id' is the index of the ULSCH process for which the decoding is performed. There is one ULSCH process for each UE
 *      connected to the network. In your case I guess there is only one UE connected which gets index 0.
 *      If had other UEs connected they will get the following indexes.
 *
 *      'C' is supposed to be the number of segments in transport block when it is segmented (see 3GPP TS 38.212 section 5.2.2).
 *      But it is not useful for decoding individual segments and it is then not used and set to zero.
 *
 *      'p_llr' buffer size = N
 *
 *      'p_out' buffer size = Kprime
 *
 *      The LLR length = codeword length (N). For structured LDPC (like 5G NR), compute N from the BG and Lifting Factor.
 *      Verify by checking the input array size in the decoder.
 *
 *      Definitions
 *
 *      Term                    Meaning                                                                         Format
 *
 *      (a) LLRs (Soft Input)   Noisy received signal represented as probabilities (log-likelihood ratios).     Floating-point (e.g., float llr[n])
 *      (b) Noisy Codeword      The physical-layer signal (e.g., after channel distortion) mapped to LLRs.      Soft values (LLRs)
 *      (c) Decoder Output      Estimated codeword (hard bits) after error correction.                          Binary (0/1)
 *
 *
 *      (a) LLRs (Soft Values)
 *              - mRepresent confidence in each bit being 0 or 1 after transmission through a noisy channel.
 *
 *              - Interpretation:
 *                      - LLR > 0: Higher probability of 0.
 *
 *                      - LLR < 0: Higher probability of 1.
 *
 *                      - Magnitude: Confidence level (larger = more reliable).
 *
 *              - Example:
 *                      llr = [+1.2, -0.8, -3.1, +0.5] -> Likely bits: [0, 1, 1, 0].
 *
 *      (b) Noisy Codeword
 *              - The received signal (before decoding) is a noisy version of the transmitted codeword.
 *
 *              - Converted to LLRs for soft-decoding.
 *
 *      (c) Decoder Output (Hard Values)
 *              - The LDPC decoder processes LLRs and outputs a valid codeword (binary) that satisfies H.c = 0.
 *
 *              - Steps:
 *                      1. Soft-Decoding: Uses LLRs to iteratively correct errors (e.g., belief propagation).
 *
 *                      2. Hard Decision: Final step converts LLRs to bits.
 *
 */
int32_t nrLDPC_decod(t_nrLDPC_dec_params *p_decParams,
                     uint8_t harq_pid,
                     uint8_t ulsch_id,
                     uint8_t C,
                     int8_t *p_llr,
                     int8_t *p_out,
                     t_nrLDPC_time_stats *p_time_stats,
                     decode_abort_t *ab)
{
        doca_error_t result;
        int exit_status = EXIT_FAILURE;


        printf("\n\n[nrLDPC_decod] *** nrLDPC_decod function has been called by the Rate dematching function of the DU High-PHY Layer - Uplink direction ***\n");
                                                                                        /* Start the LDPC decoder function offloading to DPU */
        result = nrLDPC_decod_offloading(p_decParams, harq_pid, ulsch_id, C, p_llr, p_out, p_time_stats, ab);

        if (result != DOCA_SUCCESS) {
                printf("[nrLDPC_decod] Failed to call the nrLDPC_decod_offloading function\n");
        }


        printf("[nrLDPC_decod] ====================> Final Decoder Output <====================\n");

        size_t k_bytes = p_decParams->Kprime / 8;

        for (int i = 0; i < k_bytes; i++) {
        // for (int i = 0; i < CC_LDPC_OUT_BLOCK_LEN; i++) {
                // printf("%d ", *(p_out + i));
                // printf("%u ", (uint8_t)*(p_out + i));
                printf("%02x ", (uint8_t)*(p_out + i));
        }
        printf("\n\n");



        exit_status = EXIT_SUCCESS;

        return exit_status;
}
