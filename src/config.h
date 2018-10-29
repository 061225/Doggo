#ifndef CONFIG_H
#define CONFIG_H

// Define USE_XBEE to cause all debug prints to go through the xbee
// Comment out the line to debug over usb
// #define USE_XBEE


// Define DEBUG_LOW and/or DEBUG_HIGH to enable printing debug messages
// I've made two flags so you have more options in terms of which messages to show
//#define DEBUG_HIGH
//#define DEBUG_LOW
// #define PRINT_ONCE

// Setting this value above 0 will cause the imu thread to print debug messages
#define IMU_VERBOSE 1

// Set to 1 to enable data logging, set to 0 to disable
#define ENABLE_DATALOGGER 0
#define DATALOGGER_VERBOSE 0

// Important: Thread execution frequencies
#define POSITION_CONTROL_FREQ 100
#define DEBUG_PRINT_FREQ 20
#define UART_FREQ 2000
#define USB_SERIAL_FREQ 100
#define DATALOG_FREQ 10
#define IMU_FREQ 400
#define IMU_SEND_FREQ 100

#define CURRENT_LIM 40.0f

//------------------------------------------------------------------------------
// Accessories (not including ODRives)

#define SPI_CS_PIN 10
#define SPI_WAK_PIN 9
#define SPI_INTPIN 7
#define SPI_RSTPIN 8

//------------------------------------------------------------------------------
// IMU Parameters
// Set enable flag to 1 to use accelerometer and gyro, set to 0 to just use gyro
#define IMU_ENABLE_COMPLEMENTARY_FILTER 1
#define IMU_COMPLEMENTARY_FILTER_TAU 0.99f

//------------------------------------------------------------------------------
// Xbee serial
#ifdef USE_XBEE
#define Serial Serial5
#endif

#endif
