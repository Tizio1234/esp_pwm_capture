#include "pwm_capture.hpp"

#include <esp_check.h>
#include <esp_log.h>

#include <driver/gpio.h>
#include <esp_timer.h>

void IRAM_ATTR gpio_isr_handler(void *arg)
{

    pwm_capture::isr_arg_t *isr_arg = static_cast<pwm_capture::isr_arg_t *> (arg);

    int level = gpio_get_level(isr_arg->data.gpio);
    int64_t tmp = esp_timer_get_time();
    if (level)
    {
        isr_arg->data.period = isr_arg->data.duty + tmp - isr_arg->start;
        isr_arg->start = tmp;
        xQueueSendFromISR(isr_arg->queue, &isr_arg->data, NULL);
    }
    else
    {
        isr_arg->data.duty = tmp - isr_arg->start;
        isr_arg->data.t0 = isr_arg->start;
        isr_arg->start = tmp;
    }
}

bool all_init = false;

namespace pwm_capture
{
    const char *TAG = "pwm capture";
    
    pwm_cap::pwm_cap(gpio_num_t gpio, char *TAG) : TAG_(TAG), gpio_(gpio)
    {
        isr_arg_.data.gpio = gpio_;
    }

    pwm_cap::~pwm_cap() {
        if (started_) stop();
        if (initialized_) deinit(created_queue_);
    }

    esp_err_t init_for_all()
    {
        if (all_init){
            ESP_LOGE(TAG, "ISR service already initialized");
            return ESP_ERR_INVALID_STATE;
        }

        ESP_RETURN_ON_ERROR(gpio_install_isr_service(0), TAG, "Failed to install gpio isr service");
        all_init = true;
        return ESP_OK;
    }

    esp_err_t deinit_for_all()
    {
        if (!all_init){
            ESP_LOGE(TAG, "ISR service not initialized yet");
            return ESP_ERR_INVALID_STATE;
        }

        gpio_uninstall_isr_service();
        all_init = false;
        return ESP_OK;
    }

    esp_err_t pwm_cap::init(UBaseType_t queue_length)
    {
        if (!all_init || initialized_){
            ESP_LOGE(TAG_, "Trying to initialize : %s", initialized_ ? "Already initialized" : "ISR service not initialized yet");
            return ESP_ERR_INVALID_STATE;
        }

        QueueHandle_t queue = xQueueCreate(queue_length, sizeof(pwm_item_data_t));
        if (!queue) {
            ESP_LOGE(TAG_, "Failed to create queue");
            return ESP_FAIL;
        }

        created_queue_ = true;
        
        return init(queue);
    }

    esp_err_t pwm_cap::init(QueueHandle_t queue)
    {
        if (!all_init || initialized_){
            ESP_LOGE(TAG_, "Trying to initialize : %s", initialized_ ? "Already initialized" : "ISR service not initialized yet");
            return ESP_ERR_INVALID_STATE;
        }

        gpio_reset_pin(gpio_);
        gpio_config_t io_conf = {
            .pin_bit_mask = ((uint64_t)1 << (uint64_t)gpio_),
            .mode = GPIO_MODE_INPUT,
            .pull_up_en = (gpio_pullup_t)0,
            .pull_down_en = (gpio_pulldown_t)1,
            .intr_type = GPIO_INTR_ANYEDGE,
        };

        ESP_RETURN_ON_ERROR(gpio_config(&io_conf), TAG_, "Failed to init gpio");

        queue_ = queue;
        isr_arg_.queue = queue_;

        initialized_ = true;

        return ESP_OK;
    }

    esp_err_t pwm_cap::deinit(bool delete_queue)
    {
        if (!initialized_){
            ESP_LOGE(TAG_, "Failed to deinitialize, not initialized yet");
            return ESP_ERR_INVALID_STATE;
        }

        if (delete_queue) vQueueDelete(queue_);
        ESP_RETURN_ON_ERROR(gpio_reset_pin(gpio_), TAG_, "Failed to reset gpio pin");

        initialized_ = false;
        return ESP_OK;
    }

    esp_err_t pwm_cap::start()
    {
        if (!initialized_ || started_){
            ESP_LOGE(TAG_, "Trying to start : %s", started_ ? "Already started" : "Not initialized yet");
            return ESP_ERR_INVALID_STATE;
        }

        ESP_RETURN_ON_ERROR(gpio_isr_handler_add(gpio_, gpio_isr_handler, (void *)&isr_arg_), TAG_, "Failed to add gpio isr handler");
        started_ = true;
        return ESP_OK;
    }

    esp_err_t pwm_cap::stop()
    {
        if (!started_){
            ESP_LOGE(TAG_, "Failed to stop, not started yet");
            return ESP_ERR_INVALID_STATE;
        }      

        ESP_RETURN_ON_ERROR(gpio_isr_handler_remove(gpio_), TAG_, "Failed to remove gpio isr handler");
        started_ = false;
        return ESP_OK;
    }

    gpio_num_t pwm_cap::get_gpio(){
        return gpio_;
    }

    QueueHandle_t pwm_cap::get_queue(){
        return queue_;
    }
} // namespace pwm_capture
