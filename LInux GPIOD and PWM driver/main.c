/**
 * @file main.c
 * @author Roshan Raj Kanakaiprath (rkanakai@asu.edu)
 * @brief ASU ID - 1222478062 
 * @version 0.1
 * @date 2022-03-08
 * 
 * @copyright Copyright (c) 2022
 * 
 */


#include <stdio.h>    // for printf
#include <fcntl.h>    // for open
#include <unistd.h>
#include <errno.h> 
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <gpiod.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include "project2.h"

#define PWM_EXPORT_PATH         "/sys/class/pwm/pwmchip0/export"
#define PWM_UNEXPORT_PATH       "/sys/class/pwm/pwmchip0/unexport"

//Define Macros for PWM0 path
#define PWM0_ENABLE_PATH        "/sys/class/pwm/pwmchip0/pwm0/enable"
#define PWM0_PERIOD_PATH        "/sys/class/pwm/pwmchip0/pwm0/period"
#define PWM0_DUTY_CYCLE_PATH    "/sys/class/pwm/pwmchip0/pwm0/duty_cycle"

//Define Macros for PWM1 path
#define PWM1_ENABLE_PATH        "/sys/class/pwm/pwmchip0/pwm1/enable"
#define PWM1_PERIOD_PATH        "/sys/class/pwm/pwmchip0/pwm1/period"
#define PWM1_DUTY_CYCLE_PATH    "/sys/class/pwm/pwmchip0/pwm1/duty_cycle"

#define CLOCK_FREQUENCY_PATH    "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_cur_freq"

//Define macros for GPIO pins
#define GREEN_LED           25
#define TRIGGER             22
#define ECHO                23
//Burst duration
#define TRIGGER_BURST_NS    10000

//LIMIT for UINT32
#define MAX32               4294967295
     

//Declare struct for chip and gpio lines
struct gpiod_chip *chip;
struct gpiod_line *lineG, *lineT, *lineE;

//Declare file descriptors to sysfs interface for PWM0 and PWM1
int fd_export_unexport, fd_enable_pwm0, fd_period_pwm0, fd_duty_cycle_pwm0, fd_enable_pwm1, fd_period_pwm1, fd_duty_cycle_pwm1;
char pwm1 ='1', pwm0 = '0', enable='1', disable='0'; // Character literals for pwm channel, enable and disable
char period_string[50], duty_cycle_red[50], duty_cycle_blue[50]; // define string for duty_cycles and period 
uint32_t duty_red, duty_green = PWM_PERIOD, duty_blue;// Variables to hold duty cycle for RGB led, Initialize duty_cycle to 100 for green
uint32_t period_ns = PWM_PERIOD * 1000000;// Period in nanoseconds
uint32_t loop_iter = 0, count =0, green_count=0;// variables for timer callback

float vel_inch_per_sec = 13042.8, sum_dist; 

//Timer Callback function to control the blinking of RGB Led
void timer_callback(int sig)
{
    if (sig == SIGUSR1)
    {
        count++;
        green_count++;
        if (count % STEP_DURATION == 0)
            loop_iter++;

        if (green_count == PWM_PERIOD)
        {
            green_count = 0;
        }

        if (loop_iter & 1)//Red Led
        {   
            //enable
            write(fd_enable_pwm0, &enable, 1);
        }
        else
        {
            //disable
            write(fd_enable_pwm0, &disable, 1);
        }

        if ((loop_iter >> 1) & 1)//Green Led
        {
            if (duty_green)
            {
                if (green_count == 0)
                {
                    gpiod_line_set_value(lineG, 1);
                }
                if (green_count == duty_green)
                {
                    gpiod_line_set_value(lineG, 0);
                }
            }
            else
            {
                gpiod_line_set_value(lineG, 0);
            }
        }
        else
        {
          gpiod_line_set_value(lineG, 0);  
        }

        if ((loop_iter >> 2) & 1)//Blue Led
        {   
            //enable
            write(fd_enable_pwm1, &enable, 1);
        }
        else
        {
            //disable
            write(fd_enable_pwm1, &disable, 1);
        }
    }
}

//Function to calculate elapsed time when mode is 0
void ts_sub(struct timespec *ts1, struct timespec *ts2, struct timespec *ts3)
{
    ts3->tv_sec = ts1->tv_sec - ts2->tv_sec;
    
    if(ts1->tv_nsec >= ts2->tv_nsec) {
		ts3->tv_nsec = ts1->tv_nsec - ts2->tv_nsec;	
	} else
	{	
		ts3->tv_sec--; 
		ts3->tv_nsec = 1e9 + ts1->tv_nsec - ts2->tv_nsec;	
    }
}

//Function to calculate elapsed time using cycle count register
static inline uint32_t ccnt_read (void)
{
  uint32_t cc = 0;
  __asm__ volatile ("mrc p15, 0, %0, c9, c13, 0":"=r" (cc));
  return cc;
}

//Function to read the clock frequency
static uint32_t get_clock_freq()
{
    int fd;
    char buf[20];
    fd = open(CLOCK_FREQUENCY_PATH, O_RDONLY);
    read(fd, &buf, sizeof(buf));
    //printf("%s\n", buf);
    close(fd);

    return (atoi(buf));
}
//Function to start Ultrasonic Sensor
void start_hcsr_04_sensor(int mode)
{   
    float dist;
    struct timespec next, c0, c1, c2;
    uint32_t t0, t1, freq;
    clock_gettime(CLOCK_MONOTONIC, &next);

    next.tv_sec += ((next.tv_nsec + TRIGGER_BURST_NS)/1000000000);
    next.tv_nsec = ((next.tv_nsec + TRIGGER_BURST_NS)%1000000000);

    //Set Trigger
    gpiod_line_set_value(lineT, 1);
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
    gpiod_line_set_value(lineT, 0);
    
    //Read Echo and calculate elapsed time based on mode
    if (mode == 0)
    {
        while (!(gpiod_line_get_value(lineE)))
        {
        }
        clock_gettime(CLOCK_MONOTONIC, &c0);

        while (gpiod_line_get_value(lineE))
        {
        }
        clock_gettime(CLOCK_MONOTONIC, &c1);
        ts_sub(&c1, &c0, &c2);
        //Calculate Distance
        dist = ((c2.tv_nsec + c2.tv_sec*1000000000)*(vel_inch_per_sec/1000000000))/2;
        sum_dist += dist;
        printf("Distance %0.2f inches\n", dist);
    }
    else 
    {
        while (!(gpiod_line_get_value(lineE)))
        {
        }
        t0 = ccnt_read();
        while (gpiod_line_get_value(lineE))
        {
        }
        t1 = ccnt_read();
        freq = get_clock_freq();
        //Calculate Distance
        if(t1>t0)
            dist = (vel_inch_per_sec*(t1-t0))/(2*freq*1000);
        else
            dist = (vel_inch_per_sec*(MAX32-t0 + t1))/(2*freq*1000);
        sum_dist += dist;
        printf("Distance %0.2f inches\n", dist);
    }
}

//Main Function
int main(void)
{
    timer_t t_id; //timer
    int ret, req, n_meas, mode;

    //Initially set all the duty_cycle to 100
    duty_red = period_ns; 
    duty_blue = period_ns;
    //String to capture user defined command
    char command[20];

    //Convert period and Initial Duty Cycle to string
    sprintf(period_string,"%d",period_ns);
    sprintf(duty_cycle_red,"%d",duty_red);
    sprintf(duty_cycle_blue,"%d",duty_blue);

    //Open Export path
    fd_export_unexport = open(PWM_EXPORT_PATH, O_WRONLY);
    if(fd_export_unexport == -1)
    {
        printf("error exporting pwm\n");
        return 1;
    }

    //export PWMs
    write(fd_export_unexport, &pwm0, 1);
    write(fd_export_unexport, &pwm1, 1);
    close(fd_export_unexport);

    //Open all the file descriptors for pwm channels 1 and 0

    //For PWM0
    fd_period_pwm0      = open(PWM0_PERIOD_PATH, O_WRONLY);
    fd_duty_cycle_pwm0  = open(PWM0_DUTY_CYCLE_PATH, O_WRONLY);
    fd_enable_pwm0      = open(PWM0_ENABLE_PATH, O_WRONLY);

    //For PWM1
    fd_period_pwm1      = open(PWM1_PERIOD_PATH, O_WRONLY);
    fd_duty_cycle_pwm1  = open(PWM1_DUTY_CYCLE_PATH, O_WRONLY);
    fd_enable_pwm1      = open(PWM1_ENABLE_PATH, O_WRONLY);

    //Disable both pwm channels
    write(fd_enable_pwm0, &disable, 1);
    write(fd_enable_pwm1, &disable, 1);       

    //Write initial period and duty cycles for both pwms   
    write(fd_period_pwm0, &period_string, strlen(period_string));
    write(fd_period_pwm1, &period_string, strlen(period_string));
    write(fd_duty_cycle_pwm0, &duty_cycle_red, strlen(duty_cycle_red));
    write(fd_duty_cycle_pwm1, &duty_cycle_blue, strlen(duty_cycle_blue));

    //Open GPIO1 chip and get GPIO line
    chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip)
        return -1;
    lineG = gpiod_chip_get_line(chip, GREEN_LED);
    lineT = gpiod_chip_get_line(chip, TRIGGER);
    lineE = gpiod_chip_get_line(chip, ECHO);

    if (!lineG || !lineT || !lineE) {
        gpiod_chip_close(chip);
        return -1;
    }
    req = gpiod_line_request_output(lineG, "gpio_state", GPIOD_LINE_ACTIVE_STATE_LOW);
    req = gpiod_line_request_output(lineT, "Trigger state", GPIOD_LINE_ACTIVE_STATE_LOW);
    req = gpiod_line_request_input(lineE, "Echo state");

    if (req) {
        gpiod_chip_close(chip);
        return -1;
    }

    //Define timer specs
    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR1;
    sev.sigev_value.sival_ptr = &t_id;
    signal(SIGUSR1, timer_callback);
    gpiod_line_set_value(lineG, 0);

    ret = timer_create(CLOCK_MONOTONIC, &sev, &t_id);
    if (ret == -1)
    {
        fprintf(stderr, "Error timer_create: %s\n", strerror(errno));
        return 1;
    }
    
    //long long freq_nanosecs = 1e9;
    struct itimerspec its;
    its.it_value.tv_sec =  0;
    its.it_value.tv_nsec = 1000000;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    timer_settime(t_id, TIMER_ABSTIME , &its, NULL);
    while(1)
    {
        //wait for command
        scanf("%s", command);

        if(!strcmp(command, "exit"))
            break;
        else if(!strcmp(command, "rgb"))//If rgb command is received
        {
            scanf("%d", &duty_red);
            scanf("%d", &duty_green);
            scanf("%d", &duty_blue);
            //Convert input to duty cycles
            duty_red = (period_ns * duty_red)/100;
            duty_green = (PWM_PERIOD * duty_green)/100;
            duty_blue = (period_ns * duty_blue)/100;
            //Convert duty cycle and write to file
            sprintf(duty_cycle_red,"%d",duty_red);
            sprintf(duty_cycle_blue,"%d",duty_blue);
            write(fd_duty_cycle_pwm0, &duty_cycle_red, strlen(duty_cycle_red));
            write(fd_duty_cycle_pwm1, &duty_cycle_blue, strlen(duty_cycle_blue));
        }
        else if(!strcmp(command, "dist"))
        {   

            scanf("%d", &n_meas);
            scanf("%d", &mode);
            sum_dist = 0;
            for(int i=0; i<n_meas;i++)
            {   
                start_hcsr_04_sensor(mode);
            }
            printf("Average Distance %0.2f\n", sum_dist/n_meas);
        }        
    }
    timer_delete(t_id);//Delete Timer

    //Disable pwm channels
    write(fd_enable_pwm0, &disable, 1);
    write(fd_enable_pwm1, &disable, 1);
    //Close all file Descriptors
    close(fd_enable_pwm0);
    close(fd_duty_cycle_pwm0);
    close(fd_period_pwm0);
    close(fd_enable_pwm1);
    close(fd_duty_cycle_pwm1);
    close(fd_period_pwm1);
    fd_export_unexport = open(PWM_UNEXPORT_PATH, O_WRONLY); // Unexport all pwm channels
    write(fd_export_unexport, &pwm0, 1);
    write(fd_export_unexport, &pwm1, 1);
    close(fd_export_unexport);
    //Close GPIOD chip
    gpiod_chip_close(chip);
    return 0;
}