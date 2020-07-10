#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <dht.h>

static const dht_sensor_type_t sensor_type = DHT_TYPE_AM2301;
static const gpio_num_t dht_gpio = 17;

void dht_test(void *pvParameters)
{
  float temperature = 0;
  float humidity = 0;

  while (1)
  {
    if (dht_read_float_data(sensor_type, dht_gpio, &humidity, &temperature) == ESP_OK) {
      printf("Humidity: %.2f%% Temp: %.2fC\n", humidity, temperature);
    } else {
      printf("Could not read data from sensor\n");
    }

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void app_main()
{
  xTaskCreate(dht_test, "dht_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
}
