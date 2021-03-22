/***********************************************
 *                Header Files
 **********************************************/
#include "ads1115.h" 
/***********************************************
 *             Function Defination
 **********************************************/

int write_1115_i2c_word_data(int fd, unsigned char cmd, uint16_t reg_value) {

    uint16_t length = 3;
    unsigned char buffer[8] = {0};

    buffer[0] = cmd;
    buffer[1] = (reg_value & 0xFF00) >> 8;
    buffer[2] = (reg_value & 0x00FF);

    // Write slave device register and value
    if (write(fd, buffer, length) != length) {
        printf("Cell checker Failed to write data to the i2c bus.\n");
        return -1;
    }
    return 0;
}

int i2c_1115_dev_open(int *fd, const char *i2c_bus, int addr) {

    // Open I2C bus
    *fd = open(i2c_bus, O_RDWR);
    if (*fd < 0) {
        printf("Cell checker Failed to open i2c device, I2C Bus : %s Device Address: %x\n", i2c_bus, addr);
        return -1;
    }

    // Set working device
    if (ioctl(*fd, I2C_SLAVE, addr) < 0) {
        printf("Cell checker Failed to set i2c device as slave, I2C Bus : %s Device Address: %x\n", i2c_bus, addr);
        return -1;
    }

    return 0;
}

void i2c_1115_dev_close(int fd) {

    // Close I2C bus
    if (-1 != fd) {
        close(fd);
    }
    fd = -1;
}


///////////////////////////////////////////////////////////////////////////////////////
 uint16_t ads1115_read_i2c_word_data(int fd, unsigned char cmd) {

    uint16_t length = 0, value = 0;
    unsigned char buffer[8] = {0};

    buffer[0] = cmd;
    length = 1;
    // Write slave device register
    if (write(fd, buffer, length) != length) {
        printf("Cell checker Failed to write to the i2c bus.\n");
        return 0;
    }

    usleep(75000);

    length = 2;
    // Read slave fevice register value
    if (read(fd, buffer, length) != length) {
        printf("Cell checker Failed to read from the i2c bus.\n");
    }
    else {
        value = (((uint16_t) buffer[0]) << 8) | ((uint16_t) buffer[1]);
    }
    return value;
}

int ads1115_i2c_init(int *a1_fd, int *a2_fd, int *b1_fd, int *b2_fd) {   // 48 49 4a 4b
    
    int re = 0;
    if (i2c_1115_dev_open(a1_fd, "/dev/i2c-1", 0x48) < 0) {
        printf("Cell checker Fail to initialize ads1115 0x48 device\n");
        re = -1;
        *a1_fd = -1;
    }
    if (i2c_1115_dev_open(a2_fd, "/dev/i2c-1", 0x49) < 0) {
        printf("Cell checker Fail to initialize ads1115 0x49 device\n");
        re = -1;
        *a2_fd = -1;
    }
    if (i2c_1115_dev_open(b1_fd, "/dev/i2c-1", 0x4a) < 0) {
        printf("Cell checker Fail to initialize ads1115 0x4a device\n");
        re = -1;
        *b1_fd = -1;
    }
    if (i2c_1115_dev_open(b2_fd, "/dev/i2c-1", 0x4b) < 0) {
        printf("Cell checker Fail to initialize ads1115 0x4b device\n");
        re = -1;
        *b2_fd = -1;
    }

    return re;
}

double ads1115_get_data(int fd, int read_reg, int write_reg, int write_data) {
    
    double reg_data;
    double sell_data;
    if (write_1115_i2c_word_data(fd, write_reg, write_data) == -1) {
        printf("Cell checker Fail write word data\n");
        return -1;
    } 
    reg_data = ads1115_read_i2c_word_data(fd, read_reg);
    if(reg_data > 32767) reg_data -= 65535;
    sell_data = reg_data * 0.0001875;

    return sell_data;        
}

/////////////////////////////////////////////////////////////////////////////////////
/*
I2CDevice ioctl_ads1115_i2c_init(unsigned short addr) {
    
    I2CDevice data;

    int fd;
    if (i2c_dev_open(&fd, "/dev/i2c-1", addr) < 0) {
        printf("Fail to initialize ads1115 device\n");
    }
    memset(&data, 0, sizeof(data));
    data.bus = fd;	
    data.addr = addr;  
    data.iaddr_bytes = 1;
    
    return data;
}
    // I2CDevice a1, a2, b1, b2;
    // a1 = ioctl_ads1115_i2c_init(0x48);
    // a2 = ioctl_ads1115_i2c_init(0x49);
    // b1 = ioctl_ads1115_i2c_init(0x4a);
    // b2 = ioctl_ads1115_i2c_init(0x4b);

double ioctl_ads1115_get_data(I2CDevice fd, int data) {
    
    uint16_t value; 
    double cell;
    write_i2c_word_data(fd.bus, 0x01, data);         
    
    unsigned char buffer[2];
    ssize_t size = sizeof(buffer);
    memset(buffer, 0, sizeof(buffer));

    if ((i2c_ioctl_read(&fd, 0x00, buffer, size)) != size) {
        fprintf(stderr, "read error\n");
        exit(0);
    }
    value = (((uint16_t) buffer[0]) << 8) | ((uint16_t) buffer[1]);

    if(value > 32767) value -= 65535;
    cell = value*0.0001875;
    
    return cell;
}
        // double a1_cell = ioctl_ads1115_get_data(a1, 0x4043);
        // double a2_cell = ioctl_ads1115_get_data(a1, 0x5043);
        // double a3_cell = ioctl_ads1115_get_data(a1, 0x6043);
        // double a4_cell = ioctl_ads1115_get_data(a2, 0x5043);
        // double a5_cell = ioctl_ads1115_get_data(a2, 0x6043);
        // double a6_cell = ioctl_ads1115_get_data(a2, 0x7043);
        // double b1_cell = ioctl_ads1115_get_data(b1, 0x5043);
        // double b2_cell = ioctl_ads1115_get_data(b1, 0x6043);
        // double b3_cell = ioctl_ads1115_get_data(b1, 0x7043);
        // double b4_cell = ioctl_ads1115_get_data(b2, 0x5043);
        // double b5_cell = ioctl_ads1115_get_data(b2, 0x6043);
        // double b6_cell = ioctl_ads1115_get_data(b2, 0x7043);
        // printf("1111%f  ", a1_cell);
        // printf("2222%f  ", a2_cell);
        // printf("%f  ", a3_cell);
        // printf("%f  ", a4_cell);
        // printf("%f  ", a5_cell);
        // printf("%f\n", a6_cell);
        //a1_cell =0, a2_cell =0; //a3_cell =0, a4_cell =0, a5_cell =0, a6_cell =0;
*/