nrf51-ble-gzll-device-uart
==========================
This example shows a way to run the Gazell stack in device mode concurrently with BLE, using the timeslot API in the BLE SoftDevice. 
It is based on the ble_app_uart example, and is extended to relay incoming UART packets over the Gazell stack to a listening Gazell host. 
A Gazell host UART application is also provided to act as the receiver, and this project will simply output incoming Gazell packets over the UART. 

Concurrent BLE and Gazell host is not implemented, it is assumed that the Gazell host will not require BLE connectivity. 

Requirements
------------
- nRF51 SDK version 10.0.0
- S110 SoftDevice version 8.0
- nRF51822 DK (PCA10028)

The project may need modifications to work with other versions or other boards. 

To compile it, clone the repository in the [SDK]/examples/ble_peripheral folder.

About this project
------------------
This application is one of several applications that has been built by the support team at Nordic Semiconductor, as a demo of some particular feature or use case. It has not necessarily been thoroughly tested, so there might be unknown issues. It is hence provided as-is, without any warranty. 

However, in the hope that it still may be useful also for others than the ones we initially wrote it for, we've chosen to distribute it here on GitHub. 

The application is built to be used with the official nRF51 SDK, that can be downloaded from developer.nordicsemi.com

Please post any questions about this project on https://devzone.nordicsemi.com.
