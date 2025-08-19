/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*
 * Original OAI include filename: /openairinterface5g/openair1/PHY/CODING/nrLDPC_defs.h
 *
 * Filename: nrLDPC_defs.h
 *
 * Include file: customized by: Vlademir Brusse
 *
 * Date: 2025/06/26
 *
 */

//============================================================================================================================

#ifndef __NRLDPC_DEFS__H__
#define __NRLDPC_DEFS__H__

#include <stdbool.h>

/*
 * From /openairinterface5g/openair1/PHY/CODING/nrLDPC_decoder/nrLDPCdecoder_defs.h
 */
#define NR_LDPC_ZMAX 384                /* Maximum lifting size */

#define NR_LDPC_NCOL_BG1 68             /* Number of columns in BG1 */

#define NR_LDPC_NROW_BG1 46             /* Number of rows in BG1 */

#define NR_LDPC_NCOL_BG2 52             /* Number of columns in BG2 */

#define NR_LDPC_NROW_BG2 42             /** Number of rows in BG2 */

#define NR_LDPC_MAX_NUM_LLR 27000       /* Maximum number of possible input LLR = NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX */

#define NR_LDPC_MAX_NUM_CB 144          /* Maximum number of segments per TB = MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER*NR_MAX_NB_LAYERS */


/*
 * From /openairinterface5g/common/utils/time_meas.h
 */
// structure to store data to compute cpu measurment
#if defined(__x86_64__) || defined(__i386__) || defined(__arm__) || defined(__aarch64__)
  typedef long long oai_cputime_t;
#else
  #error "building on unsupported CPU architecture"
#endif

/*
 * From /openairinterface5g/common/utils/time_meas.h
 */
typedef void(*meas_printfunc_t)(const char* format, ...);
typedef struct {
  int               msgid;                  /*!< \brief message id, as defined by TIMESTAT_MSGID_X macros */
  int               timestat_id;            /*!< \brief points to the time_stats_t entry in cpumeas table */
  oai_cputime_t  ts;                        /*!< \brief time stamp */
  meas_printfunc_t  displayFunc;            /*!< \brief function to call when DISPLAY message is received*/
} time_stats_msg_t;

/*
 * From /openairinterface5g/openair1/PHY/CODING/nrLDPC_defs.h
 */

/**
   Structure containing LDPC parameters per CB
*/
typedef struct nrLDPC_params_per_cb {
  uint32_t E_cb;
  uint8_t status_cb;
  uint8_t* p_status_cb;
} nrLDPC_params_per_cb_t;

/**
   Enum with possible LDPC output formats.
 */
typedef enum nrLDPC_outMode {
    nrLDPC_outMode_BIT, /**< 32 bits per uint32_t output */
    nrLDPC_outMode_BITINT8, /**< 1 bit per int8_t output */
    nrLDPC_outMode_LLRINT8 /**< Single LLR value per int8_t output */
} e_nrLDPC_outMode;

/**
   Structure containing LDPC decoder parameters.
 */
typedef struct nrLDPC_dec_params {
    uint8_t BG; /**< Base graph */
    uint16_t Z; /**< Lifting size */
    uint8_t R; /**< Decoding rate: Format 15,13,... for code rates 1/5, 1/3,... */
    uint8_t numMaxIter; /**< Maximum number of iterations */
    int Kprime; /**< Size of the payload bits and CRC bits in the code block */
    e_nrLDPC_outMode outMode; /**< Output format */
    int crc_type; /**< Size and type of the parity check bits (16, 24A or 24B) */
    int (*check_crc)(uint8_t* decoded_bytes, uint32_t n, uint8_t crc_type); /**< Parity check function */
} t_nrLDPC_dec_params;

/*
 * From /openairinterface5g/openair1/PHY/defs_common.h
 */
typedef struct {
  pthread_mutex_t mutex_failure;
  bool failed;
} decode_abort_t;

/*
 * From /openairinterface5g/common/utils/time_meas.h
 */
struct notifiedFIFO_elt_s;
typedef struct time_stats {
  oai_cputime_t in;          /*!< \brief time at measure starting point */
  oai_cputime_t diff;        /*!< \brief average difference between time at starting point and time at endpoint*/
  oai_cputime_t p_time;      /*!< \brief absolute process duration */
  double diff_square;        /*!< \brief process duration square */
  oai_cputime_t max;         /*!< \brief maximum difference between time at starting point and time at endpoint*/
  int trials;                /*!< \brief number of start point - end point iterations */
  int meas_flag;             /*!< \brief 1: stop_meas not called (consecutive calls of start_meas) */
  char *meas_name;           /*!< \brief name to use when printing the measure (not used for PHY simulators)*/
  int meas_index;            /*!< \brief index of this measure in the measure array (not used for PHY simulators)*/
  int meas_enabled;         /*!< \brief per measure enablement flag. send_meas tests this flag, unused today in start_meas and stop_meas*/
  struct notifiedFIFO_elt_s *tpoolmsg; /*!< \brief message pushed to the cpu measurment queue to report a measure START or STOP */
  time_stats_msg_t *tstatptr;   /*!< \brief pointer to the time_stats_msg_t data in the tpoolmsg, stored here for perf considerations*/
} time_stats_t;

/*
 * From /openairinterface5g/openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_types.h
 */
/**
   Structure containing LDPC decoder processing time statistics.
 */
#ifndef CODEGEN
typedef struct nrLDPC_time_stats {
    time_stats_t llr2llrProcBuf; /**< Statistics for function llr2llrProcBuf */
    time_stats_t llr2CnProcBuf; /**< Statistics for function llr2CnProcBuf */
    time_stats_t cnProc; /**< Statistics for function cnProc */
    time_stats_t cnProcPc; /**< Statistics for function cnProcPc */
    time_stats_t bnProcPc; /**< Statistics for function bnProcPc */
    time_stats_t bnProc; /**< Statistics for function bnProc */
    time_stats_t cn2bnProcBuf; /**< Statistics for function cn2bnProcBuf */
    time_stats_t bn2cnProcBuf; /**< Statistics for function bn2cnProcBuf */
    time_stats_t llrRes2llrOut; /**< Statistics for function llrRes2llrOut */
    time_stats_t llr2bit; /**< Statistics for function llr2bit */
    time_stats_t total; /**< Statistics for total processing time */
} t_nrLDPC_time_stats;
#endif

/*
 * From /openairinterface5g/common/utils/threadPool/task_ans.h
 */
#include <semaphore.h>

extern sem_t sfn_semaphore;

/*
 * From /openairinterface5g/common/utils/threadPool/task_ans.h
 */
#define LEVEL1_DCACHE_LINESIZE 64

typedef struct {
  sem_t sem;
  _Alignas(LEVEL1_DCACHE_LINESIZE) _Atomic(int) counter;
} task_ans_t;



/*
 * From /openairinterface5g/openair1/PHY/CODING/nrLDPC_defs.h
 */
// encoder interface

/**
   \brief LDPC encoder parameter structure
   \var n_segments number of segments in the transport block
   \var first_seg index of the first segment of the subset to encode
   within the transport block
   \var gen_code flag to generate parity check code
   0 -> encoding
   1 -> generate parity check code with AVX2
   2 -> generate parity check code without AVX2
   \var tinput time statistics for data input in the encoder
   \var tprep time statistics for data preparation in the encoder
   \var tparity time statistics for adding parity bits
   \var toutput time statistics for data output from the encoder
   \var K size of the complete code segment before encoding
   including payload, CRC bits and filler bit
   (also known as Kr, see 38.212-5.2.2)
   \var Kb number of lifting sizes to fit the payload (see 38.212-5.2.2)
   \var Zc lifting size (see 38.212-5.2.2)
   \var F number of filler bits (see 38.212-5.2.2)
   \var harq pointer to the HARQ process structure
   \var BG base graph index
   \var output output buffer after INTERLEAVING
   \var ans pointer to the task answer
   to notify thread pool about completion of the task
*/
typedef struct {
  unsigned int n_segments; // optim8seg
  unsigned int first_seg;  // optim8segmulti
  unsigned char gen_code;  //orig
  time_stats_t *tinput;
  time_stats_t *tprep;
  time_stats_t *tparity;
  time_stats_t *toutput;
  /// Size in bits of the code segments
  uint32_t K;
  /// Number of lifting sizes to fit the payload
  uint32_t Kb;
  /// Lifting size
  uint32_t Zc;
  /// Number of "Filler" bits
  uint32_t F;
  /// Encoder BG
  uint8_t BG;
  /// Interleaver outputs
  unsigned char *output;
  task_ans_t *ans;
} encoder_implemparams_t;

typedef int32_t(LDPC_initfunc_t)(void);
typedef int32_t(LDPC_shutdownfunc_t)(void);
// decoder interface
/**
   \brief LDPC decoder API type definition
   \param p_decParams LDPC decoder parameters
   \param p_llr Input LLRs
   \param p_llrOut Output vector
   \param p_profiler LDPC profiler statistics
*/

typedef int32_t(LDPC_decoderfunc_t)(t_nrLDPC_dec_params *p_decParams,
                                    uint8_t harq_pid,
                                    uint8_t ulsch_id,
                                    uint8_t C,
                                    int8_t *p_llr,
                                    int8_t *p_out,
                                    t_nrLDPC_time_stats *,
                                    decode_abort_t *ab);
typedef int32_t(LDPC_encoderfunc_t)(uint8_t **, uint8_t *, encoder_implemparams_t *);

#endif
