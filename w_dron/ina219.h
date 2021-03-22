#ifndef __INA219_
#define __INA219_

/***********************************************
 *                Header Files
 **********************************************/
#include "i2c_wrapper.h"

typedef struct _ina219_data {
    float volt;
    float current;
    float power;
} ina219_data;

/***********************************************
 *          Function Declaration
 **********************************************/
int   ina219_calibrate(int fd, float r_shunt_value);
void  *current_sensor(void *x_void_ptr);
void  check_current_overflow(int fd);
float ina219_get_bus_voltage(int fd);
float ina219_get_shunt_current(int fd);
float ina219_get_bus_power(int fd);
int   ina219_configure(int fd);
int   ina219_start(int *fd, int address);
ina219_data read_sensor_data(int i2c_slave_address);
ina219_data get_ina219_data(int fd, int i2c_slave_address);

#endif // __INA219_
