#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include <esp_attr.h>

#include <driver/gpio.h>

namespace pwm_capture
{
    extern const char *TAG;

    typedef struct
    {
        int64_t t0{0};
        int64_t duty{0};
        int64_t period{0};
        gpio_num_t gpio{GPIO_NUM_NC};
    } pwm_item_data_t;

    typedef struct
    {
        QueueHandle_t queue{NULL};
        pwm_item_data_t data;
        int64_t start{0};
    } isr_arg_t;

    esp_err_t init_for_all();
    esp_err_t deinit_for_all();

    class pwm_cap
    {
    private:
        char *TAG_ = NULL;
        isr_arg_t isr_arg_;

        gpio_num_t gpio_ = GPIO_NUM_NC;
        QueueHandle_t queue_ = NULL;

        bool initialized_ = false;
        bool started_ = false;

        bool created_queue_ = false;

    public:
        pwm_cap(gpio_num_t gpio, char *TAG);
        ~pwm_cap();

        esp_err_t init(UBaseType_t queue_length);
        esp_err_t init(QueueHandle_t queue);
        esp_err_t deinit(bool delete_queue);

        esp_err_t start();
        esp_err_t stop();

        gpio_num_t get_gpio();
        QueueHandle_t get_queue();
    };

} // namespace pwm_capture
