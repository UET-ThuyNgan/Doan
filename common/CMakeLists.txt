set(app_src bh1750.c dht11.c srf05.c mqtt_client_app.c wifi_station.c)
set(pri_req mqtt nvs_flash)
idf_component_register(SRCS ${app_src}
                    INCLUDE_DIRS "."
                    REQUIRES ${pri_req})
