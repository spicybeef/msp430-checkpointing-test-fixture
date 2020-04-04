
//-----------------------------------------------------------------------------
// Includes

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <file.h>
#include "driverlib.h"
#include "uartlib.h"

//-----------------------------------------------------------------------------
// Defines

//-----------------------------------------------------------------------------
// Globals

// stdio buffers
#define IO_BUFF_SIZE (128)
char stdinBuff[IO_BUFF_SIZE];
char stdoutBuff[IO_BUFF_SIZE];

#define AES_ENCRYPTION_DATA_SIZE (16) // Size of data to be encrypted/decrypted (must be multiple of 16)
#pragma PERSISTENT(cipherKey)
uint8_t cipherKey[32] =
{
    0xDE, 0xAD, 0xBE, 0xEF,
    0xBA, 0xDC, 0x0F, 0xEE,
    0xFE, 0xED, 0xBE, 0xEF,
    0xBE, 0xEF, 0xBA, 0xBE,
    0xBA, 0xDF, 0x00, 0x0D,
    0xFE, 0xED, 0xC0, 0xDE,
    0xD0, 0xD0, 0xCA, 0xCA,
    0xCA, 0xFE, 0xBA, 0xBE,
};
uint8_t dataAESencrypted[AES_ENCRYPTION_DATA_SIZE]; // Encrypted data
//uint8_t dataAESdecrypted[AES_ENCRYPTION_DATA_SIZE]; // Decrypted data, not used right now
char message[AES_ENCRYPTION_DATA_SIZE] = {0};

// Our power loss flag (raised by the GPIO interrupt)
volatile bool powerLoss;
// Our active work flag (raised while we're busy doing work)
volatile bool currentlyWorking;
// Our current chunk size
volatile uint16_t currentChunkSize;
// Total bytes processed by our workload
volatile uint32_t bytesProcessed;

// Our total workload size (3MB will yield about 30s of work)
#define TOTAL_WORKLOAD_SIZE_BYTES (3145728)

//-----------------------------------------------------------------------------
// Function prototypes

// Initialization
void Init_GPIO(void);
void Init_Clock(void);
bool Init_UART(void);
void Init_RTC(void);
void Init_Timer(void);
void Init_AES(uint8_t * cypherKey);

// Checkpointing fixture
void executeWorkloadPolicy(void);
void markWorkEnd(void);
void markWorkStart(void);

//-----------------------------------------------------------------------------
// Functions

/**
 * @brief      System pre-init, run before main()
 *
 * @return     If segment (BSS) initialization should be performed or not.
 *             Return: 0 to omit initialization 1 to run initialization
 */
int _system_pre_init(void)
{
    // Stop Watchdog timer
    WDT_A_hold(__MSP430_BASEADDRESS_WDT_A__);     // Stop WDT

    // Disable global interrupts
    __disable_interrupt();

    return 1;
}

/**
 * @brief      Mark that work has started
 */
void markWorkStart(void)
{
    GPIO_setOutputHighOnPin(GPIO_PORT_P8, GPIO_PIN1);
    currentlyWorking = true;
}

/**
 * @brief      Mark that work has ended
 */
void markWorkEnd(void)
{
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN1);
    currentlyWorking = false;
}

/**
 * @brief      Perform our work.
 */
void doAes(void)
{
    uint16_t i;
    // Copy the string we want to encrypt to our buffer
    const char stringToEncrypt[] = "I am a meat popsicle.           ";
    memcpy(message, stringToEncrypt, sizeof(stringToEncrypt));

    // Do stuff
    // Signal that work is starting
    markWorkStart();
    for (i = 0; i < currentChunkSize; i += 16)
    {
        // Encrypt data with preloaded cipher key. For this fixture, we will be
        // performing work on the same message (no real work is being done, just
        // counting how many successful chunks we've accomplished.
        AES256_encryptData(AES256_BASE, (uint8_t*)(message), dataAESencrypted);
        // Check if we need to abort our current chunk
        if (powerLoss)
        {
            // If we raised a power-loss flag, it means that at some point during our
            // current chunk we encountered a power-loss. This chunk is no
            // longer valid. Break out of loop.
            break;
        }
    }
    // Signal that work has halted
    markWorkEnd();
    if (!powerLoss)
    {
        // If we're here, the chunk successfully executed! Add to our total
        // bytes processed accumulator.
        bytesProcessed += currentChunkSize;
    }
    else
    {
        // Chunk failed. We won't count this as work performed and we need to
        // modify our behaviour.
        executeWorkloadPolicy();
    }

}



/**
 * @brief      This executes the policy that we're current employing to scale
 *             our workloads. This will modify the current chunk size.
 */
void executeWorkloadPolicy()
{
    // No policy yet!
}

void main(void)
{
    uint16_t startTicks;
    uint16_t currentTicks;
    uint16_t i;
    bool success = true;

    // Reset our runtime variables
    powerLoss = false;
    currentlyWorking = false;
    currentChunkSize = 1024;
    bytesProcessed = 0;

    // Peripheral initialization
    Init_GPIO();
    Init_Clock();
    Init_Timer();
    Init_AES(cipherKey);
    success = Init_UART();

    if (!success)
    {
        // Turn on red LED for failure
        GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN0);
        for (;;)
        {
            __no_operation();
        }
    }

    /* Add the UART device to the system. */
    add_device("UART", _MSA, UartLib_DeviceOpen,
               UartLib_DeviceClose, UartLib_DeviceRead,
               UartLib_DeviceWrite, UartLib_DeviceLSeek,
               UartLib_DeviceUnlink, UartLib_DeviceRename);

    /* Open UART0 for writing to stdout and set buffer */
    freopen("UART:0", "w", stdout);
    setvbuf(stdout, stdinBuff, _IOLBF, IO_BUFF_SIZE);

    /* Open UART0 for reading from stdin and set buffer */
    freopen("UART:0", "r", stdin);
    setvbuf(stdin, stdoutBuff, _IOLBF, IO_BUFF_SIZE);

    // Enable global interrupts
    __enable_interrupt();

    char c;

    printf(" ▄████▄    █████▒\r\n");
    printf("▒██▀ ▀█  ▓██   ▒ \r\n");
    printf("▒▓█    ▄ ▒████ ░ \r\n");
    printf("▒▓▓▄ ▄██▒░▓█▒  ░ \r\n");
    printf("▒ ▓███▀ ░░▒█░    \r\n");
    printf("░ ░▒ ▒  ░ ▒ ░    \r\n");
    printf("  ░  ▒    ░      \r\n");
    printf("░         ░ ░    \r\n");
    printf("░ ░              \r\n");
    printf("░                \r\n");
    printf("\r\n");
    printf("?>");
    scanf("%c", &c);
    printf("\r\nYou entered %c\r\n", c);
    
    // Main loop
    for (;;)
    {
        // Perform our AES workload
        doAes();

        // Wait 1ms, simulates work that needs to be performed in between our
        // workloads.
        startTicks = Timer_A_getCounterValue(TIMER_A0_BASE);
        do
        {
            // If we encounter a power-loss here, that's ok!, But reset the
            // timer. This will also reset the power-loss flag experienced
            // during our workload.
            if (powerLoss)
            {
                powerLoss = false;
                startTicks = Timer_A_getCounterValue(TIMER_A0_BASE);
            }
            currentTicks = Timer_A_getCounterValue(TIMER_A0_BASE);
        }
        while ((currentTicks - startTicks) < 1000);

        if (bytesProcessed >= TOTAL_WORKLOAD_SIZE_BYTES)
        {
            // We're done! Leave the workload loop
            break;
        }
    }

    // Turn on green LED for completion
    GPIO_setOutputHighOnPin(GPIO_PORT_P1, GPIO_PIN1);

    for (;;)
    {
        // Not just diamonds last forever...
        __no_operation();
    }
}

/*
 * GPIO Initialization
 */
void Init_GPIO()
{
    // Set all GPIO pins to output low for low power
    GPIO_setOutputLowOnPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P4, GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setOutputLowOnPin(GPIO_PORT_PJ, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7|GPIO_PIN8|GPIO_PIN9|GPIO_PIN10|GPIO_PIN11|GPIO_PIN12|GPIO_PIN13|GPIO_PIN14|GPIO_PIN15);
    GPIO_setOutputHighOnPin(GPIO_PORT_P4, GPIO_PIN0); // SD CS

    GPIO_setAsOutputPin(GPIO_PORT_P1, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P3, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P4, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P6, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P7, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_P8, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7);
    GPIO_setAsOutputPin(GPIO_PORT_PJ, GPIO_PIN0|GPIO_PIN1|GPIO_PIN2|GPIO_PIN3|GPIO_PIN4|GPIO_PIN5|GPIO_PIN6|GPIO_PIN7|GPIO_PIN8|GPIO_PIN9|GPIO_PIN10|GPIO_PIN11|GPIO_PIN12|GPIO_PIN13|GPIO_PIN14|GPIO_PIN15);

    // Configure P2.0 - UCA0TXD and P2.1 - UCA0RXD
    GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN0, GPIO_SECONDARY_MODULE_FUNCTION);
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P2, GPIO_PIN1, GPIO_SECONDARY_MODULE_FUNCTION);

    // Set PJ.4 and PJ.5 as Primary Module Function Input, LFXT.
    GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_PJ, GPIO_PIN4 + GPIO_PIN5, GPIO_PRIMARY_MODULE_FUNCTION);

    // P8.1 is out timing pin, active high
    GPIO_setOutputLowOnPin(GPIO_PORT_P8, GPIO_PIN1);

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PMM_unlockLPM5();
}

/*
 * Clock System Initialization
 */
void Init_Clock()
{
    // Set DCO frequency to 16 MHz
    CS_setDCOFreq(CS_DCORSEL_1, CS_DCOFSEL_4);
    // Set external clock frequency to 32.768 KHz
    CS_setExternalClockSource(32768, 0);
    // Set ACLK=LFXT
    CS_initClockSignal(CS_ACLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);
    // Set SMCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    // Set MCLK = DCO with frequency divider of 1
    CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    //Start XT1 with no time out
    CS_turnOnLFXT(CS_LFXT_DRIVE_3);
}

/**
 * Timer initialization
 * @note       This will setup timer A to have a 1us tick
 */
void Init_Timer(void)
{
    // Start timer
    Timer_A_initUpModeParam param = {0};
    param.clockSource = TIMER_A_CLOCKSOURCE_SMCLK; // Use SMCLK (= DCO = (16 MHz / 16) = 1 MHz) 
    param.clockSourceDivider = TIMER_A_CLOCKSOURCE_DIVIDER_16;
    param.timerPeriod = 0xFFFF; // Use entire 16 bit counter
    param.timerInterruptEnable_TAIE = TIMER_A_TAIE_INTERRUPT_DISABLE;
    param.captureCompareInterruptEnable_CCR0_CCIE = TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE;
    param.timerClear = TIMER_A_DO_CLEAR;
    param.startTimer = true;
    Timer_A_initUpMode(TIMER_A0_BASE, &param);

    __delay_cycles(10000); // Delay wait for clock to settle
}

/*
 * UART Communication Initialization
 */
bool Init_UART()
{
    // Configure UART
    EUSCI_A_UART_initParam param = {0};
    param.selectClockSource = EUSCI_A_UART_CLOCKSOURCE_SMCLK;
    // BRCLK = SMCLK = 16 MHz
    // Values obtained from Table 30-5 in MSP430FR59xx User's Guide (SLAU367O)
    // Baud rate: 230400
    // UCOS16 = 1, UCBRx = 4, UCBRFx = 5, UCBRSx = 0x55
    // Baud rate: 115200
    // UCOS16 = 1, UCBRx = 8, UCBRFx = 10, UCBRSx = 0xF7
    param.clockPrescalar = 8;   // UCBRx
    param.firstModReg = 10;     // UCBRFx
    param.secondModReg = 0xF7;  // UCBRSx
    param.parity = EUSCI_A_UART_NO_PARITY;
    param.msborLsbFirst = EUSCI_A_UART_LSB_FIRST;
    param.numberofStopBits = EUSCI_A_UART_ONE_STOP_BIT;
    param.uartMode = EUSCI_A_UART_MODE;
    param.overSampling = EUSCI_A_UART_OVERSAMPLING_BAUDRATE_GENERATION; // UCOS16

    if(STATUS_FAIL == EUSCI_A_UART_init(EUSCI_A0_BASE, &param))
    {
        return false;
    }

    EUSCI_A_UART_enable(EUSCI_A0_BASE);

    // No interrupts
    // EUSCI_A_UART_clearInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);
    // Enable USCI_A0 RX interrupt
    // EUSCI_A_UART_enableInterrupt(EUSCI_A0_BASE, EUSCI_A_UART_RECEIVE_INTERRUPT);

    return true;
}

/**
 * @brief      Setup the AES peripheral
 *
 * @param      cypherKey  The 32 byte cypher key to use for AES encryption/decrytion
 */
void Init_AES(uint8_t * cypherKey)
{
    // Load a cipher key to module
    AES256_setCipherKey(AES256_BASE, cypherKey, AES256_KEYLENGTH_256BIT);
}

//-----------------------------------------------------------------------------
// ISRs

/*
 * PORT1_VECTOR Interrupt Vector handler
 *
 */
#pragma vector=PORT1_VECTOR
__interrupt void PORT1_ISR(void)
{
    //
}
