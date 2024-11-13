/* This file implements a simple GPIO application example. */

/* This example illustrates the use of the FX3 firmware APIs to implement
 * a simple GPIO application example.
 *
 * The example illustrates the usage of simple GPIO API to set and get 
 * the status of the pin and the usage of GPIO interrupts.
 *
 * The example uses GPIO 54 as output. It toggles this pin ON and OFF
 * at an interval of 250ms.
 *
 * GPIO 45 is used as the input GPIO. Interrupts are enabled and a 
 * callback is registered for the GPIO edge interrupts both positive 
 * and negative edges.
 *
 */

#include <cyu3system.h>
#include <cyu3os.h>
#include <cyu3error.h>
#include <cyu3gpio.h>
#include <cyu3uart.h>
#include <stdint.h>
#include <stddef.h>

#define CY_FX_GPIOAPP_THREAD_STACK       (0x0400)   /* GPIO application thread stack size */
#define CY_FX_GPIOAPP_THREAD_PRIORITY    (8)        /* GPIO application thread priority */

#define CY_FX_GPIOAPP_GPIO_HIGH_EVENT    (1 << 0)   /* GPIO high event */
#define CY_FX_GPIOAPP_GPIO_LOW_EVENT     (1 << 1)   /* GPIO low event */

#define CY_FX_LED_GPIO 				 	 (54)
#define CY_FX_BTN_GPIO				     (45)
#define CY_LED_BLINK_RATE				 (500) 	    /* 500 ms */

#define CY_FX_DEBUG_PRIORITY			 (4)		/* Sets the debug print priority level */

CyU3PThread gpioOutputThread;   /* GPIO thread structure */
CyU3PThread gpioInputThread;    /* GPIO thread structure */
CyU3PEvent glFxGpioAppEvent;    /* GPIO input event group. */

/* Application error handler. */
void
CyFxAppErrorHandler (
        CyU3PReturnStatus_t apiRetStatus    /* API return status */
        )
{
    /* Application failed with the error code apiRetStatus */

    /* Add custom debug or recovery actions here */

    /* Loop indefinitely */
    for (;;)
    {
        /* Thread sleep : 100 ms */
        CyU3PThreadSleep (100);
    }
}

/* Initialize the debug module with UART. */
CyU3PReturnStatus_t
CyFxDebugInit (
        void)
{
    CyU3PUartConfig_t uartConfig;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize and configure the UART for logging. */
    status = CyU3PUartInit ();
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    CyU3PMemSet ((uint8_t *)&uartConfig, 0, sizeof (uartConfig));
    uartConfig.baudRate = CY_U3P_UART_BAUDRATE_115200;
    uartConfig.stopBit  = CY_U3P_UART_ONE_STOP_BIT;
    uartConfig.parity   = CY_U3P_UART_NO_PARITY;
    uartConfig.txEnable = CyTrue;
    uartConfig.rxEnable = CyFalse;
    uartConfig.flowCtrl = CyFalse;
    uartConfig.isDma    = CyTrue;
    status = CyU3PUartSetConfig (&uartConfig, NULL);
    if (status != CY_U3P_SUCCESS)
    {
        return status;
    }

    /* Set the UART transfer to a really large value. */
    status = CyU3PUartTxSetBlockXfer (0xFFFFFFFF);
    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(status);
    }

     /* Initialize the debug module. */
    status = CyU3PDebugInit (CY_U3P_LPP_SOCKET_UART_CONS, 8);
    if (status != CY_U3P_SUCCESS)
    {
        CyFxAppErrorHandler(status);
    }

    CyU3PDebugPreamble(CyFalse);

    return status;
}

/* GPIO interrupt callback handler. This is received from
 * the interrupt context. So DebugPrint API is not available
 * from here. Set an event in the event group so that the
 * GPIO thread can print the event information. */
void CyFxGpioIntrCb (
        uint8_t gpioId /* Indicates the pin that triggered the interrupt */
        )
{
    CyBool_t gpioValue = CyFalse;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Get the status of the pin */
    apiRetStatus = CyU3PGpioGetValue (gpioId, &gpioValue);
    if (apiRetStatus == CY_U3P_SUCCESS)
    {
        /* Check status of the pin */
        if (gpioValue == CyTrue)
        {
            /* Set GPIO high event */
            CyU3PEventSet(&glFxGpioAppEvent, CY_FX_GPIOAPP_GPIO_HIGH_EVENT,
                    CYU3P_EVENT_OR);
        }
        else
        {
            /* Set GPIO low Event */
            CyU3PEventSet(&glFxGpioAppEvent, CY_FX_GPIOAPP_GPIO_LOW_EVENT,
                    CYU3P_EVENT_OR);
        }
    }
}

void
CyFxGpioInit (void)
{
    CyU3PGpioClock_t gpioClock;
    CyU3PGpioSimpleConfig_t gpioConfig;
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Init the GPIO module */
    gpioClock.fastClkDiv = 2;
    gpioClock.slowClkDiv = 0;
    gpioClock.simpleDiv = CY_U3P_GPIO_SIMPLE_DIV_BY_2;
    gpioClock.clkSrc = CY_U3P_SYS_CLK;
    gpioClock.halfDiv = 0;

    apiRetStatus = CyU3PGpioInit(&gpioClock, CyFxGpioIntrCb);
    if (apiRetStatus != 0)
    {
        /* Error Handling */
        CyU3PDebugPrint (CY_FX_DEBUG_PRIORITY, "CyU3PGpioInit failed, error code = %d\n", apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Configure GPIO 45 [Button] as input with interrupt enabled for both edges */
    gpioConfig.outValue = CyTrue;
    gpioConfig.inputEn = CyTrue;
    gpioConfig.driveLowEn = CyFalse;
    gpioConfig.driveHighEn = CyFalse;
    gpioConfig.intrMode = CY_U3P_GPIO_INTR_BOTH_EDGE;
    apiRetStatus = CyU3PGpioSetSimpleConfig(CY_FX_BTN_GPIO, &gpioConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyU3PDebugPrint (CY_FX_DEBUG_PRIORITY, "CyU3PGpioSetSimpleConfig failed, error code = %d\n",
                apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Override GPIO 54 [LED] as output pin */
    apiRetStatus = CyU3PDeviceGpioOverride (CY_FX_LED_GPIO, CyTrue);
    if (apiRetStatus != 0)
    {
        /* Error Handling */
        CyU3PDebugPrint (CY_FX_DEBUG_PRIORITY, "CyU3PDeviceGpioOverride failed, error code = %d\n",
                apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Configure GPIO 54 as output */
    gpioConfig.outValue = CyFalse;
    gpioConfig.driveLowEn = CyTrue;
    gpioConfig.driveHighEn = CyTrue;
    gpioConfig.inputEn = CyFalse;
    gpioConfig.intrMode = CY_U3P_GPIO_NO_INTR;
    apiRetStatus = CyU3PGpioSetSimpleConfig(CY_FX_LED_GPIO, &gpioConfig);
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyU3PDebugPrint (CY_FX_DEBUG_PRIORITY, "CyU3PGpioSetSimpleConfig failed, error code = %d\n",
                apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }
}

/* Entry function for the gpioOutputThread */
void
GpioOutputThread_Entry (
        uint32_t input)
{
    CyU3PReturnStatus_t apiRetStatus = CY_U3P_SUCCESS;

    /* Initialize Debug module */
    apiRetStatus = CyFxDebugInit();
    if (apiRetStatus != CY_U3P_SUCCESS)
    {
        /* Error handling */
        CyU3PDebugPrint (CY_FX_DEBUG_PRIORITY, "Debug module initialization failed, error code = %d\n",
                apiRetStatus);
        CyFxAppErrorHandler(apiRetStatus);
    }

    /* Initialize GPIO module. */
    CyFxGpioInit ();

    for (;;)
    {
        /* Set the GPIO 54 [LED] to high */
        apiRetStatus = CyU3PGpioSetValue (CY_FX_LED_GPIO, CyTrue);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            /* Error handling */
            CyU3PDebugPrint (CY_FX_DEBUG_PRIORITY, "CyU3PGpioSetValue failed, error code = %d\n",
                    apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }

        /* Wait for two seconds */
        CyU3PThreadSleep(CY_LED_BLINK_RATE);

        /* Set the GPIO 21 to low */
        apiRetStatus = CyU3PGpioSetValue (CY_FX_LED_GPIO, CyFalse);
        if (apiRetStatus != CY_U3P_SUCCESS)
        {
            /* Error handling */
            CyU3PDebugPrint (CY_FX_DEBUG_PRIORITY, "CyU3PGpioSetValue failed, error code = %d\n",
                    apiRetStatus);
            CyFxAppErrorHandler(apiRetStatus);
        }

        /* Wait for two seconds */
        CyU3PThreadSleep(CY_LED_BLINK_RATE);
    }
}

/* Entry function for the gpioInputThread */
void
GpioInputThread_Entry (
        uint32_t input)
{
    uint32_t eventFlag;
    CyU3PReturnStatus_t txApiRetStatus = CY_U3P_SUCCESS;

    for (;;)
    {
        /* Wait for a GPIO event */
        txApiRetStatus = CyU3PEventGet (&glFxGpioAppEvent,
                (CY_FX_GPIOAPP_GPIO_HIGH_EVENT | CY_FX_GPIOAPP_GPIO_LOW_EVENT),
                CYU3P_EVENT_OR_CLEAR, &eventFlag, CYU3P_WAIT_FOREVER);
        if (txApiRetStatus == CY_U3P_SUCCESS)
        {
            if (eventFlag & CY_FX_GPIOAPP_GPIO_HIGH_EVENT)
            {
                /* Print the status of the pin */
                CyU3PDebugPrint (CY_FX_DEBUG_PRIORITY, "GPIO 45 [Button] is set to high\r\n");
            }
            else
            {
                /* Print the status of the pin */
                CyU3PDebugPrint (CY_FX_DEBUG_PRIORITY, "GPIO 45 [Button] is set to low\r\n");
            }
        }
    }
}

/* Application define function which creates the threads. */
void
CyFxApplicationDefine (
        void)
{
    void *ptr = NULL;
    uint32_t retThrdCreate = CY_U3P_SUCCESS;

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_GPIOAPP_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&gpioOutputThread,        /* GPIO Example App Thread structure */
                          "21:GPIO_simple_output",               /* Thread ID and Thread name */
                          GpioOutputThread_Entry,                /* GPIO Example App Thread Entry function */
                          0,                                     /* No input parameter to thread */
                          ptr,                                   /* Pointer to the allocated thread stack */
                          CY_FX_GPIOAPP_THREAD_STACK,            /* Thread stack size */
                          CY_FX_GPIOAPP_THREAD_PRIORITY,         /* Thread priority */
                          CY_FX_GPIOAPP_THREAD_PRIORITY,         /* Pre-emption threshold for the thread. */
                          CYU3P_NO_TIME_SLICE,                   /* No time slice for the application thread */
                          CYU3P_AUTO_START                       /* Start the Thread immediately */
                          );

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Thread creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }

    /* Allocate the memory for the threads */
    ptr = CyU3PMemAlloc (CY_FX_GPIOAPP_THREAD_STACK);

    /* Create the thread for the application */
    retThrdCreate = CyU3PThreadCreate (&gpioInputThread,          /* GPIO Example App Thread structure */
                          "22:GPIO_simple_input",                 /* Thread ID and Thread name */
                          GpioInputThread_Entry,                  /* GPIO Example App Thread entry function */
                          0,                                      /* No input parameter to thread */
                          ptr,                                    /* Pointer to the allocated thread stack */
                          CY_FX_GPIOAPP_THREAD_STACK,             /* Thread stack size */
                          CY_FX_GPIOAPP_THREAD_PRIORITY,          /* Thread priority */
                          CY_FX_GPIOAPP_THREAD_PRIORITY,          /* Pre-emption threshold for the thread */
                          CYU3P_NO_TIME_SLICE,                    /* No time slice for the application thread */
                          CYU3P_AUTO_START                        /* Start the Thread immediately */
                          );

    /* Check the return code */
    if (retThrdCreate != 0)
    {
        /* Thread creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }

    /* Create GPIO application event group */
    retThrdCreate = CyU3PEventCreate(&glFxGpioAppEvent);
    if (retThrdCreate != 0)
    {
        /* Event group creation failed with the error code retThrdCreate */

        /* Add custom recovery or debug actions here */

        /* Application cannot continue */
        /* Loop indefinitely */
        while(1);
    }
}

/*
 * Main function
 */
int
main (void)
{
    CyU3PIoMatrixConfig_t io_cfg;
    CyU3PReturnStatus_t status = CY_U3P_SUCCESS;

    /* Initialize the device */
    status = CyU3PDeviceInit (0);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Initialize the caches. Enable both Instruction and Data Caches. */
    status = CyU3PDeviceCacheControl (CyTrue, CyTrue, CyTrue);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* Configure the IO matrix for the device. On the FX3 DVK board,
     * the COM port is connected to the IO(53:56). This means that
     * either DQ32 mode should be selected or lppMode should be set
     * to UART_ONLY. Here we are choosing UART_ONLY configuration. */
    CyU3PMemSet ((uint8_t *)&io_cfg, 0, sizeof(io_cfg));
    io_cfg.isDQ32Bit = CyFalse;
    io_cfg.s0Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.s1Mode = CY_U3P_SPORT_INACTIVE;
    io_cfg.useUart   = CyTrue;
    io_cfg.useI2C    = CyFalse;
    io_cfg.useI2S    = CyFalse;
    io_cfg.useSpi    = CyFalse;
    io_cfg.lppMode   = CY_U3P_IO_MATRIX_LPP_UART_ONLY;
    /* GPIO 45 is used as input pin. GPIO 54 is also used but cannot
     * be selected here as it is part of the GPIF IOs (CTL4). Since
     * this IO is not used, it can be overridden to become a GPIO by
     * invoking the CyU3PDeviceGpioOverride call. */
    io_cfg.gpioSimpleEn[0]  = 0;
    io_cfg.gpioSimpleEn[1]  = 0x00002000; /* GPIO 45 */
    io_cfg.gpioComplexEn[0] = 0;
    io_cfg.gpioComplexEn[1] = 0;
    status = CyU3PDeviceConfigureIOMatrix (&io_cfg);
    if (status != CY_U3P_SUCCESS)
    {
        goto handle_fatal_error;
    }

    /* This is a non returnable call for initializing the RTOS kernel */
    CyU3PKernelEntry ();

    /* Dummy return to make the compiler happy */
    return 0;

handle_fatal_error:
    /* Cannot recover from this error. */
    while (1);
}

/* [ ] */

