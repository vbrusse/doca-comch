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
 * Original filename: comch_data_path_high_speed_client_sample.c
 *
 * Filename: nrLDPC_encod_client.c
 *
 * DOCA Communication Channel Client API customized by: Vlademir Brusse
 *
 * Date: 2024/12/18
 *
 */

#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <doca_buf.h>
#include <doca_buf_inventory.h>
#include <doca_comch.h>
#include <doca_comch_consumer.h>
#include <doca_comch_producer.h>
#include <doca_ctx.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_log.h>
#include <doca_mmap.h>
#include <doca_pe.h>

#include "comch_ctrl_path_common.h"
#include "nrLDPC_common.h"
/* #include "nrLDPC_encod_common.h"                     VBrusse */
#include "common.h"

DOCA_LOG_REGISTER(NRLDPC_ENCOD_CLIENT);

/* Sample's objects */
struct comch_data_path_client_objects {
        struct doca_dev *hw_dev;                   /* Device used in the sample */
        struct doca_pe *pe;                        /* PE object used in the sample */
        struct doca_comch_client *client;          /* Client object used in the sample */
        struct doca_comch_connection *connection;  /* CC connection object used in the sample */
        doca_error_t client_result;                /* Holds result will be updated in client callbacks */
        bool client_finish;                        /* Controls whether client progress loop should be run */
        bool data_path_test_started;               /* Indicate whether we can start data_path test */
        bool data_path_test_stopped;               /* Indicate whether we can stop data_path test */
        struct comch_data_path_objects *data_path; /* Data path objects */
};

/**
 * Callback for client send task successful completion
 *
 * @task [in]: Send task object
 * @task_user_data [in]: User data for task
 * @ctx_user_data [in]: User data for context
 */
static void client_send_task_completion_callback(struct doca_comch_task_send *task,
                                                 union doca_data task_user_data,
                                                 union doca_data ctx_user_data)
{
        struct comch_data_path_client_objects *sample_objects;

        (void)task_user_data;

        sample_objects = (struct comch_data_path_client_objects *)(ctx_user_data.ptr);
        sample_objects->client_result = DOCA_SUCCESS;



/*      DOCA_LOG_INFO("Client enviou mensagem: %s", sample_objects->data_path->text);               VBrusse */



        DOCA_LOG_INFO("Client task sent successfully");
        doca_task_free(doca_comch_task_send_as_task(task));
}

/**
 * Callback for client send task completion with error
 *
 * @task [in]: Send task object
 * @task_user_data [in]: User data for task
 * @ctx_user_data [in]: User data for context
 */
static void client_send_task_completion_err_callback(struct doca_comch_task_send *task,
                                                     union doca_data task_user_data,
                                                     union doca_data ctx_user_data)
{
        struct comch_data_path_client_objects *sample_objects;

        (void)task_user_data;

        sample_objects = (struct comch_data_path_client_objects *)(ctx_user_data.ptr);
        sample_objects->client_result = doca_task_get_status(doca_comch_task_send_as_task(task));
        DOCA_LOG_ERR("Message failed to send with error = %s", doca_error_get_name(sample_objects->client_result));
        doca_task_free(doca_comch_task_send_as_task(task));
        (void)doca_ctx_stop(doca_comch_client_as_ctx(sample_objects->client));
}

/**
 * Callback for client message recv event
 *
 * @event [in]: Recv event object
 * @recv_buffer [in]: Message buffer
 * @msg_len [in]: Message len
 * @comch_connection [in]: Connection the message was received on
 */
static void client_message_recv_callback(struct doca_comch_event_msg_recv *event,
                                         uint8_t *recv_buffer,
                                         uint32_t msg_len,
                                         struct doca_comch_connection *comch_connection)
{
        union doca_data user_data;
        struct doca_comch_client *comch_client;
        struct comch_data_path_client_objects *sample_objects;
        doca_error_t result;

        (void)event;

        DOCA_LOG_INFO("Message received: '%.*s'", (int)msg_len, recv_buffer);                   /* Recebe a resposta do server no recv_buffer */


/*
        if (strncmp(CC_LDPC_RES, (char *)recv_buffer, 1) == 0) {                                VBrusse
                DOCA_LOG_INFO("Cliente recebeu mensagem do Servidor");
        }
*/


        comch_client = doca_comch_client_get_client_ctx(comch_connection);

        result = doca_ctx_get_user_data(doca_comch_client_as_ctx(comch_client), &user_data);
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to get user data from ctx with error = %s", doca_error_get_name(result));
                return;
        }

        sample_objects = (struct comch_data_path_client_objects *)(user_data.ptr);

        if ((msg_len == strlen(STR_START_DATA_PATH_TEST)) &&
            (strncmp(STR_START_DATA_PATH_TEST, (char *)recv_buffer, msg_len) == 0))
                sample_objects->data_path_test_started = true;
        else if ((msg_len == strlen(STR_STOP_DATA_PATH_TEST)) &&
                 (strncmp(STR_STOP_DATA_PATH_TEST, (char *)recv_buffer, msg_len) == 0)) {
                sample_objects->data_path_test_stopped = true;
                sample_objects->data_path->remote_consumer_id = INVALID_CONSUMER_ID;
                (void)doca_ctx_stop(doca_comch_client_as_ctx(sample_objects->client));
        }
}

/**
 * Client sends a message to server
 *
 * @sample_objects [in]: The sample object to use
 * @msg [in]: The msg to send
 * @len [in]: The msg length
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t client_send_msg(struct comch_data_path_client_objects *sample_objects, const char *msg, size_t len)
{
        doca_error_t result;
        struct doca_comch_task_send *task;



        /* strcpy((char *)msg, CC_LDPC_REQ); */                                                      /* VBrusse */
        /* len = CC_LDPC_MSG_SIZE; */
        /* DOCA_LOG_INFO("Cliente enviou essa mensagem: '%.*s'", (int)len, msg);                        VBrusse inseriu */



        result = doca_comch_client_task_send_alloc_init(sample_objects->client,
                                                        sample_objects->connection,
                                                        (void *)msg,                            /* VBrusse: Formata msg a ser enviada ao server */
                                                        len,
                                                        &task);
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to allocate client task with error = %s", doca_error_get_name(result));
                return result;
        }

        result = doca_task_submit(doca_comch_task_send_as_task(task));                          /* Envia msg ao server */
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to send client task with error = %s", doca_error_get_name(result));
                doca_task_free(doca_comch_task_send_as_task(task));
                return result;
        }

        return DOCA_SUCCESS;
}

/**
 * Callback triggered whenever CC client context state changes
 *
 * @user_data [in]: User data associated with the CC client context.
 * @ctx [in]: The CC client context that had a state change
 * @prev_state [in]: Previous context state
 * @next_state [in]: Next context state (context is already in this state when the callback is called)
 */
static void client_state_changed_callback(const union doca_data user_data,
                                          struct doca_ctx *ctx,
                                          enum doca_ctx_states prev_state,
                                          enum doca_ctx_states next_state)
{
        (void)ctx;
        (void)prev_state;

        struct comch_data_path_client_objects *sample_objects = (struct comch_data_path_client_objects *)user_data.ptr;

        switch (next_state) {
        case DOCA_CTX_STATE_IDLE:
                DOCA_LOG_INFO("CC client context has been stopped");
                /* We can stop progressing the PE */
                sample_objects->client_finish = true;
                break;
        case DOCA_CTX_STATE_STARTING:
                /**
                 * The context is in starting state, need to progress until connection with server is established.
                 */
                DOCA_LOG_INFO("CC client context entered into starting state. Waiting for connection establishment");
                break;
        case DOCA_CTX_STATE_RUNNING:
                /* Get a connection channel */
                if (sample_objects->connection == NULL) {
                        sample_objects->client_result =
                                doca_comch_client_get_connection(sample_objects->client, &sample_objects->connection);
                        if (sample_objects->client_result != DOCA_SUCCESS) {
                                DOCA_LOG_ERR("Failed to get connection from cc client with error = %s",
                                             doca_error_get_name(sample_objects->client_result));
                                (void)doca_ctx_stop(doca_comch_client_as_ctx(sample_objects->client));
                        }
                        DOCA_LOG_INFO("CC client context is running. Get a connection from server");
                }
                break;
        case DOCA_CTX_STATE_STOPPING:
                /**
                 * The context is in stopping, this can happen when fatal error encountered or when stopping context.
                 * doca_pe_progress() will cause all tasks to be flushed, and finally transition state to idle
                 */
                DOCA_LOG_INFO("CC client context entered into stopping state. Waiting for connection termination");
                break;
        default:
                break;
        }
}

/**
 * Callback for new consumer arrival event
 *
 * @event [in]: New remote consumer event object
 * @comch_connection [in]: The connection related to the consumer
 * @id [in]: The ID of the new remote consumer
 */
static void new_consumer_callback(struct doca_comch_event_consumer *event,
                                  struct doca_comch_connection *comch_connection,
                                  uint32_t id)
{
        union doca_data user_data;
        struct doca_comch_client *comch_client;
        struct comch_data_path_client_objects *sample_objects;
        doca_error_t result;

        (void)event;

        comch_client = doca_comch_client_get_client_ctx(comch_connection);

        result = doca_ctx_get_user_data(doca_comch_client_as_ctx(comch_client), &user_data);
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to get user data from ctx with error = %s", doca_error_get_name(result));
                return;
        }

        sample_objects = (struct comch_data_path_client_objects *)(user_data.ptr);
        sample_objects->data_path->remote_consumer_id = id;

        DOCA_LOG_INFO("Got a new remote consumer with ID = [%d]", id);
}

/**
 * Callback for expired consumer arrival event
 *
 * @event [in]: Expired remote consumer event object
 * @comch_connection [in]: The connection related to the consumer
 * @id [in]: The ID of the expired remote consumer
 */
void expired_consumer_encod_callback(struct doca_comch_event_consumer *event,
                                      struct doca_comch_connection *comch_connection,
                                      uint32_t id)
{
        /* These arguments are not in use */
        (void)event;
        (void)comch_connection;
        (void)id;
}

/**
 * Clean all sample resources
 *
 * @sample_objects [in]: Sample objects struct to clean
 */
static void clean_comch_data_path_client_objects(struct comch_data_path_client_objects *sample_objects)
{
        doca_error_t result;
        struct timespec ts = {
                .tv_sec = 0,
                .tv_nsec = SLEEP_IN_NANOS,
        };

        if (sample_objects == NULL)
                return;

        /* Verify client is not already stopped due to a server error */
        if (sample_objects->client_finish == false) {
                /* Exchange message with server to make connection is reliable */
                sample_objects->client_result =
                        client_send_msg(sample_objects, STR_STOP_DATA_PATH_TEST, strlen(STR_STOP_DATA_PATH_TEST));
                if (sample_objects->client_result != DOCA_SUCCESS) {
                        DOCA_LOG_ERR("Failed to submit send task with error = %s",
                                     doca_error_get_name(sample_objects->client_result));
                        (void)doca_ctx_stop(doca_comch_client_as_ctx(sample_objects->client));
                }
                while (sample_objects->data_path_test_stopped == false) {
                        if (doca_pe_progress(sample_objects->pe) == 0)
                                nanosleep(&ts, &ts);
                }
                while (sample_objects->client_finish == false) {
                        if (doca_pe_progress(sample_objects->pe) == 0)
                                nanosleep(&ts, &ts);
                }
        }

        clean_comch_ctrl_path_client(sample_objects->client, sample_objects->pe);
        sample_objects->client = NULL;
        sample_objects->pe = NULL;

        if (sample_objects->hw_dev != NULL) {
                result = doca_dev_close(sample_objects->hw_dev);
                if (result != DOCA_SUCCESS)
                        DOCA_LOG_ERR("Failed to close hw device properly with error = %s", doca_error_get_name(result));

                sample_objects->hw_dev = NULL;
        }
}




/* VBrusse
 * terminate_comch_data_path_encod_client - This function terminates the producer and consumer
 * instances freeing their resources allocated by host (client) used by LDPC offloading.
 *
 * @data_path [in]: data/input parameters of the LDPC encoding.
 */
void terminate_comch_data_path_encod_client(struct comch_data_path_objects *data_path)
{
        /* Free consumer's mmap and doca_buf resources */
        clean_comch_consumer(data_path->consumer, data_path->consumer_pe);
        data_path->consumer = NULL;
        data_path->consumer_pe = NULL;
        clean_local_mem_bufs(&data_path->consumer_mem);

        /* Free producer's mmap and doca_buf resources */
        clean_comch_producer(data_path->producer, data_path->producer_pe);
        data_path->producer = NULL;
        data_path->producer_pe = NULL;
        clean_local_mem_bufs(&data_path->producer_mem);
}




/**
 * Initialize sample resources
 *
 * @server_name [in]: Server name to connect to
 * @dev_pci_addr [in]: PCI address to connect over
 * @pldpc_encod_params [in]: Pointer to structure to send to server
 * @sample_objects [in]: Sample objects struct to initialize
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
static doca_error_t init_comch_data_path_client_objects(const char *server_name,
                                                        const char *dev_pci_addr,
                                                        struct ldpc_encod_params_t *pldpc_encod_params,
                                                        struct comch_data_path_client_objects *sample_objects)
{
        doca_error_t result;
        struct comch_ctrl_path_client_cb_config client_cb_cfg = {
                .send_task_comp_cb = client_send_task_completion_callback,
                .send_task_comp_err_cb = client_send_task_completion_err_callback,
                .msg_recv_cb = client_message_recv_callback,
                .data_path_mode = true,
                .new_consumer_cb = new_consumer_callback,
                .expired_consumer_cb = expired_consumer_encod_callback,
                .ctx_user_data = sample_objects,
                .ctx_state_changed_cb = client_state_changed_callback};
        struct timespec ts = {
                .tv_sec = 0,
                .tv_nsec = SLEEP_IN_NANOS,
        };

        /* sample_objects->data_path->text = text;                                      VBrusse: comment */



        /* sample_objects->data_path->pldpc_enc_pars = pldpc_encod_params;      VBrusse */
        /* DOCA_LOG_INFO("Cliente envia data_path->pldpc_enc_pars ao Servidor"); */
/*      DOCA_LOG_INFO("*** Input Block ===> %s", (char *)((struct ldpc_encod_params_t *)(sample_objects->data_path->pldpc_enc_pars))->inputBlock);
        DOCA_LOG_INFO("*** bg = %d", ((struct ldpc_encod_params_t *)(sample_objects->data_path->pldpc_enc_pars))->bg);
        DOCA_LOG_INFO("*** z = %d", ((struct ldpc_encod_params_t *)(sample_objects->data_path->pldpc_enc_pars))->z);
        DOCA_LOG_INFO("*** k = %d", ((struct ldpc_encod_params_t *)(sample_objects->data_path->pldpc_enc_pars))->k);
        DOCA_LOG_INFO("*** len_filler_bits = %d", ((struct ldpc_encod_params_t *)(sample_objects->data_path->pldpc_enc_pars))->len_filler_bits);
*/



        /* Open DOCA device according to the given PCI address */
        result = open_doca_device_with_pci(dev_pci_addr, NULL, &(sample_objects->hw_dev));
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to open Comm Channel DOCA device based on PCI address");
                return result;
        }
        sample_objects->data_path->hw_dev = sample_objects->hw_dev;

        /* Init CC client */
        result = init_comch_ctrl_path_client(server_name,
                                             sample_objects->hw_dev,
                                             &client_cb_cfg,
                                             &(sample_objects->client),
                                             &(sample_objects->pe));
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to init cc client with error = %s", doca_error_get_name(result));
                goto close_hw_dev;
        }
        sample_objects->data_path->pe = sample_objects->pe;

        /* Wait connection establishment */
        while (sample_objects->connection == NULL && sample_objects->client_finish == false) {
                if (doca_pe_progress(sample_objects->pe) == 0)
                        nanosleep(&ts, &ts);
        }

        if (sample_objects->client_finish == true) {
                clean_comch_data_path_client_objects(sample_objects);
                return DOCA_ERROR_INITIALIZATION;
        }

        sample_objects->data_path->connection = sample_objects->connection;

        /* Exchange message with server, to make connection is reliable */
        sample_objects->client_result =
                client_send_msg(sample_objects, STR_START_DATA_PATH_TEST, strlen(STR_START_DATA_PATH_TEST));
        if (sample_objects->client_result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to submit send task with error = %s",
                             doca_error_get_name(sample_objects->client_result));
                (void)doca_ctx_stop(doca_comch_client_as_ctx(sample_objects->client));
                goto destroy_client;
        }
        while (sample_objects->data_path_test_started == false) {
                if (doca_pe_progress(sample_objects->pe) == 0)
                        nanosleep(&ts, &ts);
        }

        return DOCA_SUCCESS;

destroy_client:
        clean_comch_ctrl_path_client(sample_objects->client, sample_objects->pe);
close_hw_dev:
        (void)doca_dev_close(sample_objects->hw_dev);
        return result;
}

/**
 * Run comch_data_path_client sample
 *
 * @server_name [in]: Server name to connect to
 * @dev_pci_addr [in]: PCI address to connect over
 * @ldpc_encod_params [in/out]: Address of the structure to send to server
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t start_nrLDPC_encod_client(const char *server_name,
                                       const char *dev_pci_addr,
                                       struct ldpc_encod_params_t *pldpc_encod_params)
{
        doca_error_t result;
        struct comch_data_path_client_objects sample_objects = {0};
        struct comch_data_path_objects data_path = {0};




        /* VBrusse */
        printf("*** [start_nrLDPC_encod_client] Input Block (pldpc_encod_params) ===> %s\n", (char *)pldpc_encod_params->inputBlock);
        printf("*** BG = %d\n", pldpc_encod_params->bg);
        printf("*** Zc = %d\n", pldpc_encod_params->z);
        printf("*** K = %d\n", pldpc_encod_params->k);
        printf("*** F = %d\n\n", pldpc_encod_params->len_filler_bits);



        data_path.pldpc_enc_pars = (struct ldpc_encod_params_t *)pldpc_encod_params;    /* VBrusse - 2024/12/12 - area de memoria alocada para conter os ldpc data */
                                                                /* =======> testar se ha necessidade desse comando, uma vez que a area de memoria foi alocada na DU */
        data_path.pldpc_dec_pars = NULL;
        data_path.size_ldpc_data = sizeof(struct ldpc_encod_params_t);



        sample_objects.data_path = &data_path;

        result = init_comch_data_path_client_objects(server_name, dev_pci_addr, pldpc_encod_params, &sample_objects);
        if (result != DOCA_SUCCESS) {
                DOCA_LOG_ERR("Failed to initialize sample with error = %s", doca_error_get_name(result));
                return result;
        }


        result = comch_data_path_send_msg(&data_path);
        if (result != DOCA_SUCCESS)
                goto exit;


        /* VBrusse */
        DOCA_LOG_INFO("\n\n\n*** ANTES DO recv_msg - Input Block - nrLDPC_encod_client ===> %s", (char *)((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->inputBlock);
        DOCA_LOG_INFO("*** bg = %d", ((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->bg);
        DOCA_LOG_INFO("*** z = %d", ((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->z);
        DOCA_LOG_INFO("*** k = %d", ((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->k);
        DOCA_LOG_INFO("*** len_filler_bits = %d\n\n", ((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->len_filler_bits);




        result = comch_data_path_recv_msg(&data_path);
        if (result != DOCA_SUCCESS)
                goto exit;



        /* VBrusse */
        /* DOCA_LOG_INFO("\n\n\n*** =====> nrLDPC_encod_client DEPOIS do recv_msg <====="); */
        /* DOCA_LOG_INFO("*** Input Block ===> %s", (char *)((struct ldpc_encod_params_t *)(data_path->pldpc_enc_pars))->inputBlock); */

        /* DOCA_LOG_INFO("*** Input Block ===> %s", (char *)((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->inputBlock); */
        /* DOCA_LOG_INFO("*** bg = %d", ((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->bg); */
        /* DOCA_LOG_INFO("*** z = %d", ((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->z); */
        /* DOCA_LOG_INFO("*** k = %d", ((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->k); */
        /* DOCA_LOG_INFO("*** len_filler_bits = %d", ((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->len_filler_bits); */
        /* DOCA_LOG_INFO("*** Output Block ===> %s\n\n", (char *)((struct ldpc_encod_params_t *)(data_path.pldpc_enc_pars))->outputBlock); */

        /* added by VBrusse */
        *pldpc_encod_params = *(struct ldpc_encod_params_t *)data_path.pldpc_enc_pars;
        terminate_comch_data_path_encod_client(&data_path);                           /* free resources allocated by host */




exit:
        clean_comch_data_path_client_objects(&sample_objects);

        return result != DOCA_SUCCESS ? result : sample_objects.client_result;
}
