
#ifndef _HOSTLINK_H_
#define _HOSTLINK_H_

#define _GLIBCXX_USE_C99

#include <Arduino.h>
#ifdef ESP8266
#include <SoftwareSerial.h>
#else
#include <HardwareSerial.h>
#endif

#ifndef BUFFER_LENGTH
#define BUFFER_LENGTH 32
#endif

typedef enum
{
    NORMAL_COMPLETION = 0x00,
    NOT_EXECUTABLE_IN_RUN_MODE = 0x01,
    NOT_EXECUTABLE_IN_MONITOR_MODE = 0x02,
    UM_WRITE_PROTECTED = 0x03,
    ADDRESS_OVER = 0x04,
    OT_EXECUTABLE_IN_PROGRAM_MODE = 0x0B,
    FCS_ERROR = 0x13,
    FORMAT_ERROR = 0x14,
    ENTRY_NUMBER_DATA_ERROR = 0x15,
    COMMAND_NOT_SUPPORTED = 0x16,
    FRAME_LENGHT_ERROR = 0x18,
    OUT_OF_RANGE = 0x40,
    NO_PLC_ANSWER = 0x50
} Send_Status;

typedef enum
{
    PROGRAM_MODE = 0,
    RUN_MODE = 3,
    MONITOR_MODE = 2
} PLC_MODEs;

typedef enum
{
    CIO_AREA = 0,
    LR_AREA = 1,
    HR_AREA = 2,
    TIMMER_COUNTER_PV = 3,
    TIMMER_COUNTER_STATUS = 4,
    DM_AREA = 5,
    AR_AREA = 6,
    EM_AREA = 7,
    TIMMER = 8,
    COUNTER = 9
} MEM_AREA;

//
#define I2CDEV_DEFAULT_READ_TIMEOUT 1000
class HOSTLINK
{

public:
    String ABORT = "@00XZ42*";
    /*     typedef enum
        {
    MONITOR="",
        } PLC_MODE;
     */

    HOSTLINK();

    //HardwareSerial serial;
    //  HOSTLINK(HardwareSerial &serial);
    //   HOSTLINK(int TX_Pin, int RX_Pin);


#ifdef ESP8266
    SoftwareSerial Serial1;
    #endif
#ifdef ESP32
    HardwareSerial Serial1;
#endif

    HOSTLINK(int TX_Pin, int RX_Pin);
    Send_Status sendToPLC(String command, int len_, String *ans);

    Send_Status writeWord(MEM_AREA area, int begining_word, uint16_t word);

    uint16_t readWord(MEM_AREA area, int addres_word);

    Send_Status readWord(MEM_AREA area, int addres_word, String *ans);
    Send_Status forceCoil(MEM_AREA area, int addres_word, int bit_, bool state);
    Send_Status bitForceSet(MEM_AREA area, int addres_word, int bit_);
    Send_Status bitForceReset(MEM_AREA area, int addres_word, int bit_);
    Send_Status forcedToggleBitCoil(MEM_AREA area, int word_number, int bit_number);

    Send_Status cancelAllForced();

    Send_Status writeInt(MEM_AREA area, int begining_word, int data);
    Send_Status writeFloat(MEM_AREA area, int begining_word, float data);
    Send_Status writeDouble(MEM_AREA area, int begining_word, double data);

    Send_Status writeWords(MEM_AREA area, int begining_word, String data);
    Send_Status writeWords(MEM_AREA area, int begining_word, uint16_t data[], int number_of_words); // en esta
    // Read words and return it into a uint16_t array passed by reference in ans var
    Send_Status readWords(MEM_AREA area, int begining_word, int no_of_words, uint16_t ans[]);

    bool readBitCoil(MEM_AREA area, int word_number, int bit_number);
    Send_Status writeBitCoil(MEM_AREA area, int word_number, int bit_number, bool state /* high(1) or low(0)*/);
    Send_Status toggleBitCoil(MEM_AREA area, int word_number, int bit_number);

    uint16_t *readWords(MEM_AREA area, int begining_word, int no_of_words);

    Send_Status readWords(MEM_AREA area, int begining_word, int no_of_words, String *ans);
    String recieveFromPLC(bool *ans_ok);
    bool init();
    PLC_MODEs changePLCStatus(PLC_MODEs mode);

    int autoDetectBauds();

private:
    bool checkMsg(String *command, int len_);
    void add_FCS(String *command_, int len_);
    uint8_t char2int(char input);
    String int2char(uint16_t input);
};

#endif