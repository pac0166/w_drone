#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <thread>

#include "i2c.h"
#include "i2c_wrapper.h"

#include "generic_port.h"
#include "serial_port.h"
#include "ina219.h"
#include "ads1115.h"
#include "tcp_port.h"
#include "nano_gpio.h"
//logggggggggggggggggggg
#include <time.h>           // currentDateTime
#include <fstream>          // log file make 
#include <iostream>
using namespace std;
int chan = MAVLINK_COMM_0;

Generic_Port *kcmvp_port; 
Generic_Port *fc_port;                                    
Generic_Port *ptz_port;
Generic_Port *pz10t_port;
Generic_Port *current_port;
//logggggggggggggggggggg
const std::string currentDateTime() 
{
    time_t     now = time(0); //time_t type save time 
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y.%m.%d-%X", &tstruct); // YYYY-MM-DD.HH:mm:ss string

    return buf;
}

void cell_thread(int *a1_fd, int *a2_fd, int *b1_fd, int *b2_fd) {    // ads1115
    
    mavlink_message_t message;
    // //logggggggggggggggg
    // std::string date = currentDateTime();        //get time 
    // std::ofstream out1(("./log/Cell_" + date + ".txt").c_str(), std::ios::app);  //make log file 

    while(1) 
    {
        //read a cell volt data (right battery, port)  
        float a1_cell = ads1115_get_data(*a1_fd, 0x00, 0x01, 0x4043)*1.5;
        float a2_cell = ads1115_get_data(*a1_fd, 0x00, 0x01, 0x5043);
        float a3_cell = ads1115_get_data(*a1_fd, 0x00, 0x01, 0x6043);
        float a4_cell = ads1115_get_data(*a2_fd, 0x00, 0x01, 0x5043);
        float a5_cell = ads1115_get_data(*a2_fd, 0x00, 0x01, 0x6043)*(22.26/21);
        float a6_cell = ads1115_get_data(*a2_fd, 0x00, 0x01, 0x7043)*(25.62/25.2);
        float a_cell = a1_cell + a2_cell + a3_cell + a4_cell + a5_cell + a6_cell;
        //read b cell volt data (left battery,  port)
        float b1_cell = ads1115_get_data(*b1_fd, 0x00, 0x01, 0x5043)*(30.24/29.4);
        float b2_cell = ads1115_get_data(*b1_fd, 0x00, 0x01, 0x6043)*(35.7/33.6);
        float b3_cell = ads1115_get_data(*b1_fd, 0x00, 0x01, 0x7043)*(38.64/37.8);        
        float b4_cell = ads1115_get_data(*b2_fd, 0x00, 0x01, 0x5043)*(42.42/42);
        float b5_cell = ads1115_get_data(*b2_fd, 0x00, 0x01, 0x6043);
        float b6_cell = ads1115_get_data(*b2_fd, 0x00, 0x01, 0x7043);
        float b_cell = b1_cell + b2_cell + b3_cell + b4_cell + b5_cell + b6_cell;
        
        //I, watt
        // I = (ads1115_get_data(*b1_fd, 0x00, 0x01, 0x4043)-2.489)/((4-(2.489-0.23))/400);
        // if(I<0) I=0.0001;
        // a_watt = a_cell * I;
        // b_watt = b_cell * I;
        
        //cell %
        float a_per = (a_cell-20.7)/4.5*100; 
        float b_per = (b_cell-20.7)/4.5*100; 
        
        if(a_cell<=0) a_cell=0, a_per=0, a1_cell=0, a2_cell=0, a3_cell=0, a4_cell=0, a5_cell=0, a6_cell=0 ;
        if(b_cell<=0) b_cell=0, b_per=0, b1_cell=0, b2_cell=0, b3_cell=0, b4_cell=0, b5_cell=0, b6_cell=0 ;

        //printf("A=%f  B=%f\n", a_cell, b_cell);
        //printf("A= %f %f %f %f %f %f\nB= %f %f %f %f %f %f\n\n", a1_cell,a2_cell,a3_cell,a4_cell,a5_cell,a6_cell,b1_cell,b2_cell,b3_cell,b4_cell,b5_cell,b6_cell);

        //mavlink_msg_a_battery_cell_pack_chan(1, 200, 3, &message, a_cell, a_per, a1_cell, a2_cell, a3_cell, a4_cell, a5_cell, a6_cell);
        //kcmvp_port->write_message(message); 
        //printf("cell_dataa\n");

        // //logggggggggggggggg
        // std::string adate = currentDateTime();
        // out1.setf(ios::fixed); 
        // out1.precision(2);
        // if (out1.is_open()) out1 << adate <<" "<<" a_per "<<a_per<<" a_cell "<<a_cell<<"  "<<a1_cell<<" "<<a2_cell<<" "<<a3_cell<<" "<<a4_cell<<" "<<a5_cell<<" "<<a6_cell<<std::endl;

        //mavlink_msg_b_battery_cell_pack_chan(1, 200, 3, &message, b_cell, b_per, b1_cell, b2_cell, b3_cell, b4_cell, b5_cell, b6_cell);
        //kcmvp_port->write_message(message);

        // //logggggggggggggggg
        // if (out1.is_open()) out1 << adate <<" "<<" b_per "<<b_per<<" b_cell "<<b_cell<<"  "<<b1_cell<<" "<<b2_cell<<" "<<b3_cell<<" "<<b4_cell<<" "<<b5_cell<<" "<<b6_cell<<std::endl;
        
        sleep(1);
    }
}

void fc_gcs_thread(uint16_t *pack_v) {
    mavlink_message_t message;
    mavlink_sys_status_t sys;

    while(1) 
    {
        if(fc_port->read_message(message, 1) == 1)
        {   
            switch(message.msgid)
            {
                case MAVLINK_MSG_ID_SYS_STATUS:
                {
                    mavlink_msg_sys_status_decode(&message, &sys);
                    //printf("%d\n", *pack_v);
                    mavlink_msg_sys_status_pack_chan(2, message.compid, chan, &message, sys.onboard_control_sensors_present, sys.onboard_control_sensors_enabled
                    , sys.onboard_control_sensors_health, sys.load, *pack_v, sys.current_battery, sys.battery_remaining, sys.drop_rate_comm, sys.errors_comm
                    , sys.errors_count1, sys.errors_count2, sys.errors_count3, sys.errors_count4);
                    kcmvp_port->write_message(message);
                    //printf("%d %d %d %d %d\n", message.len, message.seq, message.sysid, message.compid, message.msgid);
                    break; 
                }
                
                default:  
                {
                    kcmvp_port->write_message(message);
                    //printf("                      %d %d %d %d %d\n", message.len, message.seq, message.sysid, message.compid, message.msgid);
                    break;
                }
            }
        }    
    }
    // char buf;
    // while(1)
    // {
    //     int result = fc_port->fc_read_buf(&buf);
    //     //printf("%x\n", result);
    //     if(result > 0)
    //     {
    //         kcmvp_port->write_buf(&buf);///////////////////////////////////
    //         memset(&buf,0,sizeof(buf));
    //     }
    // }
}

// void ptz_gcs_thread() {     /////////////////////////////////////////////////  print /r /n version

//     mavlink_message_t message;
//     char buf[30];
//     while(1)
//     {   
//         int result = ptz_port->ptz_read_buf(buf, 30);
//         printf("%s", buf);
//         if(result > 0)
//         {
//             mavlink_msg_ptz_pack_chan(1, 200, 1, &message, 0, buf);
//             kcmvp_port->write_message(message);
//             memset(buf,0,sizeof(buf));
//         }
//     }
// }
int read_ptz_msg(char buffer[]) {       // ptz_gcs_thread() use      
    
    char buf = 0;
    while(1)
    {
        ptz_port->fc_read_buf(&buf); //read buf

        if(buf==0x5b)  // [ //start save data
        {    
            buffer[0] = buf;
            for(int i=1; i<=40; i++) 
            {
                ptz_port->fc_read_buf(&buf);        
                if(buf==0x0d || buf==0x0a)  // check /n, /r
                {
                    //printf("ptz_msg_error\n");
                    //printf("%x\n", buf);
                    return 0;
                }
                buffer[i] =  buf;
                //printf("%x\n", buf);
            }
            return 0;  //ok        
        }
    }
}
void ptz_gcs_thread() {     ///////////////////////////////////////////////// delete /r /n

    mavlink_message_t message;

    char ptz_msg[40];

    rename("/home/usis/Desktop/log/ptz2.txt", "/home/usis/Desktop/log/ptz3.txt");
    rename("/home/usis/Desktop/log/ptz1.txt", "/home/usis/Desktop/log/ptz2.txt");
    rename("/home/usis/Desktop/log/ptz0.txt", "/home/usis/Desktop/log/ptz1.txt");
    std::ofstream out1(("/home/usis/Desktop/log/ptz0.txt"), std::ios::out | std::ios::trunc);  //make log file

    while(1)
    {   
        memset(ptz_msg, 0, sizeof(ptz_msg));
        int status = read_ptz_msg(ptz_msg);
        if(status == 0)
        {
            //mavlink_msg_ptz_pack_chan(1, 200, 1, &message, 0, ptz_msg);
            //kcmvp_port->write_message(message);
            std::string adate = currentDateTime();
            if (out1.is_open()) out1 << adate << " " << ptz_msg <<std::endl;
            printf("%s\n", ptz_msg);
        }
        status = -1;
    }
}

void gcs_fc_thread(float *cmd1, float *cmd2) {

    mavlink_message_t message;
    //mavlink_bec_t bec;
    //mavlink_ptz_t ptz;
    mavlink_command_long_t ptz;

    // //logggggggggggggggg
    // std::string date = currentDateTime();        //get time 
    // std::ofstream out1(("./log/GCS_" + date + ".txt").c_str(), std::ios::app);  //make log file 

    while(1) 
    {  
        if(kcmvp_port->read_message(message, 2) == 1)
        {
            switch(message.msgid)
            {
                /*case MAVLINK_MSG_ID_BEC:
                    mavlink_msg_bec_decode(&message, &bec);
                    //printf("print on,off message=%x\n", bec.bec_on_off);
                    *on_off = bec.bec_on_off;
                    break;*/
                
                // case MAVLINK_MSG_ID_PTZ:        ///////////////////////////////////////////////////
                //     mavlink_msg_ptz_decode(&message, &ptz);
                //     //printf("cmd = 0x%lX   msg = %s\n", ptz.ptz_cmd, ptz.ptz_msg);
                //     //ptz_port->write_ptz(0xe11e00f11f);
                //     *ptz_cmd = ptz.ptz_cmd;
                //     break;

                case MAVLINK_MSG_ID_COMMAND_LONG:
                    mavlink_msg_command_long_decode(&message, &ptz);
                    if(ptz.param1==9.0|ptz.param1==10.0|ptz.param1==11.0|ptz.param1==21.0|ptz.param1==22.0
                        |ptz.param1==23.0|ptz.param1==24.0|ptz.param1==25.0|ptz.param1==26.0) 
                    {
                        *cmd1 = ptz.param1;
                        *cmd2 = ptz.param2;
                    }
                    //printf("cmd1 = %f       cmd2 = %f\n", ptz.param1, ptz.param2);
                    fc_port->write_message(message);  
                    break;

                default:    
                    fc_port->write_message(message);
                    // //logggggggggggggggg
                    // std::string adate = currentDateTime();
                    // std::cout << adate ;
                    // printf("           gcs-> %d\n", message.msgid);
                    // out1.setf(ios::fixed); 
                    // out1.precision(2);
                    // if (out1.is_open()) out1 << adate <<" "<<" GCS "<<message.msgid<<std::endl;
                    break;

            }
        }
    }
}

void bec_thread(int *pipe_fd, uint16_t *pack_v) {
    
    mavlink_message_t message;
    int bus;
    if ((bus = i2c_open("/dev/i2c-1")) == -1) {
        fprintf(stderr, "i2c_bec_open error\n");
        exit(0);
    }

    I2CDevice bp;
    memset(&bp, 0, sizeof(bp));
    bp.bus = bus;	
    bp.addr = 0x64;   //64
    bp.iaddr_bytes = 1;	
    bp.page_bytes = 16; 

    //config 0x64 
    unsigned char buffer[2];
    ssize_t size = sizeof(buffer);
    i2c_read(&bp, 0x64, buffer, size);
    write_i2c_byte_data(bp.bus, 0x01, 0xfc);
    usleep(16000);

    // //logggggggggggggggg
    // std::string date = currentDateTime();        //get time 
    // std::ofstream out1(("./log/PACK_" + date + ".txt").c_str(), std::ios::app);  //make log file 
        
    while(1)
    {
        for(int i=0; i<=40; i++)
        {
            // 1 get main buffer
            unsigned char bp_buffer[24];
            ssize_t bp_size = sizeof(bp_buffer);
            memset(bp_buffer, 0, sizeof(bp_buffer));
            if ((i2c_read(&bp, 0x64, bp_buffer, bp_size)) != bp_size) {
                fprintf(stderr, "BEC/Pack read error\n");
                //exit(0);
            }

            //bp volt
            unsigned char bp_volt_msb = bp_buffer[8];
            unsigned char bp_volt_lsb = bp_buffer[9];
            float bp_volt_hex_data = (bp_volt_msb << 8) | bp_volt_lsb;
            float bp_volt = 70.8*(bp_volt_hex_data/65535); 
            
            *pack_v = static_cast<int>(bp_volt*1000);  
            //printf("%d\n", *pack_v);
            
            //ssr on,off command(use pipe send other process) ////////////////////////////////////////////
            if(bp_volt < 46) {
                int cmd = 1; //pm on == ssr on
                int a = write(pipe_fd[1], &cmd, sizeof(cmd));
            }
            // else if(bp_volt > 30) {
            //     int cmd = 2; //pm off == ssr off
            //     int a = write(pipe_fd[1], &cmd, sizeof(cmd));
            // }
            else {
                int cmd = 0; // if don't send / recv part stop
                int a = write(pipe_fd[1], &cmd, sizeof(cmd));
            }
            //ssr on,off command(use pipe send other process) ////////////////////////////////////////////

            //ampere
            unsigned char bp_ampere_msb = bp_buffer[14];
            unsigned char bp_ampere_lsb = bp_buffer[15];
            float bp_ampere_hex_data = (bp_ampere_msb << 8) | bp_ampere_lsb;
            float bp_ampere_data = bp_ampere_hex_data - 0x7fff;
            if(bp_ampere_data < 0) bp_ampere_data = -bp_ampere_data;
            float bp_ampere = (64/50)*(bp_ampere_data/0x7fff);  
            if(bp_ampere_hex_data <= 0.0) bp_ampere = 0;
            //temp
            unsigned char bp_temp_msb = bp_buffer[20];
            unsigned char bp_temp_lsb = bp_buffer[21];
            float bp_temp_hex_data = (bp_temp_msb << 8) | bp_temp_lsb;
            float bp_temp = (510*bp_temp_hex_data/0xffff)-273.15; 
            if(bp_temp_hex_data <= 0.0) bp_temp = 0;
            usleep(18000);

            //printf("%f   %f   %f\n", bp_volt, bp_ampere, bp_temp);
            
            /*if(i==40)
            {
                //send to kcmvp LTC2944 data
                mavlink_msg_battery_pack_pack_chan(1, 200 , 1,&message, bp_volt, bp_ampere, bp_temp);
                kcmvp_port->write_message(message);
                
                //mavlink_msg_sys_status_pack_chan(1, 200, 1, &message,0,0,0,0,test_volt,0,0,0,0,0,0,0,0);
                //kcmvp_port->write_message(message);
                //printf("%d\n",test_volt);
                
                // //logggggggggggggggg
                // std::string adate = currentDateTime();
                // out1.setf(ios::fixed); 
                // out1.precision(2);
                // if (out1.is_open()) out1 << adate <<" "<<" volt "<<bp_volt<<" current "<<bp_ampere<<" temp "<<bp_temp<<std::endl;
            }*/
        }
    }
}

void pack_thread() {
    
    mavlink_message_t message;
    int fd_40=-1, fd_42=-1, fd_43=-1, fd_44=-1, fd_45=-1, fd_46=-1, fd_47=-1, fd_4c=-1, fd_4d=-1, fd_4e=-1;
    ina219_start(&fd_40, 0x40);
    ina219_start(&fd_42, 0x42);
    ina219_start(&fd_43, 0x43);
    ina219_start(&fd_44, 0x44);
    ina219_start(&fd_45, 0x45);
    ina219_start(&fd_46, 0x46);
    ina219_start(&fd_47, 0x47);
    ina219_start(&fd_4c, 0x4c);
    ina219_start(&fd_4d, 0x4d);
    ina219_start(&fd_4e, 0x4e);

    // //logggggggggggggggg
    // std::string date = currentDateTime();        //get time 
    // std::ofstream out1(("./log/BEC_" + date + ".txt").c_str(), std::ios::app);  //make log file 

    while(1)
    {
        //INA219 data 
        ina219_data data;
        data = get_ina219_data(fd_40, 0x40);
        //mavlink_msg_bec_pack_chan(1, 200, 2, &message, 0, 0x40, data.volt, data.power, data.current);
        //kcmvp_port->write_message(message);
        //printf("bec_dataa\n");

        // //logggggggggggggggg
        // std::string adate = currentDateTime();
        // out1.setf(ios::fixed); 
        // out1.precision(2);
        // if (out1.is_open()) out1 << adate <<" "<<" addr=01(40) "<<" Volt "<<data.volt<<" Current "<<data.current<<" Power "<<data.power<<std::endl;

        data = get_ina219_data(fd_4e, 0x4e);
        //mavlink_msg_bec_pack_chan(1, 200, 2, &message, 0, 0x4e, data.volt, data.power, data.current);
        //kcmvp_port->write_message(message);
        //if (out1.is_open()) out1 << adate <<" "<<" addr=02(4e) "<<" Volt "<<data.volt<<" Current "<<data.current<<" Power "<<data.power<<std::endl;
        //printf("v=%f  p=%f  a=%f\n", data.volt, data.power, data.current);
        data = get_ina219_data(fd_47, 0x47);
        //mavlink_msg_bec_pack_chan(1, 200, 2, &message, 0, 0x47, data.volt, data.power, data.current);
        //kcmvp_port->write_message(message);
        //if (out1.is_open()) out1 << adate <<" "<<" addr=03(47) "<<" Volt "<<data.volt<<" Current "<<data.current<<" Power "<<data.power<<std::endl;
        //printf("v=%f  p=%f  a=%f\n", data.volt, data.power, data.current);
        data = get_ina219_data(fd_4d, 0x4d);
        //mavlink_msg_bec_pack_chan(1, 200, 2, &message, 0, 0x4d, data.volt, data.power, data.current);
        //kcmvp_port->write_message(message);
        //if (out1.is_open()) out1 << adate <<" "<<" addr=04(4d) "<<" Volt "<<data.volt<<" Current "<<data.current<<" Power "<<data.power<<std::endl;
        //printf("v=%f  p=%f  a=%f\n", data.volt, data.power, data.current);
        data = get_ina219_data(fd_4c, 0x4c);
        //mavlink_msg_bec_pack_chan(1, 200, 2, &message, 0, 0x4c, data.volt, data.power, data.current);
        //kcmvp_port->write_message(message);
        //if (out1.is_open()) out1 << adate <<" "<<" addr=05(4c) "<<" Volt "<<data.volt<<" Current "<<data.current<<" Power "<<data.power<<std::endl;
        //printf("v=%f  p=%f  a=%f\n", data.volt, data.power, data.current);
        data = get_ina219_data(fd_46, 0x46);
        //mavlink_msg_bec_pack_chan(1, 200, 2, &message, 0, 0x46, data.volt, data.power, data.current);
        //kcmvp_port->write_message(message);
        //if (out1.is_open()) out1 << adate <<" "<<" addr=06(46) "<<" Volt "<<data.volt<<" Current "<<data.current<<" Power "<<data.power<<std::endl;
        //printf("v=%f  p=%f  a=%f\n", data.volt, data.power, data.current);
        data = get_ina219_data(fd_44, 0x44);
        //mavlink_msg_bec_pack_chan(1, 200, 2, &message, 0, 0x44, data.volt, data.power, data.current);
        //kcmvp_port->write_message(message);
        //if (out1.is_open()) out1 << adate <<" "<<" addr=07(44) "<<" Volt "<<data.volt<<" Current "<<data.current<<" Power "<<data.power<<std::endl;
        //printf("v=%f  p=%f  a=%f\n", data.volt, data.power, data.current);
        data = get_ina219_data(fd_43, 0x43);
        //mavlink_msg_bec_pack_chan(1, 200, 2, &message, 0, 0x43, data.volt, data.power, data.current);
        //kcmvp_port->write_message(message);
        //if (out1.is_open()) out1 << adate <<" "<<" addr=08(43) "<<" Volt "<<data.volt<<" Current "<<data.current<<" Power "<<data.power<<std::endl;
        //printf("v=%f  p=%f  a=%f\n", data.volt, data.power, data.current);
        data = get_ina219_data(fd_45, 0x45);
        //mavlink_msg_bec_pack_chan(1, 200, 2, &message, 0, 0x45, data.volt, data.power, data.current);
        //kcmvp_port->write_message(message);
        //if (out1.is_open()) out1 << adate <<" "<<" addr=09(45) "<<" Volt "<<data.volt<<" Current "<<data.current<<" Power "<<data.power<<std::endl;
        //printf("v=%f  p=%f  a=%f\n", data.volt, data.power, data.current);
        data = get_ina219_data(fd_42, 0x42);
        //mavlink_msg_bec_pack_chan(1, 200, 2, &message, 0, 0x42, data.volt, data.power, data.current);
        //kcmvp_port->write_message(message);
        //if (out1.is_open()) out1 << adate <<" "<<" addr=10(42) "<<" Volt "<<data.volt<<" Current "<<data.current<<" Power "<<data.power<<std::endl<<std::endl;
        //printf("v=%f  p=%f  a=%f\n\n", data.volt, data.power, data.current);
        sleep(1);
    }
}

void IO_port_thread(int *pipe_fd, int *pm_cmd) {

    int fd;
    if (i2c_dev_open(&fd, "/dev/i2c-1", 0x41) < 0) {
        printf("IO port Fail to initialize 0x41 device\n");
    }
    write_i2c_byte_data(fd, 0x03, 0x00); //config
    usleep(18000);
    //write_i2c_byte_data(fd, 0x01, 0xb);  // addr 0x46, 0100 off
    
    for(int i=0; i<=30; i++) {           // pipe read 30
        int temp;
        read(pipe_fd[0], &temp, sizeof(temp)); //read pipe    
    }
    
    while(1)
    {
        //int msg;
        int buf;
        read(pipe_fd[0], &buf, sizeof(buf));  //read pipe ssr
        //printf("%d\n", buf);
        /*if(buf == 1) 
        {
            char on_46;
            on_46 = read_i2c_byte_data(fd, 0x00); //ssr on
            on_46 |= (0x1<<2);
            write_i2c_byte_data(fd, 0x01, on_46);
        }*/
        /* else if(buf == 2) // off ssr, not use 
        {
            char off_46;  
            off_46 = read_i2c_byte_data(fd, 0x00);
            off_46 &= ~(0x1<<2);
            write_i2c_byte_data(fd, 0x01, off_46);
        } */
        switch(*pm_cmd)
        {
            case 1:  //cam power on
            {
                printf("cam power on\n");
                char on_4c;
                on_4c = read_i2c_byte_data(fd, 0x00);
                on_4c |= (0x1<<3);
                write_i2c_byte_data(fd, 0x01, on_4c);
                *pm_cmd = 0;
                break;
            }
            case 2:  //cam power off
            {
                printf("cam power off\n");
                char off_4c;
                off_4c = read_i2c_byte_data(fd, 0x00);
                off_4c &= ~(0x1<<3);
                write_i2c_byte_data(fd, 0x01, off_4c);
                *pm_cmd = 0;
                break;
            }
            case 3:  //LED on
            {
                printf("LED on\n");
                char on_4d;
                on_4d = read_i2c_byte_data(fd, 0x00);
                on_4d |= (0x1<<1);
                write_i2c_byte_data(fd, 0x01, on_4d);
                *pm_cmd = 0;
                break;
            }
            case 4:  //LED off
            {
                printf("LED off\n");
                char off_4d;
                off_4d = read_i2c_byte_data(fd, 0x00);
                off_4d &= ~(0x1<<1);
                write_i2c_byte_data(fd, 0x01, off_4d);
                *pm_cmd = 0;
                break;
            }
            default:
            {
                break;
            }
        }
    }
} 

void w_IO_port_thread(int *pipe_fd, int *pm_cmd) {       ///////////////////////// SSR active

    int fd;
    if (i2c_dev_open(&fd, "/dev/i2c-1", 0x41) < 0) {
        printf("IO port Fail to initialize 0x41 device\n");
    }
    write_i2c_byte_data(fd, 0x03, 0x00); //config
    usleep(18000);
    write_i2c_byte_data(fd, 0x01, 0xb);  // addr 0x46, 0100 off
    
    for(int i=0; i<=30; i++) {           // pipe read 30
        int temp;
        read(pipe_fd[0], &temp, sizeof(temp)); //read pipe    
    }
    
    while(1)
    {
        //int msg;
        int buf;
        read(pipe_fd[0], &buf, sizeof(buf));  //read pipe ssr
        //printf("%d\n", buf);
        if(buf == 1) 
        {
            char on_46;
            on_46 = read_i2c_byte_data(fd, 0x00); //ssr on
            on_46 |= (0x1<<2);
            write_i2c_byte_data(fd, 0x01, on_46);
        }
        /* else if(buf == 2) // off ssr, not use 
        {
            char off_46;  
            off_46 = read_i2c_byte_data(fd, 0x00);
            off_46 &= ~(0x1<<2);
            write_i2c_byte_data(fd, 0x01, off_46);
        } */
        switch(*pm_cmd)
        {
            case 1:  //cam power on
            {
                printf("cam power on\n");
                char on_4c;
                on_4c = read_i2c_byte_data(fd, 0x00);
                on_4c |= (0x1<<3);
                write_i2c_byte_data(fd, 0x01, on_4c);
                *pm_cmd = 0;
                break;
            }
            case 2:  //cam power off
            {
                printf("cam power off\n");
                char off_4c;
                off_4c = read_i2c_byte_data(fd, 0x00);
                off_4c &= ~(0x1<<3);
                write_i2c_byte_data(fd, 0x01, off_4c);
                *pm_cmd = 0;
                break;
            }
            case 3:  //LED on
            {
                printf("LED on\n");
                char on_4d;
                on_4d = read_i2c_byte_data(fd, 0x00);
                on_4d |= (0x1<<1);
                write_i2c_byte_data(fd, 0x01, on_4d);
                *pm_cmd = 0;
                break;
            }
            case 4:  //LED off
            {
                printf("LED off\n");
                char off_4d;
                off_4d = read_i2c_byte_data(fd, 0x00);
                off_4d &= ~(0x1<<1);
                write_i2c_byte_data(fd, 0x01, off_4d);
                *pm_cmd = 0;
                break;
            }
            default:
            {
                break;
            }
        }
        //usleep(10000);

        /* if (msg == *on_off) continue;
        else msg = *on_off;
        
        switch(msg)         //togle on,off switch
        {
            case 1:         //0001 = 1 fc
                char off_47;  
                off_47 = read_i2c_byte_data(fd, 0x00);    // off
                off_47 &= ~(0x1<<0);
                write_i2c_byte_data(fd, 0x01, off_47);
                break;
            case 2:         
                char on_47;
                on_47 = read_i2c_byte_data(fd, 0x00);     // on 
                on_47 |= (0x1<<0);
                write_i2c_byte_data(fd, 0x01, on_47);
                break;

            case 3:         //0010 = 2  led
                char off_4d;
                off_4d = read_i2c_byte_data(fd, 0x00);
                off_4d &= ~(0x1<<1);
                write_i2c_byte_data(fd, 0x01, off_4d);
                break;
            case 4:        
                char on_4d;
                on_4d = read_i2c_byte_data(fd, 0x00);
                on_4d |= (0x1<<1);
                write_i2c_byte_data(fd, 0x01, on_4d);
                break;

            case 5:        //0100 = 4  ssr
                char off_46;  
                off_46 = read_i2c_byte_data(fd, 0x00);
                off_46 &= ~(0x1<<2);
                write_i2c_byte_data(fd, 0x01, off_46);
                //printf("5555\n");
                break;
            case 6:         
                char on_46;
                on_46 = read_i2c_byte_data(fd, 0x00);
                on_46 |= (0x1<<2);
                write_i2c_byte_data(fd, 0x01, on_46);
                //printf("6666\n");
                break;

            case 7:         //1000 = camera
                char off_4c;
                off_4c = read_i2c_byte_data(fd, 0x00);
                off_4c &= ~(0x1<<3);
                write_i2c_byte_data(fd, 0x01, off_4c);
                break;
            case 8:        
                char on_4c;
                on_4c = read_i2c_byte_data(fd, 0x00);
                on_4c |= (0x1<<3);
                write_i2c_byte_data(fd, 0x01, on_4c);
                break;
            default:
                break;
        } */
    }
} 

void ptz_thread(float *cmd1, float *cmd2, int *pm_cmd)      ////////////////////////////////
{
    uint64_t ptz_stop    = 0xe11e00f11f;
    uint64_t cam_stop    = 0xe11e07f11f;
    uint64_t down        = 0xe11e61f11f;
    uint64_t up          = 0xe11e62f11f;
    uint64_t right       = 0xe11e63f11f;
    uint64_t left        = 0xe11e64f11f;
    uint64_t cam_in      = 0xe11e86f11f;
    uint64_t cam_out     = 0xe11e85f11f;
    uint64_t cam_reset   = 0xe11e0bf11f;
    uint64_t tk_cross    = 0xe11e14f11f;
    uint64_t tk_on       = 0xe11e15f11f;
    uint64_t tk_off      = 0xe11e16f11f;
    uint64_t power_off   = 0xe11e19f11f;
    uint64_t power_on    = 0xe11e39f11f;
    uint64_t op_ir       = 0xe11e13f11f;
    uint64_t ir_op       = 0xe11e33f11f;
    uint64_t op          = 0xe11e53f11f;
    uint64_t ir          = 0xe11e73f11f;

    int _cmd1;
    float _cmd2;

    while(1)
    {   
        //_cmd data make 
        if(_cmd1 == static_cast<int>(*cmd1) & _cmd2 == *cmd2) 
        {
            usleep(1000);
            continue;
        }
        else 
        {
            _cmd1 = static_cast<int>(*cmd1);
            _cmd2 = *cmd2;
            printf("cmd1 = %d   cmd2 = %f\n", _cmd1, _cmd2);  //get midas controll data
        }

        switch(_cmd1)
        {
            case 9:    //ptz /up, down
            {
                if(_cmd2 == 1492)
                {
                    printf("ptz up down stop\n");
                    ptz_port->write_ptz(&ptz_stop);
                }
                else if(_cmd2 < 1492 & _cmd2 != 0) 
                {
                    printf("ptz up\n");
                    ptz_port->write_ptz(&up);
                    //usleep(100000);
                    //ptz_port->write_ptz(&ptz_stop);
                }
                else if(_cmd2 > 1492 & _cmd2 != 0) 
                {
                    printf("ptz down\n");
                    ptz_port->write_ptz(&down);
                }
                break;
            }
            case 10:    //ptz /left, right 
            {
                if(_cmd2 == 1492)
                {
                    printf("ptz left right stop\n");
                    ptz_port->write_ptz(&ptz_stop);
                }
                else if(_cmd2 < 1492 & _cmd2 != 0) 
                {
                    printf("lefte\n");
                    ptz_port->write_ptz(&left);
                }
                else if(_cmd2 > 1492 & _cmd2 != 0) 
                {
                    printf("right\n");
                    ptz_port->write_ptz(&right);
                }
                break;
            }
            case 11:    //ptz zoom /in, out
            {
                if(_cmd2 == 1492)
                {
                    printf("ptz zoom in out stop\n");
                    ptz_port->write_ptz(&cam_stop);
                }
                else if(_cmd2 < 1492 & _cmd2 != 0)
                {
                    printf("zoom in\n");
                    ptz_port->write_ptz(&cam_in);
                }
                else if(_cmd2 > 1492 & _cmd2 != 0)
                {
                    printf("zoom out\n");
                    ptz_port->write_ptz(&cam_out);
                }
                break;
            }
            case 21:    //camera power /on, off  ///////////////////////////bec controll
            {
                if(_cmd2 == 11) // on
                {
                    *pm_cmd = 1;
                }
                else if(_cmd2 == 10) // off
                {
                    *pm_cmd = 2;
                }
                break;
            }
            case 22:    //camera screen /on, off
            {
                if(_cmd2 == 11)
                {
                    printf("cam screen on\n");
                    ptz_port->write_ptz(&power_on);
                }
                else if(_cmd2 == 10)
                {
                    printf("cam screen off\n");
                    ptz_port->write_ptz(&power_off);
                }
                break;
            }
            case 23:    //set position init
            {
                if(_cmd2 == 11)
                {
                    printf("ptz set position init\n");
                    ptz_port->write_ptz(&cam_reset);
                }
                break;
            }
            case 24:    //tracking /on, off, cross
            {
                if(_cmd2 == 11)
                {
                    printf("tk on\n");
                    ptz_port->write_ptz(&tk_on);
                }
                else if(_cmd2 == 10)
                {
                    printf("tk off\n");
                    ptz_port->write_ptz(&tk_off);
                }
                else if(_cmd2 == 12)
                {
                    printf("tk cross\n");
                    ptz_port->write_ptz(&tk_cross);
                }
                break;
            }
            case 25:    //screen /optical, ir, ir'optical, optical'ir
            {
                if(_cmd2 == 11)
                {
                    printf("optical (IR)\n");
                    ptz_port->write_ptz(&op_ir);
                }
                else if(_cmd2 == 12)
                {
                    printf("IR (optical)\n");
                    ptz_port->write_ptz(&ir_op);
                }
                else if(_cmd2 == 13)
                {
                    printf("optical\n");
                    ptz_port->write_ptz(&op);
                }
                else if(_cmd2 == 14)
                {
                    printf("IR\n");
                    ptz_port->write_ptz(&ir);
                }
                break;
            }
            case 26:    //LED /on, off    ////////////////////////////////bec controll
            {
                if(_cmd2 == 11)  //on
                {
                    *pm_cmd = 3;
                }
                else if(_cmd2 == 10) //off
                {
                    *pm_cmd = 4;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

void pz10t_thread(float *cmd1, float *cmd2, int *pm_cmd)      ////////////////////////////////
{
    char stop[20] =     {0xff,0x01,0x0f,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; //stop
    char up[20] =       {0xff,0x01,0x0f,0x10,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xd4,0xfe,0x00,0x00,0x00,0x00,0xd6}; //up
    char down[20] =     {0xff,0x01,0x0f,0x10,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2c,0x01,0x00,0x00,0x00,0x00,0x31}; //down
    char left[20] =     {0xff,0x01,0x0f,0x10,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xd4,0xfe,0xd6}; //left
    char right[20] =    {0xff,0x01,0x0f,0x10,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x2c,0x01,0x31}; //right
    char cam_reset[6] = {0x3e,0x45,0x01,0x46,0x12,0x12}; //camera reset
    char zoom_in[6] =   {0x81,0x01,0x04,0x07,0x27,0xff}; //camera zoom in
    char zoom_out[6] =  {0x81,0x01,0x04,0x07,0x37,0xff}; //camera zoom out
    char zoom_stop[6] = {0x81,0x01,0x04,0x07,0x00,0xff}; //camera zoom in
    
    int _cmd1;
    float _cmd2;

    //int a = -300;
    //printf("%x\n", a);

    while(1)
    {   
        //_cmd data make 
        if(_cmd1 == static_cast<int>(*cmd1) & _cmd2 == *cmd2) 
        {
            usleep(1000);
            continue;
        }
        else 
        {
            _cmd1 = static_cast<int>(*cmd1);
            _cmd2 = *cmd2;
            printf("cmd1 = %d   cmd2 = %f\n", _cmd1, _cmd2);  //get midas controll data
        }
        

        switch(_cmd1)
        {
            case 9:    //ptz /up, down
            {
                if(_cmd2 == 1492)
                {
                    printf("pz10t_up_doun_stop\n");
                    pz10t_port->write_pz10t(stop, 20);
                }
                else if(_cmd2 < 1492 & _cmd2 != 0) 
                {
                    printf("pz10t_up\n");
                    pz10t_port->write_pz10t(up, 20);
                }
                else if(_cmd2 > 1492 & _cmd2 != 0) 
                {
                    printf("pz10t down\n");
                    pz10t_port->write_pz10t(down, 20);
                }
                break;
            }
            case 10:    //ptz /left, right 
            {
                if(_cmd2 == 1492)
                {
                    printf("pz10t_right_left_stop\n");
                    pz10t_port->write_pz10t(stop, 20);
                }
                else if(_cmd2 < 1492 & _cmd2 != 0) 
                {
                    printf("pz10t_left\n");
                    pz10t_port->write_pz10t(left, 20);
                }
                else if(_cmd2 > 1492 & _cmd2 != 0) 
                {
                    printf("pz10t_right\n");
                    pz10t_port->write_pz10t(right, 20);
                }
                break;
            }
            case 11:    //ptz zoom /in, out
            {
                if(_cmd2 == 1492)
                {
                    printf("pz10t_zoom_in_out_stop\n");
                    pz10t_port->write_pz10t(zoom_stop, 6);
                }
                else if(_cmd2 < 1492 & _cmd2 != 0)
                {
                    printf("pz10t_zoom out\n");
                    pz10t_port->write_pz10t(zoom_out, 6);
                }
                else if(_cmd2 > 1492 & _cmd2 != 0)
                {
                    printf("pz10t_zoom in\n");
                    pz10t_port->write_pz10t(zoom_in, 6);
                }
                break;
            }
            case 21:    //camera power /on, off  ///////////////////////////bec controll
            {
                if(_cmd2 == 11) // on
                {
                    *pm_cmd = 1;
                }
                else if(_cmd2 == 10) // off
                {
                    *pm_cmd = 2;
                }
                break;
            }
            // case 22:    //camera screen /on, off
            // {
            //     if(_cmd2 == 11)
            //     {
                    
            //     }
            //     else if(_cmd2 == 10)
            //     {
                    
            //     }
            //     break;
            // }
            case 23:    //set position init
            {
                if(_cmd2 == 11)
                {
                    printf("pz10t_cam_reset\n");
                    pz10t_port->write_pz10t(cam_reset, 6); 
                }
                break;
            }
            // case 24:    //tracking /on, off, cross
            // {
            //     if(_cmd2 == 11)
            //     {
                   
            //     }
            //     else if(_cmd2 == 10)
            //     {
                    
            //     }
            //     else if(_cmd2 == 12)
            //     {
                    
            //     }
            //     break;
            // }
            // case 25:    //screen /optical, ir, ir'optical, optical'ir
            // {
            //     if(_cmd2 == 11)
            //     {
                   
            //     }
            //     else if(_cmd2 == 12)
            //     {
                    
            //     }
            //     else if(_cmd2 == 13)
            //     {
                    
            //     }
            //     else if(_cmd2 == 14)
            //     {
                    
            //     }
            //     break;
            // }
            case 26:    //LED /on, off    ////////////////////////////////bec controll
            {
                if(_cmd2 == 11)  //on
                {
                    *pm_cmd = 3;
                }
                else if(_cmd2 == 10) //off
                {
                    *pm_cmd = 4;
                }
                break;
            }
            default:
            {
                break;
            }
        }
    }
}

int read_hole_senser(char buffer[]) {       // current_thread() use      
    
    char buf = 0;
    while(1)
    {
        current_port->fc_read_buf(&buf); //read buf

        if(buf==0x7e || buf==0x2b || buf==0x2d)  // + - ~  //save data
        {    
            buffer[0] = buf;
            for(int i=1; i<=5; i++) 
            {
                current_port->fc_read_buf(&buf);
                if(buf==0x0d || buf==0x0a || buf==0x7e || buf==0x2b || buf==0x2d) //check +,-,~, \n,\r 
                {
                    return -1;
                }
                buffer[i] =  buf;
            }
            return 0;  //ok         
        }
        
    }
}
void current_thread() {
    mavlink_message_t message;
    rom::wiringnano pin50{50};
    rom::wiringnano pin14{14};
    rom::wiringnano pin194{194};
	
    pin194.pulllo(); //gpio config 0xx
    //pin14.pulllo();  //gpio config x0x

    char a_sensor[7] = {0};
    char b_sensor[7] = {0};
    char c_sensor[7] = {0};
    char error[7] = "error";

    int state = -1;

    //logggggggggggggggg
    //std::string date = currentDateTime();        //get time 
    //std::ofstream out1(("./log/Wired_Current_" + date + ".txt").c_str(), std::ios::app);  //make log file 

    while(1)
    {
        pin50.pulllo();     //000 gpio num1
        pin14.pulllo();
        //std::this_thread::sleep_for(std::chrono::milliseconds(50));
        state = read_hole_senser(a_sensor);
        if(state == -1) strcpy(a_sensor, error);
        
        pin50.pullhi();     //001 gpio num2
        //std::this_thread::sleep_for(std::chrono::milliseconds(50));
        state = read_hole_senser(b_sensor);
        if(state == -1) strcpy(b_sensor, error);

        pin14.pullhi();     //011 gpio num3
        //std::this_thread::sleep_for(std::chrono::milliseconds(50));
        state = read_hole_senser(c_sensor);
        if(state == -1) strcpy(c_sensor, error);
        
        //printf("a=%s  b=%s  c=%s\n", a_sensor, b_sensor, c_sensor);
        //mavlink_msg_wired_drone_current_pack_chan(1, 200, 2, &message, a_sensor, b_sensor, c_sensor);
        //kcmvp_port->write_message(message);
        
        //loggggggggggggggggggg
        //std::string adate = currentDateTime();
        //if (out1.is_open()) out1 << adate <<" "<<" A"<<a_sensor<<" B"<<b_sensor<<" C"<<c_sensor<<std::endl;
        //usleep(15000);
    }
}

int main(void)
{
    printf("start USIS system\n\n");
    sleep(5);

    // uart config kcmvp                    /////////////////////////tcp<--->kcmvp(serial)
    kcmvp_port = new Serial_Port("/dev/4", 57600);     
	int kcmvp = kcmvp_port->start();
    //kcmvp_port = new UDP_Port();
	//kcmvp_port->start();     

    // uart config FC
	fc_port = new Serial_Port("/dev/ttyTHS1", 57600);  //"/dev/ttyTHS1"
	int fc = fc_port->start();      
    
    // uart config ptz
    ptz_port = new Serial_Port("/dev/2", 115200);
    int ptz = ptz_port->start();  

    pz10t_port = new Serial_Port("/dev/1", 115200);
    int pz10t = pz10t_port->start();      

    // current sener init
    current_port = new Serial_Port("/dev/3", 9600);     
	int current = current_port->start();

    // Cell checker i2c address 0x48, 0x49, 0x4a, 0x4b
    //int a1_fd=-1, a2_fd=-1, b1_fd=-1, b2_fd=-1;
    //ads1115_i2c_init(&a1_fd, &a2_fd, &b1_fd, &b2_fd);
    //int ads1115 = write_1115_i2c_word_data(a1_fd, 0x01, 0x00); //test i2c connect
    
    fprintf(stderr, "\nkcmvp=%d  FC=%d  ptz=%d  pz10t=%d  current=%d\n", kcmvp, fc, ptz, pz10t, current);

    int pipe_fd[2];
    pipe(pipe_fd);      //pipe create
    
    int pid;  
    pid = fork();
    if(pid > 0) //fc ---> gcs  
    {
        /* if(kcmvp==0&fc==0&ptz==0&pz10t==-1&current==-1&ads1115==0)  
        {
            fprintf(stderr,"open_drone_tx\n");
            uint16_t pack_v = 0;

            //thread start
            std::thread th1(&cell_thread, &a1_fd, &a2_fd, &b1_fd, &b2_fd);
            std::thread th2(&fc_gcs_thread, &pack_v);
            std::thread th3(&bec_thread, pipe_fd, &pack_v);
            std::thread th4(&pack_thread);

            th1.join(); 
            th2.join();
            th3.join();
            th4.join();    
        }*/
        //else if(kcmvp==0&fc==0&ptz==0&pz10t==-1&current==0&ads1115==-1)
        //{
            fprintf(stderr,"open_w_drone_tx_20210107\n");
            uint16_t pack_v = 0;

            //thread start
            std::thread th2(&fc_gcs_thread, &pack_v);
            std::thread th3(&bec_thread, pipe_fd, &pack_v);
            std::thread th4(&pack_thread);
            std::thread th9(&current_thread);
            std::thread th5(&ptz_gcs_thread);
 
            th2.join();
            th3.join();
            th4.join();
            th9.join();
            th5.join();
        //}
        /* else if(kcmvp==0&fc==0&ptz==-1&pz10t==0&current==-1&ads1115==0)
        {
            fprintf(stderr,"open_box_drone_tx\n");
            uint16_t pack_v = 0;

            //thread start
            std::thread th1(&cell_thread, &a1_fd, &a2_fd, &b1_fd, &b2_fd);
            std::thread th2(&fc_gcs_thread, &pack_v);
            std::thread th3(&bec_thread, pipe_fd, &pack_v);
            std::thread th4(&pack_thread);
            
            th1.join(); 
            th2.join();
            th3.join();
            th4.join();
        }
        else 
        {
            fprintf(stderr,"fail USIS system_tx\n");
            return 0;
        }*/

        //uint16_t pack_v = 0;

        //thread start
        // std::thread th1(&cell_thread, &a1_fd, &a2_fd, &b1_fd, &b2_fd);
        // std::thread th2(&fc_gcs_thread, &pack_v);
        // std::thread th3(&bec_thread, pipe_fd, &pack_v);
        // std::thread th4(&pack_thread);
        // std::thread th5(&ptz_gcs_thread);
        // std::thread th9(&current_thread);

        // th1.join(); 
        // th2.join();
        // th3.join();
        // th4.join();
        // th5.join();
        // th9.join();
    }
    
    else if(pid == 0) //gcs ---> fc   //////////////////////////////////////////////////////////////////////////////////////////////////
    {
        /*if(kcmvp==0&fc==0&ptz==0&pz10t==-1&current==-1&ads1115==0)  
        {
            fprintf(stderr,"open_drone_rx\n");
            float cmd1 = 0.0;           //ptz controll
            float cmd2 = 0.0;
            int pm_cmd = 0;             //pm controll

            //thread start
            std::thread th6(&gcs_fc_thread, &cmd1, &cmd2);
            std::thread th7(&IO_port_thread, pipe_fd, &pm_cmd);
            std::thread th8(&ptz_thread, &cmd1, &cmd2, &pm_cmd);
            //std::thread th10(&pz10t_thread, &cmd1, &cmd2, &pm_cmd);

            th6.join();
            th7.join();
            th8.join();
            //th10.join();
        }*/
        //else if(kcmvp==0&fc==0&ptz==0&pz10t==-1&current==0&ads1115==-1)
        //{
            fprintf(stderr,"open_w_drone_rx_20210107\n");
            float cmd1 = 0.0;           //ptz controll
            float cmd2 = 0.0;
            int pm_cmd = 0;             //pm controll

            //thread start
            std::thread th6(&gcs_fc_thread, &cmd1, &cmd2);
            std::thread th11(&w_IO_port_thread, pipe_fd, &pm_cmd);     ////////////////// SSR active
            std::thread th8(&ptz_thread, &cmd1, &cmd2, &pm_cmd);
            //std::thread th10(&pz10t_thread, &cmd1, &cmd2, &pm_cmd);

            th6.join();
            th11.join();
            th8.join();
            //th10.join();
        //}
        /*else if(kcmvp==0&fc==0&ptz==-1&pz10t==0&current==-1&ads1115==0)
        {
            fprintf(stderr,"open_box_drone_rx\n");
            float cmd1 = 0.0;           //ptz controll
            float cmd2 = 0.0;
            int pm_cmd = 0;             //pm controll

            //thread start
            std::thread th6(&gcs_fc_thread, &cmd1, &cmd2);
            std::thread th7(&IO_port_thread, pipe_fd, &pm_cmd);
            //std::thread th8(&ptz_thread, &cmd1, &cmd2, &pm_cmd);
            std::thread th10(&pz10t_thread, &cmd1, &cmd2, &pm_cmd);

            th6.join();
            th7.join();
            //th8.join();
            th10.join();
        }
        else 
        {
            fprintf(stderr,"fail USIS system_rx\n");
            return 0;
        }*/
    
        ////int on_off = 0;
        //float cmd1 = 0.0;           //ptz controll
        //float cmd2 = 0.0;
        //int pm_cmd = 0;             //pm controll

        ////thread start
        //std::thread th6(&gcs_fc_thread, &cmd1, &cmd2);
        //std::thread th7(&IO_port_thread, pipe_fd, &pm_cmd);
        //std::thread th8(&ptz_thread, &cmd1, &cmd2, &pm_cmd);
        //std::thread th10(&pz10t_thread, &cmd1, &cmd2, &pm_cmd);
        //std::thread th11(&w_IO_port_thread, pipe_fd, &pm_cmd);     ////////////////// SSR active

        //th6.join();
        //th7.join();
        //th8.join();
        //th10.join();
        //th11.join();
    }
    else if(pid == -1)
    {
        printf("fork error\n");
        exit(0);
    }
    return 0;
}
 
