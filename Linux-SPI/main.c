/*
 * Assignment 4
 * Author:- Roshan Raj Kanakaiprath 
*/
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define SPI_DEVICE_PATH     "/dev/spidev0.0"
#define HCSR04_DEVICE_PATH     "/dev/hcsr04"
#define MAX7219_REG_NOOP 0x00
#define MAX7219_REG_DIGIT0 0x01
#define MAX7219_REG_DIGIT1 0x02
#define MAX7219_REG_DIGIT2 0x03
#define MAX7219_REG_DIGIT3 0x04
#define MAX7219_REG_DIGIT4 0x05
#define MAX7219_REG_DIGIT5 0x06
#define MAX7219_REG_DIGIT6 0x07
#define MAX7219_REG_DIGIT7 0x08
#define MAX7219_REG_DECODEMODE 0x09
#define MAX7219_REG_INTENSITY 0x0A
#define MAX7219_REG_SCANLIMIT 0x0B
#define MAX7219_REG_SHUTDOWN 0x0C
#define MAX7219_REG_DISPLAYTEST 0x0F
#define LENGTH 0x02
#define NUM_CHARCTERS 5U

//Define the ioctl code
#define SET_TRIGGER _IOW('a', 'a', int *)
#define SET_ECHO	_IOW('a','b', int *)

int TRIGGER_PIN = 22;
int ECHO_PIN = 23;

//thread ID for main thread and keyboard thread
pthread_t keyboard_thread_id, main_thread_id;
//file descriptor for spi device
int spi_fd, hcsr_fd;
static uint32_t mode; //SPI communication mode
static uint8_t bits = 8U; //Bits per word
static uint32_t speed = 10000000U; //Speed of SPI communication
static uint16_t delay = 1000U; //Delay in transmitting 1 SPI frame

//Array for intensity control
static uint8_t intensity_array[ ] = {0x00,0x02,0x04,0x06,0x07,0x09,0x0B,0x0D,0x0F};

//Flags to control execution
static volatile bool flag = true, blink_flag = false;
static uint8_t shutdown_data = 0x01;

//Variable to control scrolling speed
static int scroll_speed;

//structure for digit register data
struct max7219_data{
	uint8_t data[8];
};

//Initialize pattern for display
static struct max7219_data pattern[NUM_CHARCTERS] = {\
							{{0xF0,0x2C,0x23,0x23,0x2C,0xF0,0x00, 0x00}},\
							{{0x86,0x89,0x89,0x89,0x71,0x00,0x00, 0x00}},\
							{{0x7F,0x80,0x80,0x80,0x80,0x7F,0x00, 0x00}},\
							{{0x24,0xFF,0x24,0xFF,0x24,0x00,0x00, 0x00}},\
							{{0x84,0x82,0xFF,0x80,0x80,0x00,0x00, 0x00}}	\
							};

//Signal handler to detect Ctrl + C
static void sig_handler(int dummy)
{
	flag = false;
	pthread_cancel(keyboard_thread_id);
}

//Routine for SPI transfer
static int spi_transfer(uint8_t const reg, uint8_t const data)
{
	int ret;
	uint8_t tx[2];
	uint8_t rx[2];
	tx[0] = reg;
	tx[1] = data;
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = LENGTH,
		.delay_usecs = delay,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(spi_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1){
		printf("can't send spi message\n");
		return -1;
	}
	return 0;
}	

//Routine to initialize MAX7219
static int spi_dev_init()
{	
	int ret;
	ret = spi_transfer(MAX7219_REG_SHUTDOWN, 0x01);
	if(ret < 0){
		printf("cannot set shutdown register\n");
		return -1;
	}
	ret = spi_transfer(MAX7219_REG_DECODEMODE, 0x00);
	if(ret < 0){
		printf("cannot set decode register\n");
		return -1;
	}
	
	ret = spi_transfer(MAX7219_REG_INTENSITY, 0x0F);
	if(ret < 0){
		printf("cannot set intensity register\n");
		return -1;
	}
	ret = spi_transfer(MAX7219_REG_SCANLIMIT, 0x07);
	if(ret < 0){
		printf("cannot set scan limit register\n");
		return -1;
	}
	ret = spi_transfer(MAX7219_REG_DISPLAYTEST, 0x00);
	if(ret < 0){
		printf("cannot set display test register\n");
		return -1;
	}

	//Clear All Digit Register
	for (uint8_t i = 1; i <= 8; i++)
	{
		ret = spi_transfer(i, 0x00);
		if(ret < 0){
		printf("cannot set digit %d register\n", i-1);
		return -1;
		}	
	}				
	return 0;
}

//Function to write pattern to the digit registers
static int pattern_write(struct max7219_data pattern)
{	
	int ret;
	for (uint8_t i = 1; i<=8;i++)
	{
		ret = spi_transfer(i, pattern.data[i-1]);
		if(ret < 0)
			return ret;
	}
	return 0;
}

//Function to control intensity
static void set_max7219_intensity(uint8_t intensity)
{	
	int ret=0;

	if(intensity>=0 && intensity<=8)
		ret = spi_transfer(MAX7219_REG_INTENSITY, intensity_array[intensity]);
	else
		printf("Invalid argument for intensity\n");
	if(ret < 0)
		printf("Couldn't set Intensity\n");	
}

//Thread Routine for Keyboard Events 
void *keyboard_thread(void *arg)
{
	int old_type;
	char command[20];
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &old_type);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	while(1)
	{
		printf("Enter command and press ENTER\n");
		scanf("%s", command);

		if(!strcmp(command, "b"))
		{	
			//printf("%d length\n", strlen(command));
			blink_flag = true;
		}
		else if(strlen(command) == 1)
		{	
			uint8_t intensity;
			intensity = atoi(command);
			set_max7219_intensity(intensity);
		}
		else{
			printf("invalid command\n");
		}

	}
	pthread_exit(NULL);
}

//Function for scrolling
void max7219_scrolling()
{
	int ret;
	for(int idx = 0; idx<5; idx++)
	{
	ret = pattern_write(pattern[idx]);
	if(ret < 0)
	{
		printf("error in pattern write\n");
		return;
	}
	usleep(scroll_speed);		
	}

}

//Timer callback function
void timer_callback(int sig)
{
	int ret;
    if (sig == SIGUSR1)
    {
		if (blink_flag)
		{
			shutdown_data ^= 0x01;
		}
		ret = spi_transfer(MAX7219_REG_SHUTDOWN, shutdown_data);
		if(ret < 0)
			printf("cannot set shutdown register\n");		
	}
}

//Main Routine
int main(int argc, char *argv[])
{
    int ret = 0;
	timer_t t_id; //timer
	//Open spi device file
    spi_fd = open(SPI_DEVICE_PATH, O_RDWR);
    if(spi_fd < 0){
        printf("Cannot open device\n");
        return -1;
    }
	//open hcsr_04 device file
	hcsr_fd = open(HCSR04_DEVICE_PATH, O_RDWR);
    if(hcsr_fd < 0){
        printf("Cannot open device\n");
        return -1;
    }
	signal(SIGINT, sig_handler);
    // Set spi mode
	ret = ioctl(spi_fd, SPI_IOC_WR_MODE32, &mode);
	if (ret == -1){
		printf("can't set spi mode\n");return -1;}

	ret = ioctl(spi_fd, SPI_IOC_RD_MODE32, &mode);
	if (ret == -1){
		printf("can't get spi mode\n");return -1;}

   // Set bits per word
	ret = ioctl(spi_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1){
		printf("can't set bits per word\n");return -1;}

	ret = ioctl(spi_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1){
		printf("can't get bits per word\n");return -1;}

   // Set spi clk speed
	ret = ioctl(spi_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1){
		printf("can't set max speed hz\n");return -1;}

	ret = ioctl(spi_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1){
		printf("can't get max speed hz\n");return -1;}

	//Initialize MAX7219
    ret = spi_dev_init();
	if(ret < 0){
		printf("can't initialize MAX7219\n");return -1;
	}

	//Set Trigger and Echo pins
	ret = ioctl(hcsr_fd, SET_ECHO, &ECHO_PIN);
    if(ret<0)
    {
        printf("Could set echo\n");
        return -1;
    }
    ioctl(hcsr_fd, SET_TRIGGER, &TRIGGER_PIN);
    if(ret<0)
    {
        printf("Could set echo\n");
        return -1;
    }
	//Initiate initial measurement
	write(hcsr_fd, NULL, 0);
	usleep(500000);
	read(hcsr_fd, &scroll_speed, sizeof(scroll_speed));
	
	//Define timer specs
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_ptr = &t_id;
    signal(SIGUSR1, timer_callback);
	ret = timer_create(CLOCK_MONOTONIC, &sev, &t_id);
    if (ret == -1)
    {
        fprintf(stderr, "Error timer_create: %s\n", strerror(errno));
        return 1;
    }
    
	struct itimerspec its;
    its.it_value.tv_sec =  0;
    its.it_value.tv_nsec = 500000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

	pthread_attr_t thread_attr;
	struct sched_param param;
    main_thread_id = pthread_self();
    param.sched_priority = 90;
    pthread_setschedparam(main_thread_id, SCHED_FIFO, &param);
	pthread_attr_init(&thread_attr);
	pthread_attr_setschedpolicy(&thread_attr, SCHED_FIFO);
	pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
	param.sched_priority = 95;
    pthread_attr_setschedparam(&thread_attr, &param);
    pthread_create(&keyboard_thread_id, &thread_attr, keyboard_thread, NULL);
    
	timer_settime(t_id, TIMER_ABSTIME , &its, NULL);
	while(flag)
	{
		max7219_scrolling();
		write(hcsr_fd, NULL, 0);
		read(hcsr_fd, &scroll_speed, sizeof(scroll_speed));

	}
	pthread_join(keyboard_thread_id, NULL);
	printf("Keyboard thread exited\n");
	pthread_attr_destroy(&thread_attr);
	for (uint8_t i = 1; i <= 8; i++)
	{
		ret = spi_transfer(i, 0x00);
		if(ret < 0){
		printf("cannot set digit %d register\n", i-1);
		return -1;
		}	
	}	
	ret = spi_transfer(MAX7219_REG_SHUTDOWN, 0x00);
	//sleep(3);
	if(ret < 0){
		printf("cannot set shutdown register\n");
		return -1;
	}
    printf("closing all devices\n");
    close(spi_fd);
	close(hcsr_fd);
    return 0;
}//End of main