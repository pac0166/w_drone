#ifndef __ADS1115_
#define __ADS1115_
/***********************************************
 *                Header Files
 **********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <dirent.h> 
#include <pthread.h>
#include <math.h>

typedef struct _ads1115 {   //no use
    float a_cell;
    float b_cell;
    float a1_cell, a2_cell, a3_cell, a4_cell, a5_cell, a6_cell;
    float b1_cell, b2_cell, b3_cell, b4_cell, b5_cell, b6_cell;
    float I;
    float a_watt;
    float b_watt;
    float a_per;
    float b_per;
    
} ads1115;
/***********************************************
 *          Function Declaration
 **********************************************/
int write_1115_i2c_word_data(int fd, unsigned char cmd, uint16_t reg_value);
int i2c_1115_dev_open(int *fd, const char *i2c_bus, int addr);
void i2c_1115_dev_close(int fd);

uint16_t ads1115_read_i2c_word_data(int fd, unsigned char cmd);
double ads1115_get_data(int fd, int read_reg, int write_reg, int write_data);
int ads1115_i2c_init(int *a1_fd, int *a2_fd, int *b1_fd, int *b2_fd);

// ioctl version
/*
I2CDevice ioctl_ads1115_i2c_init(unsigned short addr);
double ioctl_ads1115_get_data(I2CDevice fd, int data);
*/

 #endif // __ADS1115_
