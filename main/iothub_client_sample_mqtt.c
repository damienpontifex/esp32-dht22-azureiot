// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdio.h>
#include <stdlib.h>
#include <dht.h>

#include "iothub_client.h"
#include "iothub_device_client_ll.h"
#include "iothub_client_options.h"
#include "iothub_message.h"
#include "azure_c_shared_utility/threadapi.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/platform.h"
#include "azure_c_shared_utility/shared_util_options.h"
#include "iothubtransportmqtt.h"
#include "iothub_client_options.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef MBED_BUILD_TIMESTAMP
#define SET_TRUSTED_CERT_IN_SAMPLES
#endif // MBED_BUILD_TIMESTAMP

#ifdef SET_TRUSTED_CERT_IN_SAMPLES
#include "certs.h"
#endif // SET_TRUSTED_CERT_IN_SAMPLES

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
#define EXAMPLE_IOTHUB_CONNECTION_STRING CONFIG_IOTHUB_CONNECTION_STRING
static const char* connectionString = EXAMPLE_IOTHUB_CONNECTION_STRING;

static const dht_sensor_type_t sensor_type = DHT_TYPE_AM2301;
static const gpio_num_t dht_gpio = 17;


static int callbackCounter;
static char msgText[1024];
static char propText[1024];
static bool g_continueRunning;
#define MESSAGE_COUNT CONFIG_MESSAGE_COUNT
#define DOWORK_LOOP_NUM     3

typedef struct EVENT_INSTANCE_TAG
{
  IOTHUB_MESSAGE_HANDLE messageHandle;
  size_t messageTrackingId;  // For tracking the messages within the user callback.
} EVENT_INSTANCE;

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
  EVENT_INSTANCE* eventInstance = (EVENT_INSTANCE*)userContextCallback;
  size_t id = eventInstance->messageTrackingId;

  if (result == IOTHUB_CLIENT_CONFIRMATION_OK) {
    (void)printf("Confirmation[%d] received for message tracking id = %d with result = %s\r\n", callbackCounter, (int)id, MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
    /* Some device specific action code goes here... */
    callbackCounter++;
  }
  IoTHubMessage_Destroy(eventInstance->messageHandle);
}

void connection_status_callback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason, void* userContextCallback)
{
  (void)printf("\n\nConnection Status result:%s, Connection Status reason: %s\n\n", MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS, result),
      MU_ENUM_TO_STRING(IOTHUB_CLIENT_CONNECTION_STATUS_REASON, reason));
}

void iothub_client_sample_mqtt_run(void)
{
  IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

  EVENT_INSTANCE message;

  g_continueRunning = true;
  srand((unsigned int)time(NULL));

  callbackCounter = 0;

  if (platform_init() != 0)
  {
    (void)printf("Failed to initialize the platform.\r\n");
  }
  else
  {
    if ((iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol)) == NULL)
    {
      (void)printf("ERROR: iotHubClientHandle is NULL!\r\n");
    }
    else
    {
      bool traceOn = true;
      IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &traceOn);

      IoTHubClient_LL_SetConnectionStatusCallback(iotHubClientHandle, connection_status_callback, NULL);
      // Setting the Trusted Certificate.  This is only necessary on system with without
      // built in certificate stores.
#ifdef SET_TRUSTED_CERT_IN_SAMPLES
      IoTHubDeviceClient_LL_SetOption(iotHubClientHandle, OPTION_TRUSTED_CERT, certificates);
#endif // SET_TRUSTED_CERT_IN_SAMPLES

      /* Now that we are ready to receive commands, let's send some messages */
      int iterator = 0;
      float temperature = 0;
      float humidity = 0;
      time_t sent_time = 0;
      time_t current_time = 0;
      do
      {
        //(void)printf("iterator: [%d], callbackCounter: [%d]. \r\n", iterator, callbackCounter);
        time(&current_time);
        if ((MESSAGE_COUNT == 0 || iterator < MESSAGE_COUNT)
            && iterator <= callbackCounter
            && (difftime(current_time, sent_time) > ((CONFIG_MESSAGE_INTERVAL_TIME) / 1000))
            && (dht_read_float_data(sensor_type, dht_gpio, &humidity, &temperature) == ESP_OK))
        {
          sprintf_s(msgText, sizeof(msgText), "{\"deviceId\":\"espdevice\",\"temperature\":%.2f,\"humidity\":%.2f}", temperature, humidity);
          if ((message.messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char*)msgText, strlen(msgText))) == NULL)
          {
            (void)printf("ERROR: iotHubMessageHandle is NULL!\r\n");
          }
          else
          {
            message.messageTrackingId = iterator;
            MAP_HANDLE propMap = IoTHubMessage_Properties(message.messageHandle);
            (void)sprintf_s(propText, sizeof(propText), temperature > 28 ? "true" : "false");
            if (Map_AddOrUpdate(propMap, "temperatureAlert", propText) != MAP_OK)
            {
              (void)printf("ERROR: Map_AddOrUpdate Failed!\r\n");
            }

            if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, message.messageHandle, SendConfirmationCallback, &message) != IOTHUB_CLIENT_OK)
            {
              (void)printf("ERROR: IoTHubClient_LL_SendEventAsync..........FAILED!\r\n");
            }
            else
            {
              time(&sent_time);
              (void)printf("IoTHubClient_LL_SendEventAsync accepted message [%d] for transmission to IoT Hub.\r\n", (int)iterator);
            }
          }
          iterator++;
        }
        IoTHubClient_LL_DoWork(iotHubClientHandle);
        ThreadAPI_Sleep(10);

        if (MESSAGE_COUNT != 0 && callbackCounter >= MESSAGE_COUNT)
        {
          printf("exit\n");
          break;
        }
      } while (g_continueRunning);

      (void)printf("iothub_client_sample_mqtt has gotten quit message, call DoWork %d more time to complete final sending...\r\n", DOWORK_LOOP_NUM);
      size_t index = 0;
      for (index = 0; index < DOWORK_LOOP_NUM; index++)
      {
        IoTHubClient_LL_DoWork(iotHubClientHandle);
        ThreadAPI_Sleep(1);
      }
      IoTHubClient_LL_Destroy(iotHubClientHandle);
    }
    platform_deinit();
  }
}

