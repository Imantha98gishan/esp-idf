
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define GPIO_OUTPUT_IO_0 GPIO_NUM_0
#define GPIO_OUTPUT_PIN_SEL (1ULL << GPIO_OUTPUT_IO_0) 

#define GPIO_INPUT_IO_1 GPIO_NUM_2
#define GPIO_INPUT_PIN_SEL (1ULL << GPIO_INPUT_IO_1)

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;
uint32_t toggle=0;
// IRAM_ATTR
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    toggle^=1;
    gpio_set_level(GPIO_OUTPUT_IO_0,toggle);  
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    /*queues are used to passed data from interrupt to task
    if the gpio_evt_queue task leave the blocked state and
    pirority is higher than the current running task
    &xHigherPriorityTaskWoken set to pdTrue inside the xQueueSendFromISR
    this will allow user to use the ISR in safe manner*/
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);  
}

void app_main(void)
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
    gpio_set_level(GPIO_OUTPUT_IO_0,0);

    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    // bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO_1, gpio_isr_handler, (void *)GPIO_INPUT_IO_1);

    while (1);
}