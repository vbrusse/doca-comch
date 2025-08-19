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
 * Filename: nrLDPC_encod.c
 *
 * DOCA Communication Channel Client API customized by: Vlademir Brusse
 *
 * Date: 2025/07/30
 */

#include <stdlib.h>

#include <doca_argp.h>
#include <doca_dev.h>
#include <doca_log.h>

#include "comch_ctrl_path_common.h"

#define DEFAULT_PCI_ADDR "b1:00.0"
#define DEFAULT_MESSAGE "Message from the client"                                         /* VBrusse */


/* single block - a random binary sequence of length 512 */
/* a 512-bit input block */
#define INPUT_BLOCK_512 "00010010001000011001111100010101000001100010011110100101000010100101000001001100000101110101011110000001010011011001011000110010001101110001100010100101100111000010000101101111101101111000101001011011111011001001001111011001111011000001011101011100101000100011011011100000010101010010111011001001000100001110110000011110111111100100011011010110100001001111010101001010011100010011011001001111111111111011000001101010110100010011011000111111011001001011110001110001000100000010101100011011101101011011101000010110"
/* 128-bit binary sequence */
#define INPUT_BLOCK_128 "10110101010111010110100111011001000110011011111101001100110110011011101000100110111011001010011110010001010101000110111011011101"
/* 112-bit binary sequence */
#define INPUT_BLOCK_112 "101101100101110011011011011100010010110110110010111000100101101110010101111001001010111001110011"
/* 64-bit binary sequence A */
#define INPUT_BLOCK_64_A "1101101001110100101011110011010110100111010111101110000101101100"
/* 64-bit binary sequence B */
#define INPUT_BLOCK_64_B "1101010101100101001110111001101001100011010101011101110100101010"
#define FILLER_BITS_FOR_BLOCK_LEN 2304                                          /* Filler Bits for BG1, Zc = 128, Kb = 22 and a 512-bit input block */



// ALIAS DECLARATION
// LDPCencoder declared as an alias for nrLDPC_encod
extern int32_t LDPCencoder(uint8_t **input, uint8_t *output, encoder_implemparams_t *pencod_params)
        __attribute__((alias("nrLDPC_encod")));
// END ALIAS



DOCA_LOG_REGISTER(NRLDPC_ENCOD_CLIENT::MAIN);

/* DOCA comch client's logic */
doca_error_t start_nrLDPC_encod_client(const char *server_name,
                                       const char *dev_pci_addr,
                                       struct ldpc_encod_params_t *pldpc_encod_params);


/*
 * nrLDPC_encod_offloading - This host function starts/calls the Offloading Service as a task
 * exposured by "nrLDPC_encod_server" to compute the 5G NR LDPC function on the DPU over a
 * PCIe channel established through DOCA Communication Channel API client using the ConnectX
 * adapter to run on DPU the LDPC encoder kernel implemented/provided by ArmRAL.
 * It makes the function offloading and leverages the DPU's hardware acceleration capabilities
 * to efficiently execute the LDPC encoding task, potentially improving performance and reducing
 * the CPU load.
 *
 * @input [in]:
 * @output [out]:
 * @encod_params [in]:
 *
 * Information Block Size (K)
 *      K represents the size of the information block, i.e., the number of information bits that
 *      are input to the LDPC encoder (the number of information bits before LDPC encoding).
 *      These bits form the portion of the transmitted codeword that carries actual user data.
 *      For each base graph (BG1 and BG2) in 5G NR, the size of K varies based on the chosen code
 *      rate and the lifting size Z.
 *      Higher values of K result in larger codewords, which can affect the efficiency of error
 *      correction and the system's overall throughput.
 *
 * Base Information Block Size (Kb)
 *      Kb is a parameter related to the base graph (BG) used for LDPC encoding. It defines the
 *      maximum size of the information block before the encoder starts lifting (expanding the
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
 *      Zc is the lifting factor that expands the base graph matrix used in LDPC encoding. In 5G NR, two
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
 *              In LDPC encoding, a base graph is selected (BG1 or BG2), which is a small, predefined matrix
 *              that specifies the connections between information bits and parity bits.
 *              The Zc parameter controls how the base graph is "lifted" into a much larger matrix by replacing
 *              each entry in the base graph with either a zero matrix or a circularly shifted identity matrix
 *              of size Zc.
 *              This lifting process determines the final structure of the LDPC codeword, including the number
 *              of parity bits and the overall size of the encoded block.
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
 *      In LDPC encoding, filler bits are used to pad the input block when the size of the information block K is smaller
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
 *      The higher code rate reduces the number of parity bits generated during LDPC encoding, which makes BG1 more
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
 * The choice between BG1 and BG2 in the LDPC encoding process is made based on the size of the transport block and the
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
 *      In the structure encoder_implemparams_t, the field gen_code typically refers to a generator matrix code or a flag
 *      related to code generation in the context of LDPC encoding.
 *
 * gen_code might represent in this context
 *      Generator Matrix:
 *              In LDPC encoding, the generator matrix (often denoted as G) is used to generate the codewords from the input
 *              information bits. When gen_code is set, it might instruct the encoder to either use a precomputed generator
 *              matrix or dynamically generate one for encoding the information block.
 *      Code Generation Flag:
 *              It could also be a flag that signals whether the implementation should generate the LDPC code for the specific
 *              block being processed. This could be relevant if the encoder needs to dynamically adjust based on the block size,
 *              code rate, or base graph.
 *      Implementation Optimization:
 *              In certain implementations, gen_code might be used for switching between different encoding methods or
 *              optimizations. For example, it might select between using a systematic encoder (where the information bits appear
 *              directly in the codeword) or a non-systematic one.
 *
 * The specific function of gen_code could vary according to the implementation, but it generally relates to code generation logic
 * in the LDPC encoding process.
 *
 * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
 */

        /* VBrusse - OAI interface */
        /*
        Steps to Create the Test Input:

        1. Generate the Information Block:

                Create a K-bit binary sequence (e.g., random 512 bits).

                Input block (512 bits)

        2. LDPC Encoding:

                Apply the LDPC encoding algorithm based on your selected base graph and lifting size (Zc).

                The output encoded block will be 1536 bits for this example (because the code rate is 1/3, the encoded size is
                three times the input size).

        3. Encoded Output:

                After LDPC encoding with rate 1/3, the encoded block will contain the original 512 information bits and 1024
                parity bits.

                Output block (1536 bits)


        Verifying the LDPC Algorithm:
                - Run your LDPC encoder on this input block.

                - Ensure that the output block is exactly 3 times the input size, and validate the encoded bits by running the
                  LDPC decoder (to ensure proper decoding and error correction).


        Focus on K for your LDPC test input rather than Kb.

                - K is the size of the actual information block (i.e., the number of information bits you want to encode).
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
                - This is the actual number of bits that are encoding in the LDPC encoder.

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

        /* VBrusse - OAI ldpc encoder interface - LDPCencoder.c function input parameters (GitLab) */

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
int32_t nrLDPC_encod_offloading(uint8_t **inputArr, uint8_t *outputArr, encoder_implemparams_t *impp)
{
        struct comch_config cfg = {0};                  /* Antes de incluir a estrutura ldpc nao precisava inicializar */
        const char *server_name = "nrLDPC_encod_server";
        doca_error_t result;
        struct doca_log_backend *sdk_log;
        int exit_status = EXIT_FAILURE;


        int argc = 3;                                                   /* VBrusse */
        char *argv[] = {
                "nrLDPC_encod_client",
                "-p",
                "03:00.0"                                               /* VBrusse - pcie address = "03:00.0", representor address = "b1:00.0" */
        };                                                              /* client connection with -r, server connection with -p and -r */

        struct oai_encoder_params_t oai_ldpc_encod = {                  /* VBrusse: the useful ldpc encoder input data */
                .inputArray = *inputArr,                                /* single code block iinput as a sequence of bits to be transmitted */
                .outputArray = outputArr,                               /* ldpc output as a sequence of encoded bits (codeword) */
                .BG = impp->BG,                                         /* selected Base Graph (BG) */
                .Zc = impp->Zc,                                         /* Lifting Size (Zc) */
                .K = impp->K,                                           /* Size in bits of the code segments (K) */
                .Kb = impp->Kb,                                         /* Number of lifting sizes to fit the payload (Kb) */
                .F = impp->F                                            /* Number of "Filler" bits */
        };


/*      oai_ldpc_encod.inputArray = *inputArr;                          single code block iinput as a sequence of bits to be transmitted */
/*      oai_ldpc_encod.outputArray = outputArr;                         ldpc output as a sequence of encoded bits (codeword) */
/*      oai_ldpc_encod.BG = impp->BG;                                   selected Base Graph (BG) */
/*      oai_ldpc_encod.Zc = impp->Zc;                                   Lifting Size (Zc) */
/*      oai_ldpc_encod.K = impp->K;                                     Size in bits of the code segments (K) */
/*      oai_ldpc_encod.Kb = impp->Kb;                                   Number of lifting sizes to fit the payload (Kb) */
/*      oai_ldpc_encod.F = impp->F;                                     Number of "Filler" bits */


        /* cfg.ldpc_encod_params.inputBlock = oai_ldpc_encod.inputArray;        VBrusse */
        strcpy((char *)cfg.ldpc_encod_params.inputBlock, (char *)oai_ldpc_encod.inputArray);
        /* cfg.ldpc_encod_params.outputBlock = oai_ldpc_encod.outputArray; */
        cfg.ldpc_encod_params.bg = oai_ldpc_encod.BG;
        cfg.ldpc_encod_params.z = oai_ldpc_encod.Zc;
        cfg.ldpc_encod_params.k = oai_ldpc_encod.K;
        // cfg.ldpc_encod_params.kb = oai_ldpc_encod.Kb;
        cfg.ldpc_encod_params.len_filler_bits = oai_ldpc_encod.F;

        printf("*** [nrLDPC_encod_offloading] Input Block (cfg.ldpc_encod_params) ===> %s\n", (char *)cfg.ldpc_encod_params.inputBlock);
        printf("*** BG = %d\n", cfg.ldpc_encod_params.bg);
        printf("*** Zc = %d\n", cfg.ldpc_encod_params.z);
        printf("*** K = %d\n", cfg.ldpc_encod_params.k);
        // printf("*** Kb = %d\n", cfg.ldpc_encod_params.kb);
        printf("*** F = %d\n\n", cfg.ldpc_encod_params.len_filler_bits);




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



        /* strcpy((char *)cfg.ldpc_encod_params.inputBlock, INPUT_BLOCK_128);              VBrusse
        DOCA_LOG_INFO("\n\n\n*** Input Block ===> %s", (char *)cfg.ldpc_encod_params.inputBlock);
        DOCA_LOG_INFO("*** bg = %d", cfg.ldpc_encod_params.bg);
        DOCA_LOG_INFO("*** z = %d", cfg.ldpc_encod_params.z);
        DOCA_LOG_INFO("*** k = %d", cfg.ldpc_encod_params.k);
        DOCA_LOG_INFO("*** kb = %d", cfg.ldpc_encod_params.kb);
        DOCA_LOG_INFO("*** len_filler_bits = %d\n\n", cfg.ldpc_encod_params.len_filler_bits); */



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
        result = start_nrLDPC_encod_client(server_name, cfg.comch_dev_pci_addr, &cfg.ldpc_encod_params);
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to run sample: %s", doca_error_get_descr(result));
                goto argp_cleanup;
        }




        // *outputArr = &cfg.ldpc_encod_params.outputBlock[0];

        memcpy(outputArr, cfg.ldpc_encod_params.outputBlock, 128);

        /* \n\n\n*** ==========> nrLDPC_encod <========== ***");
        DOCA_LOG_INFO("*** Input Block ===> %s", (char *)cfg.ldpc_encod_params.inputBlock);
        DOCA_LOG_INFO("*** bg = %d", cfg.ldpc_encod_params.bg);
        DOCA_LOG_INFO("*** z = %d", cfg.ldpc_encod_params.z);
        DOCA_LOG_INFO("*** k = %d", cfg.ldpc_encod_params.k);
        DOCA_LOG_INFO("*** kb = %d", cfg.ldpc_encod_params.kb);
        DOCA_LOG_INFO("*** len_filler_bits = %d", cfg.ldpc_encod_params.len_filler_bits);
        DOCA_LOG_INFO("\n\n*** AQUI - Output Block ===> %s\n\n", (char *)cfg.ldpc_encod_params.outputBlock);    */


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
 * nrLDPC_encod - OpenAirInterfa (OAI) interface function to start/call the DOCA Communication
 * Channel Client API on host and offload the LDPC function on DPU.
 *
 * The function signature is provided by OAI. The function call is executed by OAI 5G gNB in
 * the High-PHY O-DU stack.
 *
 * @input [in]:
 *      This parameter is a pointer to a pointer of uint8_t. It represents a two-dimensional array
 *      (or a dynamically allocated array of arrays). In the context of an LDPC encoder function,
 *      this represents the input data to be encoded, where each row represents a codeword or a
 *      message bit sequence.
 *
 * @output [out]:
 *      This parameter is a pointer to a pointer of uint8_t, used to represent the output of the
 *      encoding process. It signifies where the encoded output (the encoded data or codewords)
 *      will be stored after the encoding function processes the input data.
 *
 * @pencod_params [in]:
 *      This parameter is a pointer to a structure of type encoder_implemparams_t. This structure
 *      likely contains various parameters or settings that configure the behavior of the LDPC encoder.
 *      This third parameter allows the encoder function to be flexible and adaptable to different
 *      encoding scenarios by providing contextual information that might be necessary for the
 *      encoding process.
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
 *      https://gitlab.eurecom.fr/oai/openairinterface5g/-/blob/develop/openair1/PHY/CODING/nrLDPC_defs.h
 *
 */
int32_t nrLDPC_encod(uint8_t **input, uint8_t *output, encoder_implemparams_t *pencod_params)
{
        int exit_status = EXIT_FAILURE;


        /* printf("\n\n\n*** [nrLDPC_encod] Input Block ===> %s\n", (char *)input); */
        printf("*** [nrLDPC_encod] Input Block ===> %s\n", (char *)*input);
        printf("*** BG = %d\n", pencod_params->BG);
        printf("*** Zc = %d\n", pencod_params->Zc);
        printf("*** K = %d\n", pencod_params->K);
        printf("*** K = %d\n", pencod_params->Kb);
        printf("*** F = %d\n\n", pencod_params->F);


        exit_status = nrLDPC_encod_offloading(input, output, pencod_params);


        exit_status = EXIT_SUCCESS;
        if (exit_status != EXIT_SUCCESS) {
                printf("[nrLDPC_encod] Failed to call the nrLDPC_encod_offloading function\n");
        }

        DOCA_LOG_INFO("\n\n[nrLDPC_encod] ==========> Block encoded <========== ***");
        DOCA_LOG_INFO("*** Output Block ===> %s\n\n", (char *)output);



        return (exit_status == EXIT_FAILURE ? EXIT_FAILURE : EXIT_SUCCESS);
}
