idf_component_register(SRCS "src/pwm_capture.cpp"
                    INCLUDE_DIRS "include"
                    PRIV_REQUIRES esp_timer
                    REQUIRES driver)
