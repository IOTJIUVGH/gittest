/*
 * AWS IoT Device SDK for Embedded C 202012.01
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Demo for showing the use of MQTT APIs to establish an MQTT session,
 * subscribe to a topic, publish to a topic, receive incoming publishes,
 * unsubscribe from a topic and disconnect the MQTT session.
 *
 * A mutually authenticated TLS connection is used to connect to the AWS IoT
 * MQTT message broker in this example. Define ROOT_CA_CERT_PATH for server
 * authentication in the client. Client authentication can be achieved in either
 * of the 2 different ways mentioned below.
 * 1. Define CLIENT_CERT_PATH and CLIENT_PRIVATE_KEY_PATH in demo_config.h
 *    for client authentication to be done based on the client certificate
 *    and client private key. More details about this client authentication
 *    can be found in the link below.
 *    https://docs.aws.amazon.com/iot/latest/developerguide/client-authentication.html
 * 2. Define CLIENT_USERNAME and CLIENT_PASSWORD in demo_config.h for client
 *    authentication to be done using a username and password. More details about
 *    this client authentication can be found in the link below.
 *    https://docs.aws.amazon.com/iot/latest/developerguide/enhanced-custom-authentication.html
 *    An authorizer setup needs to be done, as mentioned in the above link, to use
 *    username/password based client authentication.
 *
 * The example is single threaded and uses statically allocated memory;
 * it uses QOS1 and therefore implements a retransmission mechanism
 * for Publish messages. Retransmission of publish messages are attempted
 * when a MQTT connection is established with a session that was already
 * present. All the outgoing publish messages waiting to receive PUBACK
 * are resent in this demo. In order to support retransmission all the outgoing
 * publishes are stored until a PUBACK is received.
 */

/* Standard includes. */
#include <assert.h>

/* POSIX includes. */
#include "mxos.h"

/* Include Demo Config as the first non-system header. */
#include "mqtt_config.h"

/* MQTT API headers. */
#include "core_mqtt.h"
#include "core_mqtt_state.h"

/* OpenSSL sockets transport implementation. */
#include "openssl_posix.h"

/*Include backoff algorithm header for retry logic.*/
#include "backoff_algorithm.h"

/* Clock for timer. */
#include "clock.h"

#include "philips_param.h"
#include "philips_device_model.h"
#include "philips_log.h"
#include "philips_device_upload_manage.h"
#include "philips_mqtt_api.h"
#include "philips_https_request.h"

#define app_log(M, ...)             philips_custom_log("MQTT", M, ##__VA_ARGS__)

#define MAX_TOPIC_LENGTH						128
#define WILL_TOPIC_SIZE							64
#define WILL_MSG_SIZE							300
char topic_shadow_update[MAX_TOPIC_LENGTH] = {0};
char topic_shadow_get[MAX_TOPIC_LENGTH] = {0};

static mos_queue_id_t mqtt_send_msg_queue = NULL;

static mos_thread_id_t aws_iot_mqtt_thread = NULL;

static bool aws_iot_mqtt_thread_exit = false;

static uint32_t mqtt_connect_continuous_fail_cnt = 0; //wifi log

/**
 * @brief The maximum number of retries for connecting to server.
 */
#define CONNECTION_RETRY_MAX_ATTEMPTS            ( 5U )

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying connection to server.
 */
#define CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS    ( 20000U )

/**
 * @brief The base back-off delay (in milliseconds) to use for connection retry attempts.
 */
#define CONNECTION_RETRY_BACKOFF_BASE_MS         ( 2000U )

/**
 * @brief Timeout for receiving CONNACK packet in milli seconds.
 */
#define CONNACK_RECV_TIMEOUT_MS                  ( 1000U )

/**
 * @brief Timeout for MQTT_ProcessLoop function in milliseconds.
 */
#define MQTT_PROCESS_LOOP_TIMEOUT_MS        ( 500U )

/**
 * @brief The maximum time interval in seconds which is allowed to elapse
 *  between two Control Packets.
 *
 *  It is the responsibility of the Client to ensure that the interval between
 *  Control Packets being sent does not exceed the this Keep Alive value. In the
 *  absence of sending any other Control Packets, the Client MUST send a
 *  PINGREQ Packet.
 */
// #define MQTT_KEEP_ALIVE_INTERVAL_SECONDS    ( 60U )
#define MQTT_KEEP_ALIVE_INTERVAL_SECONDS    ( 35U )

/**
 * @brief Number of PUBLISH messages sent per iteration.
 */
#define MQTT_PUBLISH_COUNT_PER_LOOP         ( 5U )

/**
 * @brief Delay in seconds between two iterations of subscribePublishLoop().
 */
#define MQTT_SUBPUB_LOOP_DELAY_SECONDS      ( 5U )

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
// #define TRANSPORT_SEND_RECV_TIMEOUT_MS      ( 500 )
#define TRANSPORT_SEND_RECV_TIMEOUT_MS      ( 5000 )

/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/

/**
 * @brief Packet Identifier generated when Subscribe request was sent to the broker;
 * it is used to match received Subscribe ACK to the transmitted subscribe.
 */
static uint16_t globalSubscribePacketIdentifier = 0U;

/**
 * @brief Packet Identifier generated when Unsubscribe request was sent to the broker;
 * it is used to match received Unsubscribe ACK to the transmitted unsubscribe
 * request.
 */
static uint16_t globalUnsubscribePacketIdentifier = 0U;

/**
 * @brief Array to keep subscription topics.
 * Used to re-subscribe to topics that failed initial subscription attempts.
 */
static MQTTSubscribeInfo_t pGlobalSubscriptionList[ 2 ];

/**
 * @brief The network buffer must remain valid for the lifetime of the MQTT context.
 */
static uint8_t buffer[ NETWORK_BUFFER_SIZE ];

/**
 * @brief Status of latest Subscribe ACK;
 * it is updated every time the callback function processes a Subscribe ACK
 * and accounts for subscription to a single topic.
 */
static MQTTSubAckStatus_t globalSubAckStatus = MQTTSubAckFailure;

/*-----------------------------------------------------------*/

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    OpensslParams_t * pParams;
};

/*-----------------------------------------------------------*/

/**
 * @brief The random number generator to use for exponential backoff with
 * jitter retry logic.
 *
 * @return The generated random number.
 */
static uint32_t generateRandomNumber();

/**
 * @brief Connect to MQTT broker with reconnection retries.
 *
 * If connection fails, retry is attempted after a timeout.
 * Timeout value will exponentially increase until maximum
 * timeout value is reached or the number of attempts are exhausted.
 *
 * @param[out] pNetworkContext The output parameter to return the created network context.
 *
 * @return EXIT_FAILURE on failure; EXIT_SUCCESS on successful connection.
 */
static int connectToServerWithBackoffRetries( NetworkContext_t * pNetworkContext );

/**
 * @brief A function that connects to MQTT broker,
 * subscribes a topic, publishes to the same
 * topic MQTT_PUBLISH_COUNT_PER_LOOP number of times, and verifies if it
 * receives the Publish message back.
 *
 * @param[in] pMqttContext MQTT context pointer.
 * @param[in,out] pClientSessionPresent Pointer to flag indicating if an
 * MQTT session is present in the client.
 *
 * @return EXIT_FAILURE on failure; EXIT_SUCCESS on success.
 */
static int subscribePublishLoop( MQTTContext_t * pMqttContext,
                                 bool * pClientSessionPresent );

/**
 * @brief The function to handle the incoming publishes.
 *
 * @param[in] pPublishInfo Pointer to publish info of the incoming publish.
 * @param[in] packetIdentifier Packet identifier of the incoming publish.
 */
static void handleIncomingPublish( MQTTPublishInfo_t * pPublishInfo,
                                   uint16_t packetIdentifier );

/**
 * @brief The application callback function for getting the incoming publish
 * and incoming acks reported from MQTT library.
 *
 * @param[in] pMqttContext MQTT context pointer.
 * @param[in] pPacketInfo Packet Info pointer for the incoming packet.
 * @param[in] pDeserializedInfo Deserialized information from the incoming packet.
 */
static void eventCallback( MQTTContext_t * pMqttContext,
                           MQTTPacketInfo_t * pPacketInfo,
                           MQTTDeserializedInfo_t * pDeserializedInfo );

/**
 * @brief Initializes the MQTT library.
 *
 * @param[in] pMqttContext MQTT context pointer.
 * @param[in] pNetworkContext The network context pointer.
 *
 * @return EXIT_SUCCESS if the MQTT library is initialized;
 * EXIT_FAILURE otherwise.
 */
static int initializeMqtt( MQTTContext_t * pMqttContext,
                           NetworkContext_t * pNetworkContext );

/**
 * @brief Sends an MQTT CONNECT packet over the already connected TCP socket.
 *
 * @param[in] pMqttContext MQTT context pointer.
 * @param[in] createCleanSession Creates a new MQTT session if true.
 * If false, tries to establish the existing session if there was session
 * already present in broker.
 * @param[out] pSessionPresent Session was already present in the broker or not.
 * Session present response is obtained from the CONNACK from broker.
 *
 * @return EXIT_SUCCESS if an MQTT session is established;
 * EXIT_FAILURE otherwise.
 */
static int establishMqttSession( MQTTContext_t * pMqttContext,
                                 bool createCleanSession,
                                 bool * pSessionPresent );

/**
 * @brief Close an MQTT session by sending MQTT DISCONNECT.
 *
 * @param[in] pMqttContext MQTT context pointer.
 *
 * @return EXIT_SUCCESS if DISCONNECT was successfully sent;
 * EXIT_FAILURE otherwise.
 */
static int disconnectMqttSession( MQTTContext_t * pMqttContext );

/**
 * @brief Sends an MQTT SUBSCRIBE to subscribe to #MQTT_EXAMPLE_TOPIC
 * defined at the top of the file.
 *
 * @param[in] pMqttContext MQTT context pointer.
 *
 * @return EXIT_SUCCESS if SUBSCRIBE was successfully sent;
 * EXIT_FAILURE otherwise.
 */
static int subscribeToTopic( MQTTContext_t * pMqttContext );

/**
 * @brief Sends an MQTT UNSUBSCRIBE to unsubscribe from
 * #MQTT_EXAMPLE_TOPIC defined at the top of the file.
 *
 * @param[in] pMqttContext MQTT context pointer.
 *
 * @return EXIT_SUCCESS if UNSUBSCRIBE was successfully sent;
 * EXIT_FAILURE otherwise.
 */
static int unsubscribeFromTopic( MQTTContext_t * pMqttContext );

/**
 * @brief Function to update variable globalSubAckStatus with status
 * information from Subscribe ACK. Called by eventCallback after processing
 * incoming subscribe echo.
 *
 * @param[in] Server response to the subscription request.
 */
static void updateSubAckStatus( MQTTPacketInfo_t * pPacketInfo );

/**
 * @brief Function to handle resubscription of topics on Subscribe
 * ACK failure. Uses an exponential backoff strategy with jitter.
 *
 * @param[in] pMqttContext MQTT context pointer.
 */
static int handleResubscribe( MQTTContext_t * pMqttContext );

/*-----------------------------------------------------------*/

static uint32_t generateRandomNumber()
{
    return( rand() );
}

/*-----------------------------------------------------------*/
static int connectToServerWithBackoffRetries( NetworkContext_t * pNetworkContext )
{
    int returnStatus = EXIT_SUCCESS;
    BackoffAlgorithmStatus_t backoffAlgStatus = BackoffAlgorithmSuccess;
    OpensslStatus_t opensslStatus = OPENSSL_SUCCESS;
    BackoffAlgorithmContext_t reconnectParams;
    ServerInfo_t serverInfo;
    OpensslCredentials_t opensslCredentials;
    uint16_t nextRetryBackOff;

    /* Initialize information to connect to the MQTT broker. */
    serverInfo.pHostName = get_philips_running_status()->flash_param.mqtt_host;
    serverInfo.hostNameLength = strlen(get_philips_running_status()->flash_param.mqtt_host);
    serverInfo.port = get_philips_running_status()->flash_param.mqtt_port;

    /* Initialize credentials for establishing TLS session. */
    memset( &opensslCredentials, 0, sizeof( OpensslCredentials_t ) );
    opensslCredentials.pRootCaPath = ROOT_CA_CERT_PATH;
    opensslCredentials.pClientCertPath = get_philips_running_status()->certificate;
    opensslCredentials.pPrivateKeyPath = get_philips_running_status()->privatekey;

    /* AWS IoT requires devices to send the Server Name Indication (SNI)
     * extension to the Transport Layer Security (TLS) protocol and provide
     * the complete endpoint address in the host_name field. Details about
     * SNI for AWS IoT can be found in the link below.
     * https://docs.aws.amazon.com/iot/latest/developerguide/transport-security.html */
    // opensslCredentials.sniHostName = AWS_IOT_ENDPOINT;
    opensslCredentials.sniHostName = get_philips_running_status()->flash_param.mqtt_host;

    /* Initialize reconnect attempts and interval */
    BackoffAlgorithm_InitializeParams( &reconnectParams,
                                       CONNECTION_RETRY_BACKOFF_BASE_MS,
                                       CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS,
                                       CONNECTION_RETRY_MAX_ATTEMPTS );

    /* Attempt to connect to MQTT broker. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase until maximum
     * attempts are reached.
     */
    do
    {
        /* Establish a TLS session with the MQTT broker. This example connects
         * to the MQTT broker as specified in AWS_IOT_ENDPOINT and AWS_MQTT_PORT
         * at the demo config header. */
        app_log("Establishing a TLS session to %.*s:%d.", serverInfo.hostNameLength, serverInfo.pHostName, serverInfo.port);
        opensslStatus = Openssl_Connect( pNetworkContext,
                                         &serverInfo,
                                         &opensslCredentials,
                                         TRANSPORT_SEND_RECV_TIMEOUT_MS,
                                         TRANSPORT_SEND_RECV_TIMEOUT_MS );

        if( opensslStatus != OPENSSL_SUCCESS )
        {
            // WiFi log
            get_philips_running_status()->wifi_log_param.err_code = opensslStatus;
            get_philips_running_status()->wifi_log_param.err_type = 6;
            get_philips_running_status()->wifi_log_param.err_line = __LINE__;
            /* Generate a random number and get back-off value (in milliseconds) for the next connection retry. */
            backoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &reconnectParams, generateRandomNumber(), &nextRetryBackOff );

            if( backoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                app_log("Connection to the broker failed, all attempts exhausted.");
                returnStatus = EXIT_FAILURE;
            }
            else if( backoffAlgStatus == BackoffAlgorithmSuccess )
            {
                app_log("Connection to the broker failed. Retrying connection after %hu ms backoff.", ( unsigned short ) nextRetryBackOff);
                Clock_SleepMs( nextRetryBackOff );
            }
        }
    } while( ( opensslStatus != OPENSSL_SUCCESS ) && ( backoffAlgStatus == BackoffAlgorithmSuccess ) );

    return returnStatus;
}

/*-----------------------------------------------------------*/

static void handleIncomingPublish( MQTTPublishInfo_t * pPublishInfo,
                                   uint16_t packetIdentifier )
{
    assert( pPublishInfo != NULL );

    /* Process incoming Publish. */
    LogInfo( ( "Incoming QOS : %d.", pPublishInfo->qos ) );

    /* Verify the received publish is for the topic we have subscribed to. */
    if( ( pPublishInfo->topicNameLength == pGlobalSubscriptionList[0].topicFilterLength ) &&
        ( 0 == strncmp( pGlobalSubscriptionList[0].pTopicFilter, pPublishInfo->pTopicName, pPublishInfo->topicNameLength ) ) )
    {
        app_log("Incoming publish Topic:%.*s, Packet ID:%u, Message:\r\n%.*s", pPublishInfo->topicNameLength, pPublishInfo->pTopicName,
                    packetIdentifier, ( int ) pPublishInfo->payloadLength, ( const char * ) pPublishInfo->pPayload );

        philips_process_cloud_data((char *)pPublishInfo->pPayload, (int)pPublishInfo->payloadLength);
    }
    else if( ( pPublishInfo->topicNameLength == pGlobalSubscriptionList[1].topicFilterLength ) &&
        ( 0 == strncmp( pGlobalSubscriptionList[1].pTopicFilter, pPublishInfo->pTopicName, pPublishInfo->topicNameLength ) ) )
    {
        app_log( "Incoming publish Topic:%.*s, Packet ID:%u", pPublishInfo->topicNameLength, pPublishInfo->pTopicName, packetIdentifier );
        
        get_philips_running_status()->device_upload_manage.app_get_status = true;
    }
    else
    {
        LogInfo( ( "Incoming Publish Topic Name: %.*s does not match subscribed topic.",
                   pPublishInfo->topicNameLength, pPublishInfo->pTopicName ) );
    }
}

/*-----------------------------------------------------------*/

static void updateSubAckStatus( MQTTPacketInfo_t * pPacketInfo )
{
    uint8_t * pPayload = NULL;
    size_t pSize = 0;

    MQTTStatus_t mqttStatus = MQTT_GetSubAckStatusCodes( pPacketInfo, &pPayload, &pSize );

    /* MQTT_GetSubAckStatusCodes always returns success if called with packet info
     * from the event callback and non-NULL parameters. */
    assert( mqttStatus == MQTTSuccess );

    /* Suppress unused variable warning when asserts are disabled in build. */
    ( void ) mqttStatus;

    /* Demo only subscribes to one topic, so only one status code is returned. */
    if(pPayload[0] == MQTTSubAckSuccessQos0 && pPayload[1] == MQTTSubAckSuccessQos0)
    {
        globalSubAckStatus = MQTTSubAckSuccessQos0;
    } 
    else
    {
        app_log("payload0:%d, payload1:%d", pPayload[0], pPayload[1]);
        globalSubAckStatus = MQTTSubAckFailure;
    }    
}

/*-----------------------------------------------------------*/

static int handleResubscribe( MQTTContext_t * pMqttContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    BackoffAlgorithmStatus_t backoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t retryParams;
    uint16_t nextRetryBackOff = 0U;

    assert( pMqttContext != NULL );

    /* Initialize retry attempts and interval. */
    BackoffAlgorithm_InitializeParams( &retryParams,
                                       CONNECTION_RETRY_BACKOFF_BASE_MS,
                                       CONNECTION_RETRY_MAX_BACKOFF_DELAY_MS,
                                       CONNECTION_RETRY_MAX_ATTEMPTS );

    do
    {
        /* Send SUBSCRIBE packet.
         * Note: reusing the value specified in globalSubscribePacketIdentifier is acceptable here
         * because this function is entered only after the receipt of a SUBACK, at which point
         * its associated packet id is free to use. */
        mqttStatus = MQTT_Subscribe( pMqttContext,
                                     pGlobalSubscriptionList,
                                     sizeof( pGlobalSubscriptionList ) / sizeof( MQTTSubscribeInfo_t ),
                                     globalSubscribePacketIdentifier );

        if( mqttStatus != MQTTSuccess )
        {
            LogError( ( "Failed to send SUBSCRIBE packet to broker with error = %s.",
                        MQTT_Status_strerror( mqttStatus ) ) );
            returnStatus = EXIT_FAILURE;
            break;
        }

        app_log( "SUBSCRIBE sent for topic %.*s to broker.\n\n", pGlobalSubscriptionList[0].topicFilterLength, pGlobalSubscriptionList[0].pTopicFilter );
        app_log( "SUBSCRIBE sent for topic %.*s to broker.\n\n", pGlobalSubscriptionList[1].topicFilterLength, pGlobalSubscriptionList[1].pTopicFilter );

        /* Process incoming packet. */
        mqttStatus = MQTT_ProcessLoop( pMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );

        if( mqttStatus != MQTTSuccess )
        {
            LogError( ( "MQTT_ProcessLoop returned with status = %s.",
                        MQTT_Status_strerror( mqttStatus ) ) );
            returnStatus = EXIT_FAILURE;
            break;
        }

        /* Check if recent subscription request has been rejected. globalSubAckStatus is updated
         * in eventCallback to reflect the status of the SUBACK sent by the broker. It represents
         * either the QoS level granted by the server upon subscription, or acknowledgement of
         * server rejection of the subscription request. */
        if( globalSubAckStatus == MQTTSubAckFailure )
        {
            /* Generate a random number and get back-off value (in milliseconds) for the next re-subscribe attempt. */
            backoffAlgStatus = BackoffAlgorithm_GetNextBackoff( &retryParams, generateRandomNumber(), &nextRetryBackOff );

            if( backoffAlgStatus == BackoffAlgorithmRetriesExhausted )
            {
                LogError( ( "Subscription to topic failed, all attempts exhausted." ) );
                returnStatus = EXIT_FAILURE;
            }
            else if( backoffAlgStatus == BackoffAlgorithmSuccess )
            {
                LogWarn( ( "Server rejected subscription request. Retrying "
                           "connection after %hu ms backoff.",
                           ( unsigned short ) nextRetryBackOff ) );
                Clock_SleepMs( nextRetryBackOff );
            }
        }
    } while( ( globalSubAckStatus == MQTTSubAckFailure ) && ( backoffAlgStatus == BackoffAlgorithmSuccess ) );

    return returnStatus;
}

/*-----------------------------------------------------------*/

static void eventCallback( MQTTContext_t * pMqttContext,
                           MQTTPacketInfo_t * pPacketInfo,
                           MQTTDeserializedInfo_t * pDeserializedInfo )
{
    uint16_t packetIdentifier;

    assert( pMqttContext != NULL );
    assert( pPacketInfo != NULL );
    assert( pDeserializedInfo != NULL );

    /* Suppress unused parameter warning when asserts are disabled in build. */
    ( void ) pMqttContext;

    packetIdentifier = pDeserializedInfo->packetIdentifier;

    /* Handle incoming publish. The lower 4 bits of the publish packet
     * type is used for the dup, QoS, and retain flags. Hence masking
     * out the lower bits to check if the packet is publish. */
    if( ( pPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        assert( pDeserializedInfo->pPublishInfo != NULL );
        /* Handle incoming publish. */
        handleIncomingPublish( pDeserializedInfo->pPublishInfo, packetIdentifier );
    }
    else
    {
        /* Handle other packets. */
        switch( pPacketInfo->type )
        {
            case MQTT_PACKET_TYPE_SUBACK:
                /* A SUBACK from the broker, containing the server response to our subscription request, has been received.
                 * It contains the status code indicating server approval/rejection for the subscription to the single topic
                 * requested. The SUBACK will be parsed to obtain the status code, and this status code will be stored in global
                 * variable globalSubAckStatus. */
                updateSubAckStatus( pPacketInfo );

                /* Check status of the subscription request. If globalSubAckStatus does not indicate
                 * server refusal of the request (MQTTSubAckFailure), it contains the QoS level granted
                 * by the server, indicating a successful subscription attempt. */
                if( globalSubAckStatus != MQTTSubAckFailure )
                {
                    app_log("Subscribed Success");
                }

                /* Make sure ACK packet identifier matches with Request packet identifier. */
                assert( globalSubscribePacketIdentifier == packetIdentifier );
                break;

            case MQTT_PACKET_TYPE_UNSUBACK:
                app_log("Unsubscribed Success");
                /* Make sure ACK packet identifier matches with Request packet identifier. */
                assert( globalUnsubscribePacketIdentifier == packetIdentifier );
                break;

            case MQTT_PACKET_TYPE_PINGRESP:

                /* Nothing to be done from application as library handles PINGRESP. */
                LogWarn( ( "PINGRESP should not be handled by the application callback when using MQTT_ProcessLoop.\n\n" ) );
                break;

            case MQTT_PACKET_TYPE_PUBACK:
                LogInfo( ( "PUBACK received for packet id %u.\n\n", packetIdentifier ) );
                break;

            /* Any other packet type is invalid. */
            default:
                LogError( ( "Unknown packet type received:(%02x).\n\n", pPacketInfo->type ) );
        }
    }
}

/*-----------------------------------------------------------*/

static int establishMqttSession( MQTTContext_t * pMqttContext,
                                 bool createCleanSession,
                                 bool * pSessionPresent )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;
    MQTTConnectInfo_t connectInfo = { 0 };
    MQTTPublishInfo_t will_info;

    char will_topic[WILL_TOPIC_SIZE] = {0};
	char will_msg[WILL_MSG_SIZE] = {0};

    assert( pMqttContext != NULL );
    assert( pSessionPresent != NULL );

    /* Establish MQTT session by sending a CONNECT packet. */

    /* If #createCleanSession is true, start with a clean session
     * i.e. direct the MQTT broker to discard any previous session data.
     * If #createCleanSession is false, directs the broker to attempt to
     * reestablish a session which was already present. */
    connectInfo.cleanSession = createCleanSession;

    connectInfo.pClientIdentifier = get_philips_running_status()->flash_param.device_id;
    connectInfo.clientIdentifierLength = strlen(get_philips_running_status()->flash_param.device_id);

    connectInfo.keepAliveSeconds = MQTT_KEEP_ALIVE_INTERVAL_SECONDS;

    connectInfo.pUserName = NULL;
    connectInfo.userNameLength = 0U;
    connectInfo.pPassword = NULL;
    connectInfo.passwordLength = 0U;

    memset(&will_info, 0, sizeof(MQTTPublishInfo_t));
    memset(will_topic, 0x00, WILL_TOPIC_SIZE);
	snprintf(will_topic, WILL_TOPIC_SIZE, "Philips/%s/offline", get_philips_running_status()->flash_param.device_id);
	memset(will_msg, 0x00, WILL_MSG_SIZE);
	snprintf(will_msg, WILL_MSG_SIZE, "{\"state\":{\"reported\":{\"ConnectType\":\"Offline\",\"StatusType\":\"disconnect\",\"DeviceId\":\"%s\",\"ProductId\":\"%s\",\"pwr\":\"%c\"}}}", get_philips_running_status()->flash_param.device_id, get_philips_running_status()->flash_param.product_id, '1'/* uartcmd.pwr */);
    will_info.pTopicName = will_topic;
    will_info.topicNameLength = strlen(will_topic);
    will_info.pPayload = will_msg;
    will_info.payloadLength = strlen(will_msg);

    /* Send MQTT CONNECT packet to broker. */
    mqttStatus = MQTT_Connect( pMqttContext, &connectInfo, &will_info, CONNACK_RECV_TIMEOUT_MS, pSessionPresent );

    if( mqttStatus != MQTTSuccess )
    {
        returnStatus = EXIT_FAILURE;
        app_log("Connection with MQTT broker failed with status %s.", MQTT_Status_strerror( mqttStatus ));
    }
    else
    {
        app_log("MQTT connection successfully established with broker.");
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int disconnectMqttSession( MQTTContext_t * pMqttContext )
{
    MQTTStatus_t mqttStatus = MQTTSuccess;
    int returnStatus = EXIT_SUCCESS;

    assert( pMqttContext != NULL );

    /* Send DISCONNECT. */
    mqttStatus = MQTT_Disconnect( pMqttContext );

    if( mqttStatus != MQTTSuccess )
    {
        app_log("Sending MQTT DISCONNECT failed with status=%s.", MQTT_Status_strerror( mqttStatus ));
        returnStatus = EXIT_FAILURE;
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int subscribeToTopic( MQTTContext_t * pMqttContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;

    assert( pMqttContext != NULL );

    /* Start with everything at 0. */
    ( void ) memset( ( void * ) pGlobalSubscriptionList, 0x00, sizeof( pGlobalSubscriptionList ) );

    sprintf(topic_shadow_update, "$aws/things/%s/shadow/update/accepted", get_philips_running_status()->flash_param.device_id);
    sprintf(topic_shadow_get, "$aws/things/%s/shadow/get/accepted", get_philips_running_status()->flash_param.device_id);

    /* This example subscribes to only one topic and uses QOS1. */
    pGlobalSubscriptionList[ 0 ].qos = MQTTQoS0;
    pGlobalSubscriptionList[ 0 ].pTopicFilter = topic_shadow_update;
    pGlobalSubscriptionList[ 0 ].topicFilterLength = strlen(topic_shadow_update);

    pGlobalSubscriptionList[ 1 ].qos = MQTTQoS0;
    pGlobalSubscriptionList[ 1 ].pTopicFilter = topic_shadow_get;
    pGlobalSubscriptionList[ 1 ].topicFilterLength = strlen(topic_shadow_get);

    app_log( "Subscribing to the MQTT topic %.*s.",  pGlobalSubscriptionList[0].topicFilterLength, pGlobalSubscriptionList[0].pTopicFilter );
    app_log( "Subscribing to the MQTT topic %.*s.",  pGlobalSubscriptionList[1].topicFilterLength, pGlobalSubscriptionList[1].pTopicFilter );

    /* Generate packet identifier for the SUBSCRIBE packet. */
    globalSubscribePacketIdentifier = MQTT_GetPacketId( pMqttContext );

    /* Send SUBSCRIBE packet. */
    mqttStatus = MQTT_Subscribe( pMqttContext,
                                 pGlobalSubscriptionList,
                                 sizeof( pGlobalSubscriptionList ) / sizeof( MQTTSubscribeInfo_t ),
                                 globalSubscribePacketIdentifier );

    if( mqttStatus != MQTTSuccess )
    {
        app_log("Failed to send SUBSCRIBE packet to broker with error = %s.", MQTT_Status_strerror( mqttStatus ));
        returnStatus = EXIT_FAILURE;
    }
    else
    {
        app_log("send successfully SUBSCRIBE packet to broker.");
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int unsubscribeFromTopic( MQTTContext_t * pMqttContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;

    assert( pMqttContext != NULL );

    /* Start with everything at 0. */
    ( void ) memset( ( void * ) pGlobalSubscriptionList, 0x00, sizeof( pGlobalSubscriptionList ) );

    sprintf(topic_shadow_update, "$aws/things/%s/shadow/update/accepted", get_philips_running_status()->flash_param.device_id);
    sprintf(topic_shadow_get, "$aws/things/%s/shadow/get/accepted", get_philips_running_status()->flash_param.device_id);

    /* This example subscribes to and unsubscribes from only one topic
     * and uses QOS1. */
    pGlobalSubscriptionList[ 0 ].qos = MQTTQoS0;
    pGlobalSubscriptionList[ 0 ].pTopicFilter = topic_shadow_update;
    pGlobalSubscriptionList[ 0 ].topicFilterLength = strlen(topic_shadow_update);

    pGlobalSubscriptionList[ 1 ].qos = MQTTQoS0;
    pGlobalSubscriptionList[ 1 ].pTopicFilter = topic_shadow_get;
    pGlobalSubscriptionList[ 1 ].topicFilterLength = strlen(topic_shadow_get);

    app_log( "Unsubscribing from the MQTT topic %.*s.", pGlobalSubscriptionList[0].topicFilterLength, pGlobalSubscriptionList[0].pTopicFilter );
    app_log( "Unsubscribing from the MQTT topic %.*s.", pGlobalSubscriptionList[1].topicFilterLength, pGlobalSubscriptionList[1].pTopicFilter );

    /* Generate packet identifier for the UNSUBSCRIBE packet. */
    globalUnsubscribePacketIdentifier = MQTT_GetPacketId( pMqttContext );

    /* Send UNSUBSCRIBE packet. */
    mqttStatus = MQTT_Unsubscribe( pMqttContext,
                                   pGlobalSubscriptionList,
                                   sizeof( pGlobalSubscriptionList ) / sizeof( MQTTSubscribeInfo_t ),
                                   globalUnsubscribePacketIdentifier );

    if( mqttStatus != MQTTSuccess )
    {
        app_log( "Failed to send UNSUBSCRIBE packet to broker with error = %s.", MQTT_Status_strerror( mqttStatus ) );
        returnStatus = EXIT_FAILURE;
    }
    else
    {
        app_log( "send successfully UNSUBSCRIBE packet to broker." );
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int initializeMqtt( MQTTContext_t * pMqttContext,
                           NetworkContext_t * pNetworkContext )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTStatus_t mqttStatus;
    MQTTFixedBuffer_t networkBuffer;
    TransportInterface_t transport;

    assert( pMqttContext != NULL );
    assert( pNetworkContext != NULL );

    /* Fill in TransportInterface send and receive function pointers.
     * For this demo, TCP sockets are used to send and receive data
     * from network. Network context is SSL context for OpenSSL.*/
    transport.pNetworkContext = pNetworkContext;
    transport.send = Openssl_Send;
    transport.recv = Openssl_Recv;

    /* Fill the values for network buffer. */
    networkBuffer.pBuffer = buffer;
    networkBuffer.size = NETWORK_BUFFER_SIZE;

    /* Initialize MQTT library. */
    mqttStatus = MQTT_Init( pMqttContext,
                            &transport,
                            Clock_GetTimeMs,
                            eventCallback,
                            &networkBuffer );

    if( mqttStatus != MQTTSuccess )
    {
        returnStatus = EXIT_FAILURE;
        app_log("MQTT init failed: Status = %s.", MQTT_Status_strerror( mqttStatus ));
    }

    return returnStatus;
}

/*-----------------------------------------------------------*/

static int subscribePublishLoop( MQTTContext_t * pMqttContext,
                                 bool * pClientSessionPresent )
{
    int returnStatus = EXIT_SUCCESS;
    bool mqttSessionEstablished = false, brokerSessionPresent;
    MQTTStatus_t mqttStatus = MQTTSuccess;
    uint32_t publishCount = 0;
    const uint32_t maxPublishCount = MQTT_PUBLISH_COUNT_PER_LOOP;
    bool createCleanSession = false;

    queue_message_t queue_msg = {MT_Unknow, NULL, 0};
    MQTTPublishInfo_t pubInfo;
    uint16_t packetId = 0;

    char publish_msg[MAX_TOPIC_LENGTH] = {0};

    assert( pMqttContext != NULL );
    assert( pClientSessionPresent != NULL );

    /* A clean MQTT session needs to be created, if there is no session saved
     * in this MQTT client. */
    createCleanSession = ( *pClientSessionPresent == true ) ? false : true;

    /* Establish MQTT session on top of TCP+TLS connection. */
    LogInfo( ( "Creating an MQTT connection to %.*s.", strlen(get_philips_running_status()->flash_param.mqtt_host), get_philips_running_status()->flash_param.mqtt_host ) );

    /* Sends an MQTT Connect packet using the established TLS session,
     * then waits for connection acknowledgment (CONNACK) packet. */
    returnStatus = establishMqttSession( pMqttContext, createCleanSession, &brokerSessionPresent );

    if( returnStatus == EXIT_SUCCESS )
    {
        /* Keep a flag for indicating if MQTT session is established. This
         * flag will mark that an MQTT DISCONNECT has to be sent at the end
         * of the demo, even if there are intermediate failures. */
        mqttSessionEstablished = true;

        /* Update the flag to indicate that an MQTT client session is saved.
         * Once this flag is set, MQTT connect in the following iterations of
         * this demo will be attempted without requesting for a clean session. */
        *pClientSessionPresent = true;

        /* Check if session is present and if there are any outgoing publishes
         * that need to resend. This is only valid if the broker is
         * re-establishing a session which was already present. */
        if( brokerSessionPresent == true )
        {
            app_log( "An MQTT session with broker is re-established. Resending unacked publishes." );
            /* Handle all the resend of publish messages. */
        }
        else
        {
            app_log( "A clean MQTT connection is established. Cleaning up all the stored outgoing publishes." );
            /* Clean up the outgoing publishes waiting for ack as this new
             * connection doesn't re-establish an existing session. */
        }

        get_philips_running_status()->wifi_status = WRS_wifiui_connect_cloud;
        get_philips_running_status()->is_reconnect_cloud = true;
    }
    else
    {
        // WiFi log
        get_philips_running_status()->wifi_log_param.err_code = returnStatus;
        get_philips_running_status()->wifi_log_param.err_type = 6;
        get_philips_running_status()->wifi_log_param.err_line = __LINE__;
    }

    if( returnStatus == EXIT_SUCCESS )
    {
        /* The client is now connected to the broker. Subscribe to the topic
         * as specified in MQTT_EXAMPLE_TOPIC at the top of this file by sending a
         * subscribe packet. This client will then publish to the same topic it
         * subscribed to, so it will expect all the messages it sends to the broker
         * to be sent back to it from the broker. This demo uses QOS1 in Subscribe,
         * therefore, the Publish messages received from the broker will have QOS1. */
        returnStatus = subscribeToTopic( pMqttContext );
        // WiFi log
        if(returnStatus != EXIT_SUCCESS)
        {
            get_philips_running_status()->wifi_log_param.err_code = returnStatus;
            get_philips_running_status()->wifi_log_param.err_type = 6;
            get_philips_running_status()->wifi_log_param.err_line = __LINE__;
        }
    }

    if( returnStatus == EXIT_SUCCESS )
    {
        /* Process incoming packet from the broker. Acknowledgment for subscription
         * ( SUBACK ) will be received here. However after sending the subscribe, the
         * client may receive a publish before it receives a subscribe ack. Since this
         * demo is subscribing to the topic to which no one is publishing, probability
         * of receiving publish message before subscribe ack is zero; but application
         * must be ready to receive any packet. This demo uses MQTT_ProcessLoop to
         * receive packet from network. */
        mqttStatus = MQTT_ProcessLoop( pMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );

        if( mqttStatus != MQTTSuccess )
        {
            returnStatus = EXIT_FAILURE;
            LogError( ( "MQTT_ProcessLoop returned with status = %s.",  MQTT_Status_strerror( mqttStatus ) ) );
            // WiFi log
            get_philips_running_status()->wifi_log_param.err_code = mqttStatus;
            get_philips_running_status()->wifi_log_param.err_type = 6;
            get_philips_running_status()->wifi_log_param.err_line = __LINE__;
        }
    }

    /* Check if recent subscription request has been rejected. globalSubAckStatus is updated
     * in eventCallback to reflect the status of the SUBACK sent by the broker. */
    if( ( returnStatus == EXIT_SUCCESS ) && ( globalSubAckStatus == MQTTSubAckFailure ) )
    {
        /* If server rejected the subscription request, attempt to resubscribe to topic.
         * Attempts are made according to the exponential backoff retry strategy
         * implemented in retryUtils. */
        app_log( "Server rejected initial subscription request. Attempting to re-subscribe to topic");
        returnStatus = handleResubscribe( pMqttContext );
        // WiFi log
        if(returnStatus != EXIT_SUCCESS)
        {
            get_philips_running_status()->wifi_log_param.err_code = returnStatus;
            get_philips_running_status()->wifi_log_param.err_type = 6;
            get_philips_running_status()->wifi_log_param.err_line = __LINE__;
        }
    }

    if( returnStatus == EXIT_SUCCESS )
    {
        if (get_philips_running_status()->is_first_config_wifi && !(get_philips_running_status()->softap_option & 0x08)) //配网成功后，连接MQTT成功
        {
            // get_philips_running_status()->wifi_log_param.step = PHILIPS_WIFI_LOG_STEP_MQTT;
            // get_philips_running_status()->wifi_log_param.result = true;
            // get_philips_running_status()->wifi_log_param.err_code = 0;
            // get_philips_running_status()->wifi_log_param.err_type = 0;
            // get_philips_running_status()->wifi_log_param.err_line = 0;
            // strcpy(get_philips_running_status()->wifi_log_param.message, "none");
            //philips_mqtt_publish_notice_wifi_log();
        }

        mqtt_connect_continuous_fail_cnt = 0;

        if(get_philips_running_status()->mqtt_monitor_start == false)		//MQTT看门狗未开启，开启MQTT看门狗
        {
            if(mxos_system_monitor_register(&get_philips_running_status()->mqtt_monitor, PHILIPS_SOFT_WATCHDOG_TIMEOUT) == kNoErr)	//MQTT看门狗注册成功
            {
                get_philips_running_status()->mqtt_monitor_start = true;
            }
        }
        else if(get_philips_running_status()->mqtt_monitor_start == true)	//MQTT看门狗已开启，重新设置喂狗超时时间
        {
            mxos_system_monitor_update(&get_philips_running_status()->mqtt_monitor, PHILIPS_SOFT_WATCHDOG_TIMEOUT);
            get_philips_running_status()->mqtt_feed_dog_time = mos_time();
        }

        while (1)
        {
            if(mos_time() - get_philips_running_status()->mqtt_feed_dog_time >= PHILIPS_SOFT_FEEDDOG_PERIOD)
            {
                mxos_system_monitor_update(&get_philips_running_status()->mqtt_monitor, PHILIPS_SOFT_WATCHDOG_TIMEOUT);
                get_philips_running_status()->mqtt_feed_dog_time = mos_time();
                app_log("feed mqtt watchdog");
            }

            mqttStatus = MQTT_ProcessLoop( pMqttContext, 200 );
            if (mqttStatus != MQTTSuccess)
            {
                app_log("MQTT_ProcessLoop returned with status = %s.", MQTT_Status_strerror(mqttStatus));
                returnStatus = EXIT_FAILURE;
                // WiFi log
                get_philips_running_status()->wifi_log_param.err_code = mqttStatus;
                get_philips_running_status()->wifi_log_param.err_type = 6;
                get_philips_running_status()->wifi_log_param.err_line = __LINE__;
                break;
            }

            // 主动断开，上报离线消息
            if(aws_iot_mqtt_thread_exit)
            {
                char *upload_data = NULL;

                philips_encode_json(&upload_data, STATUS_TYPE_DISCONNECT, CONNECT_TYPE_OFFLINE, true);

                sprintf(publish_msg, "$aws/things/%s/shadow/update", get_philips_running_status()->flash_param.device_id);

                memset(&pubInfo, 0, sizeof(pubInfo));
                pubInfo.qos = MQTTQoS0;
                pubInfo.pTopicName = publish_msg;
                pubInfo.topicNameLength = strlen(publish_msg);
                pubInfo.pPayload = (void *)upload_data;
                pubInfo.payloadLength = strlen((char *)upload_data);
                
                packetId = MQTT_GetPacketId( pMqttContext );

                mqttStatus = MQTT_Publish( pMqttContext, &pubInfo, packetId );
                if( mqttStatus != MQTTSuccess )
                {
                    app_log( "Failed to send PUBLISH packet to broker with error = %s.", MQTT_Status_strerror( mqttStatus ) );
                    returnStatus = EXIT_FAILURE;
                    // WiFi log
                    get_philips_running_status()->wifi_log_param.err_code = mqttStatus;
                    get_philips_running_status()->wifi_log_param.err_type = 6;
                    get_philips_running_status()->wifi_log_param.err_line = __LINE__;
                    break;
                }
                app_log("push packetid:%d, topic:%.*s, data:%.*s", packetId, pubInfo.topicNameLength, (char *)pubInfo.pTopicName,
                    pubInfo.payloadLength, (char *)pubInfo.pPayload);

                if(upload_data != NULL)
                {
                    free(upload_data);
                    upload_data = NULL;
                }
                
                break;
            }

            if(mos_queue_get_total(mqtt_send_msg_queue) == 0)
                continue;
            if(mos_queue_pop(mqtt_send_msg_queue, &queue_msg, 200) == kNoErr)
            {
                switch ( queue_msg.type )
                {
                    case MT_SHADOW_UPDATE:
                        sprintf(publish_msg, "$aws/things/%s/shadow/update", get_philips_running_status()->flash_param.device_id);
                        break;
                    case MT_SHADOW_DELETE:
                        sprintf(publish_msg, "$aws/things/%s/shadow/delete", get_philips_running_status()->flash_param.device_id);
                        break;
                    case MT_SHADOW_GET:
                        sprintf(publish_msg, "$aws/things/%s/shadow/get", get_philips_running_status()->flash_param.device_id);
                        break;
                    case MT_DEVICE_NOTICE:
                        sprintf(publish_msg, "Philips/%s/notice", get_philips_running_status()->flash_param.device_id);
                        break;
                    case MT_DEVICE_DATA:
                        sprintf(publish_msg, "Philips/%s/data", get_philips_running_status()->flash_param.device_id);
                        break;
                    default:
                        break;
                }

                memset(&pubInfo, 0, sizeof(pubInfo));
                pubInfo.qos = MQTTQoS0;
                pubInfo.pTopicName = publish_msg;
                pubInfo.topicNameLength = strlen(publish_msg);
                pubInfo.pPayload = (void *)(queue_msg.data);
                pubInfo.payloadLength = strlen((char *)(queue_msg.data));

                packetId = MQTT_GetPacketId( pMqttContext );

                mqttStatus = MQTT_Publish( pMqttContext, &pubInfo, packetId );
                if( mqttStatus != MQTTSuccess )
                {
                    app_log( "Failed to send PUBLISH packet to broker with error = %s.", MQTT_Status_strerror( mqttStatus ) );
                    returnStatus = EXIT_FAILURE;
                    // WiFi log
                    get_philips_running_status()->wifi_log_param.err_code = mqttStatus;
                    get_philips_running_status()->wifi_log_param.err_type = 6;
                    get_philips_running_status()->wifi_log_param.err_line = __LINE__;
                    break;
                }

                app_log("push packetid:%d, topic:%.*s, data:%.*s", packetId, pubInfo.topicNameLength, (char *)pubInfo.pTopicName,
                        pubInfo.payloadLength, (char *)pubInfo.pPayload);

                if(queue_msg.data != NULL)
                {
                    free(queue_msg.data);
                    queue_msg.data = NULL;
                }
                memset(&queue_msg, 0x00, sizeof(queue_message_t));
            }
        }

        if(get_philips_running_status()->mqtt_monitor_start)		//MQTT喂狗操作退出，设置看门狗超时时间为最大0xFFFFFFFF
        {
            mxos_system_monitor_update(&get_philips_running_status()->mqtt_monitor, PHILIPS_WATCHDOG_TIMEOUT_MAX);
            get_philips_running_status()->mqtt_feed_dog_time = mos_time();
            app_log("mqtt thread exit, feed mqtt watchdog");
        }
    }

    if( returnStatus == EXIT_SUCCESS )
    {
        /* Unsubscribe from the topic. */
        returnStatus = unsubscribeFromTopic( pMqttContext );
        // WiFi log
        if(returnStatus != EXIT_SUCCESS)
        {
            get_philips_running_status()->wifi_log_param.err_code = returnStatus;
            get_philips_running_status()->wifi_log_param.err_type = 6;
            get_philips_running_status()->wifi_log_param.err_line = __LINE__;
        }
    }

    if( returnStatus == EXIT_SUCCESS )
    {
        /* Process Incoming UNSUBACK packet from the broker. */
        mqttStatus = MQTT_ProcessLoop( pMqttContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );

        if( mqttStatus != MQTTSuccess )
        {
            returnStatus = EXIT_FAILURE;
            LogError( ( "MQTT_ProcessLoop returned with status = %s.", MQTT_Status_strerror( mqttStatus ) ) );
            // WiFi log
            get_philips_running_status()->wifi_log_param.err_code = mqttStatus;
            get_philips_running_status()->wifi_log_param.err_type = 6;
            get_philips_running_status()->wifi_log_param.err_line = __LINE__;
        }
    }

    /* Send an MQTT Disconnect packet over the already connected TCP socket.
     * There is no corresponding response for the disconnect packet. After sending
     * disconnect, client must close the network connection. */
    if( mqttSessionEstablished == true )
    {
        app_log( "Disconnecting the MQTT connection with %.*s.",  strlen(get_philips_running_status()->flash_param.mqtt_host), get_philips_running_status()->flash_param.mqtt_host );

        if( returnStatus == EXIT_FAILURE )
        {
            /* Returned status is not used to update the local status as there
             * were failures in demo execution. */
            ( void ) disconnectMqttSession( pMqttContext );
        }
        else
        {
            returnStatus = disconnectMqttSession( pMqttContext );
        }
    }

    get_philips_running_status()->wifi_status = WRS_wifiui_connect_router;

    /* Reset global SUBACK status variable after completion of subscription request cycle. */
    globalSubAckStatus = MQTTSubAckFailure;

    return returnStatus;
}

/*-----------------------------------------------------------*/

void mqtt_thread( void *arg )
{
    int returnStatus = EXIT_SUCCESS;
    MQTTContext_t mqttContext = { 0 };
    NetworkContext_t networkContext = { 0 };
    OpensslParams_t opensslParams = {-1, NULL};
    bool clientSessionPresent = false;
    uint32_t time_ms = 0;

    /* Set the pParams member of the network context with desired transport. */
    networkContext.pParams = &opensslParams;

    /* Seed pseudo random number generator (provided by ISO C standard library) for
     * use by retry utils library when retrying failed network operations. */

    /* Get current time to seed pseudo random number generator. */
    time_ms = mos_time();
    /* Seed pseudo random number generator with nanoseconds. */
    srand( time_ms );

RECONNECT:
    /* Initialize MQTT library. Initialization of the MQTT library needs to be
     * done only once in this demo. */
    returnStatus = initializeMqtt( &mqttContext, &networkContext );

    if( returnStatus == EXIT_SUCCESS )
    {
        for( ; ; )
        {
            if(aws_iot_mqtt_thread_exit)
            {
                break;
            }

           // if((mqtt_connect_continuous_fail_cnt >= 2)&&(get_philips_running_status()->wifi_status == WRS_wifiui_connect_router))
           if((mqtt_connect_continuous_fail_cnt)&&(get_philips_running_status()->wifi_status == WRS_wifiui_connect_router))
            {
                mqtt_connect_continuous_fail_cnt = 0;
               // if (get_philips_running_status()->is_first_config_wifi && !(get_philips_running_status()->softap_option & 0x08)) //配网成功后，连接MQTT成功
                {
                    get_philips_running_status()->wifi_log_param.step = PHILIPS_WIFI_LOG_STEP_MQTT;
                    get_philips_running_status()->wifi_log_param.result = false;
                    strcpy(get_philips_running_status()->wifi_log_param.message, "MQTT error");

                 //  if((get_philips_running_status()->wifi_log_param.step!=get_philips_running_status()->offline_log_flash_param.step)||(get_philips_running_status()->wifi_log_param.err_code!=get_philips_running_status()->offline_log_flash_param.err_code)||(get_philips_running_status()->wifi_log_param.err_type!=get_philips_running_status()->offline_log_flash_param.err_type))
                    {
                        // get_philips_running_status()->offline_log_flash_param.step=get_philips_running_status()->wifi_log_param.step;
                        // get_philips_running_status()->offline_log_flash_param.err_code=get_philips_running_status()->wifi_log_param.err_code;
                        // get_philips_running_status()->offline_log_flash_param.err_type=get_philips_running_status()->wifi_log_param.err_type;
                        kv_save_offlinelog(get_philips_running_status()->wifi_log_param.step,get_philips_running_status()->wifi_log_param.err_code,get_philips_running_status()->wifi_log_param.err_type);
                        mos_sleep(4);
                        // philips_wifi_log(get_philips_running_status()->philips_urls[get_philips_running_status()->flash_param.url_type].url_wifilog);
                    }
                }
            }

            /* Attempt to connect to the MQTT broker. If connection fails, retry after
             * a timeout. Timeout value will be exponentially increased till the maximum
             * attempts are reached or maximum timeout value is reached. The function
             * returns EXIT_FAILURE if the TCP connection cannot be established to
             * broker after configured number of attempts. */
            returnStatus = connectToServerWithBackoffRetries( &networkContext );

            if( returnStatus == EXIT_FAILURE )
            {
                /* Log error to indicate connection failure after all
                 * reconnect attempts are over. */
                LogError( ( "Failed to connect to MQTT broker %.*s.", strlen(get_philips_running_status()->flash_param.mqtt_host), get_philips_running_status()->flash_param.mqtt_host ) );
            }
            else
            {
                mqtt_connect_continuous_fail_cnt=0;
                kv_item_delete(DATA_KV_WIFI_OFFLINE_LOG);
                /* If TLS session is established, execute Subscribe/Publish loop. */
                returnStatus = subscribePublishLoop( &mqttContext, &clientSessionPresent );
            }

            /* End TLS session, then close TCP connection. */
            ( void ) Openssl_Disconnect( &networkContext );

            app_log( "Short delay before starting the next iteration...." );
            mos_sleep( MQTT_SUBPUB_LOOP_DELAY_SECONDS );

            mqtt_connect_continuous_fail_cnt++;
        }
    }
    else
    {
        mos_sleep( MQTT_SUBPUB_LOOP_DELAY_SECONDS );
        goto RECONNECT;
    }

    mos_thread_delete(NULL);
}

/*-----------------------------------------------------------*/

merr_t philips_mqtt_param_init(void)
{
    merr_t err = kNoErr;

    mqtt_send_msg_queue = mos_queue_new(sizeof(queue_message_t), 10);
    require_action( (mqtt_send_msg_queue != NULL), exit, err = kGeneralErr );
exit:
    return err;
}

/*-----------------------------------------------------------*/

merr_t philips_mqtt_msg_push_queue(queue_message_t *msg)
{
    merr_t err = kNoErr;

    if(mos_queue_get_free(mqtt_send_msg_queue))
    {
        err = mos_queue_push(mqtt_send_msg_queue, msg, 200);
        require_noerr(err, exit);
    }
    else
    {
        err = kGeneralErr;
    }
exit:
    return err;
}

/*-----------------------------------------------------------*/

bool philips_mqtt_client_is_running(void)
{
    if(aws_iot_mqtt_thread)
        return true;
    else
        return false;
}

/*-----------------------------------------------------------*/

merr_t philips_mqtt_client_close(void)
{
    merr_t err = kNoErr;

    aws_iot_mqtt_thread_exit = true;

    mos_thread_join(aws_iot_mqtt_thread);

    aws_iot_mqtt_thread = NULL;

    aws_iot_mqtt_thread_exit = false;

    return err;
}

/*-----------------------------------------------------------*/

merr_t philips_mqtt_client_start(void)
{
    merr_t err = kNoErr;
    
    aws_iot_mqtt_thread = mos_thread_new(MOS_APPLICATION_PRIORITY, "aws shadow", mqtt_thread, 1024*7, NULL);
    require_action((aws_iot_mqtt_thread != NULL), exit, err = kGeneralErr);

exit:
    return err;
}

/*-----------------------------------------------------------*/
