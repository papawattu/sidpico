#ifndef __SID_H_
#define __SID_H_

#include "pico/util/queue.h"

#define CS 	15
#define RES 17
#define CLK 16

#define DATA_OFFSET 9
#define ADDR_OFFSET 14

#define BASE_PIN 2

#define A0  2
#define A1  3
#define A2  4
#define A3  5
#define A4  6

#define D0  7
#define D1  8
#define D2  9
#define D3  10
#define D4  11
#define D5  12
#define D6  13
#define D7  14

#define QUEUE_SIZE 1024

queue_t command_queue;

#endif