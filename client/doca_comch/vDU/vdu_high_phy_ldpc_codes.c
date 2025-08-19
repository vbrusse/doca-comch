/*
 * Test component for offloading of the LDPC encoder/decoder of the High-PHY Layer of the vDU.
 *
 * Author: Vlademir Brusse
 *
 * Date: 2025/06/26
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <pthread.h>

#include <nrLDPC_defs.h>

/* single code block - a random binary sequence of length 512 */
/* a 512-bit input block */
#define INPUT_BLOCK_512 "00010010001000011001111100010101000001100010011110100101000010100101000001001100000101110101011110000001010011011001011000110010001101110001100010100101100111000010000101101111101101111000101001011011111011001001001111011001111011000001011101011100101000100011011011100000010101010010111011001001000100001110110000011110111111100100011011010110100001001111010101001010011100010011011001001111111111111011000001101010110100010011011000111111011001001011110001110001000100000010101100011011101101011011101000010110"
/* 128-bit binary sequence */
#define INPUT_BLOCK_128 "10110101010111010110100111011001000110011011111101001100110110011011101000100110111011001010011110010001010101000110111011011101"


/* OAI LDPC Interfaces */

/* OAI 5G NR - LDPC encoding function signature */
int32_t nrLDPC_encod(uint8_t **inputArr, uint8_t *outputArr, encoder_implemparams_t *impp);

/* OAI 5G NR - LDPC decoding function signature */
int32_t nrLDPC_decod(t_nrLDPC_dec_params *p_decParams,
                                    uint8_t harq_pid,
                                    uint8_t ulsch_id,
                                    uint8_t C,
                                    int8_t *p_llr,
                                    int8_t *p_out,
                                    t_nrLDPC_time_stats *,
                                    decode_abort_t *ab);
/*
 * Example CRC check function
 *
 */
int example_crc_check(uint8_t* decoded_bytes, uint32_t n, uint8_t crc_type) {
    /* Implement the CRC checking logic here */
    return 0; /* Placeholder */
}

/*
 * Component: High PHY layer of the vDU.
 *
 * vdu_high_phy_ldpc_codes - LDPC encoder/decoder Test component of the chaining pipeline at High PHY Layer of the O-DU.
 * The test purpose is to call the nrLDPC_encod/nrLDPC_decod function with the arguments required by the OAI interface.
 * nrLDPC_encod and nrLDPC_decod are implemented as DOCA Communication Channel API clients, they do the offloading of the
 * 5G NR workload from CPU to the DPU through the hw accelerators ConnectX adapter (NIC) and the PCIe channel/interface to compute
 * the ArmRAL LDPC encoder kernel inside DPU.
 *
 * @argc: 1
 * @argv[0]: vdu_high_phy_ldpc_codes
 * @argv[1]: 0 (encoding) / 1 (decoding)
 *
 * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
 *
 *
 * Command line:        $./vdu_high_phy_ldpc_codes 0            (ldpc encoding)
 *                      $./vdu_high_phy_ldpc_codes 1            (ldpc decoding)
 *
 */
int main(int argc, char **argv)
{
        int32_t result = 0;

        /*********** Encoding input data **********/
        uint8_t inputBlock[10560];
        uint8_t outputBlock[10560];
        encoder_implemparams_t enc_params = {
                .BG = 0,                                        /* Encoder BG - BG1 */
                .Zc = 8,                                        /* Lifting size */
                .K = 128,                                       /* Size in bits of the code segments */
                .Kb = 4,                                        /* Number of lifting sizes to fit the payload */
                .F = 48,                                        /* Number of "Filler" bits */
        };
        uint8_t *pinput = inputBlock;                           /* pointer to the start of the inputBlock array */
        uint8_t *poutput = outputBlock;


        /*********** Decoding input data **********/

                                                                        /* Assuming CC_LDPC_BLOCK_LEN is 12 for BG1 */
        /* int8_t llrs_array[12] = {15, -5, 15, -5, 5, -15, 5, -15, 15, -5, 5, -15}; */

        int8_t llrs_array[128] = {1, 0, 0, 0, 0, 0, 0, 0, -95, -66, -127, -54, -24, 97, 0, 0, 48, 9, 0, 0, 13, 0, 0, 0, 0, 61, 9, -68, 116, 117, 0, 0, -128, -40, 43, -4, -24, 97, 0, 0, -64, -82, -3, -32, 116, 117, 0, 0, -32, -45, 43, -4, -24, 97};
        int8_t data_out_array[408] = {0};                       /* Ensure it's cleared before use */
        t_nrLDPC_dec_params dec_params = {
                .BG = 1,                                        // Base graph - BG1 = 0, BG2 = 1
                // .Z = 8,                                      // Lifting Size
                .Z = 96,
                .R = 13,                                        // Code rate (e.g., 13 for 1/3)
                .numMaxIter = 10,                               // Maximum iterations
                // .Kprime = 256,
                .Kprime = 904,
                .outMode = nrLDPC_outMode_LLRINT8,              // Output format
                .crc_type = 2,                                  // Example CRC type
/*              .check_crc = example_crc_check,                 // Pointer to CRC check function
                .setCombIn = 0,                                 // Example combination input flag
                .perCB = {                                      // Initialize each element in the array
                        { .E_cb = 0, .status_cb = 0, .p_status_cb = NULL },
                        { .E_cb = 0, .status_cb = 0, .p_status_cb = NULL },
                        { .E_cb = 0, .status_cb = 0, .p_status_cb = NULL },
                        { .E_cb = 0, .status_cb = 0, .p_status_cb = NULL },
                        { .E_cb = 0, .status_cb = 0, .p_status_cb = NULL },
                        { .E_cb = 0, .status_cb = 0, .p_status_cb = NULL },
                        { .E_cb = 0, .status_cb = 0, .p_status_cb = NULL },
                        { .E_cb = 0, .status_cb = 0, .p_status_cb = NULL }
                } */
        };

        uint8_t harq_pid = 0;
        uint8_t ulsch_id = 0;
        uint8_t C = 0;
        int8_t *p_llr = &llrs_array[0];
        int8_t *p_out = &data_out_array[0];
        t_nrLDPC_time_stats *time_stats = NULL;
        decode_abort_t ab = {0};


/*
        typedef struct nrLDPC_dec_params {
                uint8_t BG;                         *< Base graph
                uint16_t Z;                         *< Lifting size
                uint8_t R;                          *< Decoding rate: Format 15,13,... for code rates 1/5, 1/3,...
                uint16_t F;                         *< Filler bits
                uint8_t Qm;                         *< Modulation
                uint8_t rv;
                uint8_t numMaxIter;                 *< Maximum number of iterations
                int E;
                e_nrLDPC_outMode outMode;           *< Output format
                int crc_type;
                int (*check_crc)(uint8_t* decoded_bytes, uint32_t n, uint8_t crc_type);
                uint8_t setCombIn;
                nrLDPC_params_per_cb_t perCB[NR_LDPC_MAX_NUM_CB];
        } t_nrLDPC_dec_params;
*/

/*
struct oai_decoder_params_t {                                           VBrusse - structure to store the OAI nrLDCP_decod function's data/input parameters
        t_nrLDPC_dec_params *p_decPars;                                 p_decParams
        uint8_t h_pid;                                                  harq_pid
        uint8_t u_id;                                                   ulsch_id
        uint8_t C_par;                                                  C
        int8_t *llr;                                                    p_llr
        int8_t *out;                                                    p_out
        t_nrLDPC_time_stats *p_t_stats;                                 p_time_stats
        decode_abort_t *u_ab;                                           ab
};
*/

/*
        struct ldpc_decod_params_t {                                    VBrusse: structure to store the ldpc decoding data/input parameters
                const int8_t *llrs;                                     Pointer for LLRs (soft values)
                armral_ldpc_graph_t bg;                                 Base graph type. LDPC_BASE_GRAPH_1 = 0, LDPC_BASE_GRAPH_2 = 1
                uint32_t z;                                             Lifting size
                uint32_t crc_idx;                                       CRC index
                uint32_t num_its;                                       Number of iterations
                uint8_t *data_out;                                      Pointer to store decoded output
        };
*/


        if (argc != 2) {
                printf("1 Parameter: 0 (encoding) / 1 (decoding)\n");
                return result;
        }


        /* Using *argv++: Iterate through all arguments:
        printf(("*** argv[1] = %s\n", *argv++); */

        /* Using argv[1]: Access a specific argument without modifying the pointer: */
        /* printf("*** argv[1] = %s\n", argv[1]); */


        if (strncmp("0", argv[1], 1) == 0) {                    /* LDPC encoding */

                /*********** Logic for Encoding **********/

                printf("***** [vdu_high_phy_ldpc_codes] Start the test of the LDPC encoder\n\n");

                strcpy((char *)inputBlock, INPUT_BLOCK_512);            /* input code block */

                printf("***** [vdu_high_phy_ldpc_codes] Input Block: %s\n\n", pinput);

                /* result = nrLDPC_encod(&pinput, &poutput, &enc_params); */
                printf("i\n\n\nVOU CHAMAR A nrLDPC_encod\n\n\n");
                result = nrLDPC_encod(&pinput, poutput, &enc_params);

                if (result != 0) {
                        printf("[vdu_high_phy_ldpc_codes] Failed to call the nrLDPC_encod function\n");
                        return result;
                }

                printf("\n\n\n***** [vdu_high_phy_ldpc_codes] Input Block ===> %s\n", (char *)pinput);
                // printf("\n***** [vdu_high_phy_ldpc_codes] poutput ===> %p\n", poutput);
                printf("\n***** [vdu_high_phy_ldpc_codes] Output Block (codeword) ===> %s\n", (char *)poutput);


                printf("\n***** [vdu_high_phy_ldpc_codes] End of the test of the LDPC encoder\n");


        } else {                                                        /* argv[1] = "1" - LDPC decoding */

                if (strncmp("1", argv[1], 1) == 0) {                     /* LDPC decoding */
                        /*********** Logic for Decoding **********/

                        /* For BG1: The most common value for z is 6.
                        For BG2: The most common value for z is 168. */

                        /* Assuming CC_LDPC_BLOCK_LEN is 12 for BG1 */
/*                      int8_t llrs_array[12] = {15, -5, 15, -5, 5, -15, 5, -15, 15, -5, 5, -15}; */
/*                      llr = &llrs_array;                       Pointer for LLRs (soft values) */

                        /* uint8_t data_out[CC_LDPC_CODE_RATE * CC_LDPC_BLOCK_LEN];*/
                        /* uint32_t code_rate = 1/2; // Code rate for BG1 */
                        /* For BG1, a common code rate is 1/2. This means that for a block length of 12, the data_out array should have a size of 6. */
                        /* ArmRAL: The decoded bits. These are of length 68 * z for base graph 1 and 52 * z for base graph 2. It is assumed that the array data_out is able to store this many bits. */
                        /* z=6, 68x6=408 */
/*                      int8_t data_out_array[408] = {0};                        Ensure it's cleared before use */
/*                      oai_dec_data.out = &data_out_array; */

                        /* Set up the struct with initialized values */
/*      struct ldpc_decod_params_t dec_params = {
                .llrs = llrs_array,
                .bg = LDPC_BASE_GRAPH_2,        Example base graph value, adjust to your case. LDPC_BASE_GRAPH_1 = 0, LDPC_BASE_GRAPH_2 = 1
                .z = 6,                         Example lifting size value
                .crc_idx = 0,                   Example CRC index, adjust as needed - No CRC for BG1, ARMRAL_LDPC_NO_CRC 0
                .num_its = 10,                  Example iteration count
                .data_out = data_out_array      Pointer to store decoded output
        };
*/
                        printf("***** [vdu_high_phy_ldpc_codes] Start the test of the LDPC decoder\n\n");

                        printf("[vdu_high_phy_ldpc_codes] ====================> LLRs (Log-Likelihood Ratios) values <====================\n");
                        for (int i = 0; i < 128; i++) {
                                printf("%d ", *(p_llr + i));
                        }

                        printf("\n\n");

                        /* result = nrLDPC_decod(&dec_params, harq_pid, ulsch_id, C, p_llr, p_out, time_stats, &ab); */
                        result = nrLDPC_decod(&dec_params, harq_pid, ulsch_id, C, p_llr, p_out, time_stats, &ab);

                        if (result != 0) {
                                printf("[vdu_high_phy_ldpc_codes] Failed to call the nrLDPC_decod function\n");
                                return result;
                        }

                        printf("[vdu_high_phy_ldpc_codes] ==========> Decoding Output <==========\n");
                        for (int i= 0; i < 128; i++) {
                                // printf("%d ", *(p_out + i));
                                printf("%u ", (uint8_t)*(p_out + i));
                        }
                        printf("\n");

                        printf("\n***** [vdu_high_phy_ldpc_codes] End of the test of the LDPC decoder\n");
                }
        }

        return result;
}
