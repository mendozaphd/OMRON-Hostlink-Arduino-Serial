#include "Hostlink.h"
#include <string.h>

using namespace std;

String buffer, tmp_cmd;
String test_message = "@00TS0123456789ABCDEFG*";
String mem_area_headers[8] = {"R" /*CIO AREA*/, "L" /*LR AREA*/, "H" /*HR AREA*/, "C" /*TIMMER/COUNTER PV READ*/, "G" /*TIMMER/COUNTER STATUS READ*/, "D" /*DM AREA*/, "J" /*AR AREA*/, "E" /*EM AREA*/};
String mem_area_clasifications[10] = {"CIO " /*CIO AREA*/, "LR  " /*LR AREA*/, "HR  " /*HR AREA*/, "****" /*TIMMER*/, "****" /*COUNTER*/, "DM  " /*DM AREA*/, "AR  " /*AR AREA*/, "EM  " /*EM AREA*/, "TIM " /*TIMMER*/, "CNT " /*COUNTER*/};

unsigned long BAUDS[] = {9600, 19200, 38400, 57600, 115200, 4800, 2400};

unsigned long time_out;
int ans_frames = 0;
int RXD_pin, TXD_pin;

HOSTLINK::HOSTLINK()
{
}


HOSTLINK::HOSTLINK(int RX_Pin, int TX_Pin)
{
    RXD_pin = RX_Pin;
    TXD_pin = TX_Pin;
}

bool HOSTLINK::init()
{

    Serial1.println(ABORT);
    sendToPLC(ABORT, ABORT.length(), &buffer);
    Serial1.println("@**");
    for (int i = 0; i < 10; i++)
        Serial1.read();

    String test_ = test_message;
    add_FCS(&test_, test_.length());
    test_ += '*';
    sendToPLC(test_message, test_message.length(), &buffer);
    sendToPLC(test_message, test_message.length(), &buffer);
    bool tst_cmd = false;
    if (!buffer.compareTo(test_))
        tst_cmd = true;
    // changePLCStatus(MONITOR_MODE);
    // Serial.print("MOnitor...");
    if (changePLCStatus(MONITOR_MODE) == MONITOR_MODE)
    {
        Serial.println("El plc esta en modo monitor");
        return tst_cmd;
    }
    return false;
}

PLC_MODEs HOSTLINK::changePLCStatus(PLC_MODEs mode)
{
    char mode_ = '0' + mode;
    String SC = "@00SC0"; // SC COMMAND Changes the CPU Unit operating mode.
    // Serial.println("Monitor mode command1:" + SC);
    SC = SC + mode_ + '*';
    // Serial.println("Monitor mode command2:" + SC);
    sendToPLC(SC, SC.length(), &buffer);
    SC = "@00MS5E*"; // MS COMMAND reads the operating conditions (status) of the CPU Unit.
    sendToPLC(SC, SC.length(), &buffer);
    if ((buffer[8] - '0') == 0)
        return PROGRAM_MODE;
    else if ((buffer[8] - '0') == 3)
        return MONITOR_MODE;
    return PLC_MODEs((int)buffer[8] - '0');
}

Send_Status HOSTLINK::sendToPLC(String command, int len_, String *ans)
{
    time_out = millis() + 350;
    // unsigned long time_start = millis();

    if (command[0] != '@' || command[len_ - 1] != '*') // Check if the command to send Starts with @ and ends with *
        return FORMAT_ERROR;

    if (checkMsg(&command, len_))
    {
        add_FCS(&command, len_);
        command += '*';
    }

    if (Serial1.available())
    { // put buffer in 0
        Serial1.println(ABORT);
        // Serial.print("Buffer=");
        delay(50); // wait to end abort comand
        while (Serial1.available())
        {
            Serial.print((char)Serial1.read());
            if (Serial1.available() < 2)
                delay(10);
        }
    }

    Serial1.println(command);
    bool ans_verified = false;

    ans_frames = 0;
    String ans_buff = recieveFromPLC(&ans_verified);

    if (ans_buff.length() < 1)
    {
        Serial.println("PLC no Answer to the Command[" + command + "]");
        return NO_PLC_ANSWER;
    }

    if (ans_buff == "@00QQMR005F*")
    { // is an answere to send a QQIR command
        String QQIR = "@00QQIR5B*";
        Serial.println("Es un mensaje QQMR");
        Serial1.println(QQIR);
        ans_buff = recieveFromPLC(&ans_verified);
    }

    // if (ans_verified)
    //     Serial.print("PLC ans OK to the Command[" + command + "] in " + String(ans_frames + 1) + " frames ANS=[" + String(ans_buff) + "]" + String());
    // else
    //     Serial.print("PLC ans NG to the Command[" + command + "] in " + String(ans_frames + 1) + " frames ANS=[" + String(ans_buff) + "]" + String());
    // Serial.print(" - in " + String(millis() - time_start) + " ms - ");
    // (*ans).clear();
    (*ans) = "";
    if (ans_verified)
    {
        *ans = ans_buff;

        char msg_end_code[2] = "";
        if (ans_buff[3] != 'T' && ans_buff[3] != 'X' && ans_buff[1] != '*')
        {
            // end_coded_msg = true;
            if (ans_buff[3] == 'Q') // QQMR & QQMR comands generates the end code in diferent positions
            {
                msg_end_code[0] = ans_buff[7];
                msg_end_code[1] = ans_buff[8];
            }
            else
            {
                msg_end_code[0] = ans_buff[5];
                msg_end_code[1] += ans_buff[6];
            }

            unsigned int end_ = strtol(msg_end_code, 0, 16);
            //  Serial.println("   EndCode=" + String(end_));

            return Send_Status(end_);
        }
    }
    else
        return FCS_ERROR;
    return NORMAL_COMPLETION;
}

String HOSTLINK::recieveFromPLC(bool *ans_ok)
{
    char char_ = 0;
    String ans_[18], ans_buff;
    bool ans_check[18];
    int ans_frames = 0, iter = 0;
    *ans_ok = true;
    while (time_out > millis())
    {
        if (Serial1.available())
        {
            // Serial.print(",");
            //  Serial.print((char)Serial1.read());
            char_ = (char)Serial1.read();
            ans_[ans_frames] += char_;
            if (char_ == '@')
            {
                for (int y = 0; y <= ans_frames; y++)
                    ans_[y] = "";
                // ans_[y].clear();
                ans_frames = 0;
                ans_[0] += '@';
            }
            if (char_ == '*')
            {
                Serial1.read();
                // Serial.print("--ASS->" + String(ans_frames) + "<");
                ans_check[ans_frames] = true;
                String ans__ = ans_[ans_frames];
                ans__.remove(ans__.length() - 2);
                add_FCS(&ans__, (ans__.length()));
                // Serial.print("--Comprobando >" + ans__ + "< len=" + String(ans__.length()) + " con >" + ans_[ans_frames] + "< len=" + String(ans_[ans_frames].length()));

                for (int f = 0; f < int(ans__.length()); f++)
                    if (ans__[f] != ans_[ans_frames][f])
                    {
                        ans_check[ans_frames] = false;
                        // Serial.println("Ans__=" + String(ans__[f]) + "< ans[i][" + String(f) + "]=" + String(ans_[ans_frames][f]) + "<");
                        break;
                    }
                if (ans_check[ans_frames])
                {
                    ans_[ans_frames].remove(ans_[ans_frames].length() - 3);
                    if (ans_frames == 0)
                    {
                        ans_[ans_frames] = ans__;
                        ans_[ans_frames] += '*';
                    }
                    // Serial.println("Ans correcta#" + String(ans_frames) + "=" + ans_[ans_frames]);
                }

                break;
            }
            if (char_ == 13)
            {
                time_out += 200; // add 100 ms to the wait time
                ans_check[ans_frames] = true;
                String ans__ = ans_[ans_frames];
                ans__.remove(ans__.length() - 2);
                add_FCS(&ans__, (ans__.length()));
                // Serial.print("--Comprobando >" + ans__ + "< len=" + String(ans__.length()) + " con >" + ans_[ans_frames] + "< len=" + String(ans_[ans_frames].length()));
                // Serial.print("--13->" + String(ans_frames) + "<");
                for (int f = 0; f < int(ans__.length()); f++)
                    if (ans__[f] != ans_[ans_frames][f])
                    {
                        ans_check[ans_frames] = false;
                        // Serial.println("Ans__=" + String(ans__[f]) + "< ans[i][" + String(f) + "]=" + String(ans_[ans_frames][f]) + "<");
                        break;
                    }
                if (ans_check[ans_frames])
                {
                    ans_[ans_frames].remove(ans_[ans_frames].length() - 3);
                    // Serial.println("Ans correcta#" + String(ans_frames) + "=" + ans_[ans_frames]);
                }
                ans_frames++;
            }
        }
        iter++;
        if (iter > 40)
        {
            Serial1.println();
            iter = 0;
        }
        if (ans_[0][0] != '@')
            ans_[0] = "";
        // ans_[0].clear();
    }
    // bool ans_verified = true;
    //(*ans).clear();
    if (ans_frames)
    {
        for (int i = 0; i <= ans_frames; i++)
        {
            if (ans_check[i])
                ans_buff += ans_[i];
            //*ans += ans_[i];
            else
            {
                *ans_ok = false;
                // ans_verified = false;
                // return FCS_ERROR;
            }
        }
        ans_buff += '*';
        add_FCS(&ans_buff, ans_buff.length());
        ans_buff += '*';
    }
    else
    {
        if (ans_check[0])
        {
            ans_buff = ans_[0];
            *ans_ok = true;
            //   *ans = ans_[0];
            //   return NORMAL_COMPLETION;
        }
        else
            *ans_ok = false;
    }

    // if (ans_buff.length() < 2)
    //     *ans_ok = false;

    return ans_buff;
}

bool HOSTLINK::checkMsg(String *command, int len_)
{
    String command_ = *command;
    command_.remove(command_.length() - 3);
    int fcs_value = 0;
    char buf_hex[3];

    for (int i = 0; i < (len_ - 3); i++)
        fcs_value = fcs_value ^ (*command)[i];

    sprintf(buf_hex, "%.2X", fcs_value);
    command_ += buf_hex;
    command_ += "*";
    return command_.compareTo(*command);
}

void HOSTLINK::add_FCS(String *command_, int len_)
{
    int fcs_value = 0;
    char buf_hex[3];
    for (int i = 0; i < (len_ - 1); i++)
        fcs_value = fcs_value ^ (*command_)[i];
    (*command_).remove(len_ - 1);
    sprintf(buf_hex, "%.2X", fcs_value);
    *command_ += buf_hex;
}

int HOSTLINK::autoDetectBauds()
{

    int len = sizeof(BAUDS) / sizeof(BAUDS[0]);
#ifdef ESP8266
    Serial.print("Detecting Bauds at; ");
    for (int i = 0; i < (len); i++)
    {
        Serial.print(String(BAUDS[i]) + " bauds.");
        //  HostLinkSerial.begin(BAUD_RATE, SWSERIAL_7E2, RXD2, TXD2, false, MAXIMO, 250);
        Serial1.begin(BAUDS[i], SWSERIAL_7E2, RXD_pin, TXD_pin, false, 250, 250);
        // Serial1.updateBaudRate(BAUDS[i]);
        String test_ = test_message;
        add_FCS(&test_, test_.length());
        test_ += '*';
        sendToPLC(test_message, test_message.length(), &buffer);
        if (!buffer.compareTo(test_))
        {
            Serial.println("< Success! PLC Founded at " + String(BAUDS[i]));
            return (BAUDS[i]);
        }
        Serial.print("< Fail, testing at; ");
        Serial1.end();
    }
    Serial.println();

#else
    Serial.print("Detecting Bauds at; ");
#ifdef ESP32
    Serial1.begin(19200, SERIAL_7E2, RXD_pin, TXD_pin); // Serial1 init at Hostlink Bauds(could be any speed if you dont know the speed and after use the autoDetectBauds() ESP32 only)
                                                       // #else
                                                       //     Serial1.begin(19200, SERIAL_7E2);
#endif

    for (int i = 0; i < (len); i++)
    {
        Serial.print(String(BAUDS[i]) + " bauds.");
#ifdef ESP32
        Serial1.updateBaudRate(BAUDS[i]);
#else
        Serial1.begin(19200, SERIAL_7E2);
#endif

        String test_ = test_message;
        add_FCS(&test_, test_.length());
        test_ += '*';
        sendToPLC(test_message, test_message.length(), &buffer);
        if (!buffer.compareTo(test_))
        {
            Serial.println("< Success! PLC Founded at " + String(BAUDS[i]));
            return (BAUDS[i]);
        }
        Serial.print("< Fail, testing at; ");
#ifndef ESP32
        Serial1.end();
#endif
    }
    Serial.println();
#endif
    return 0;
}

Send_Status HOSTLINK::writeWords(MEM_AREA area, int begining_word, String data)
{
    String comando = "@00W";
    comando += mem_area_headers[area];
    String words = String(begining_word);
    for (int i = 0; i < int(4 - words.length()); i++)
        comando += '0'; // first 0
    comando += words;
    comando += data;
    comando += '*';
    return sendToPLC(comando, comando.length(), &buffer);
}

Send_Status HOSTLINK::writeWords(MEM_AREA area, int begining_word, uint16_t data[], int number_of_words)
{
    // Serial.print(" -Aqui va bien 1 >" + comando + "<");
    // tmp_cmd.clear();
    tmp_cmd = "";
    tmp_cmd = "@00W";
    //  String words = String(begining_word);
    tmp_cmd += mem_area_headers[area];
    char hex_info[5];
    sprintf(hex_info, "%.4X", begining_word);
    //  sprintf(hex_info, "%04X", *(unsigned long int *)&begining_word);
    tmp_cmd += String(hex_info);

    // Serial.println();
    // Serial.println(">>Comand result=" + tmp_cmd);

    for (int i = 0; i < number_of_words; i++)
    {
        memset(hex_info, 0, 4);
        sprintf(hex_info, "%.4X", data[i]);
        tmp_cmd += String(hex_info);
    }

    //         tmp_cmd += '0'; // first 0
    //                         // Serial.println("Size begin=" + String(words.length()));
    //    // tmp_cmd += words;

    //     String data_;
    //     data_.clear();
    //     // falta arr3glar los datos en uint16_t a una palabra en string

    //     for (int i = 0; i < number_of_words; i++)
    //     {
    //         char hex_string[4];

    //         memset(hex_string, 0, 4);
    //         sprintf(hex_string, "000%X", data[i]);
    //          Serial.print("||");
    //         for (int j = 0; j < 4; j++)
    //         {
    //             if (hex_string[j] == 0)
    //                 hex_string[j] = '0';
    //             Serial.print(",I=" + String(j) + " = " + String(hex_string[j]));
    //         }
    //         //     if (hex_string[j] == 0)
    //         //         hex_string[j] = '0';
    //         data_ += String(hex_string);
    //         Serial.print(hex_string);
    //         Serial.print("||");

    // for (int i = 0; i < number_of_words; i++)
    // {
    //     Serial.print(int2char(data[i]));
    //     data_ += int2char(data[i]);
    // }
    //   Serial.print("  Data= >" + data_ + "<");

    // Serial.print("-Aqui va bien 2 -");
    tmp_cmd += '*';
    // Serial.println("Comand result=" + tmp_cmd);
    return sendToPLC(tmp_cmd, tmp_cmd.length(), &buffer);
    // return Send_Status(0);
}

Send_Status HOSTLINK::readWords(MEM_AREA area, int begining_word, int no_of_words, String *ans)
{
    String comando = "@00R";
    comando += mem_area_headers[area];

    String words = String(begining_word);

    for (int i = 0; i < int(4 - words.length()); i++)
        comando += '0'; // first 0
                        // Serial.println("Size begin=" + String(words.length()));
    comando += words;

    words = String(no_of_words);
    for (int i = 0; i < int(4 - words.length()); i++)
        comando += '0'; // first 0
                        // Serial.println("Size begin=" + String(words.length()));
    comando += words;
    comando += '*';
    return sendToPLC(comando, comando.length(), &(*ans));
}

Send_Status HOSTLINK::writeWord(MEM_AREA area, int begining_word, uint16_t word)
{
    uint16_t word_[1] = {word};
    return writeWords(area, begining_word, word_, 1);
}

Send_Status HOSTLINK::readWord(MEM_AREA area, int addres_word, String *ans)
{
    return readWords(area, addres_word, 1, &(*ans));
}

uint16_t HOSTLINK::readWord(MEM_AREA area, int addres_word)
{
    uint16_t ans[1];
    Send_Status error = readWords(area, addres_word, 1, &(*ans));
    if (error != NORMAL_COMPLETION)
        return 0xFFFF;
    return ans[0];
}

Send_Status HOSTLINK::forceCoil(MEM_AREA area, int addres_word, int bit_, bool state)
{
    String comando = "@00";
    if (state)
        comando += "KS"; // ks to set
    else
        comando += "KR"; // kc to reset
    comando += mem_area_clasifications[area];
    String words = String(addres_word);

    for (int i = 0; i < int(4 - words.length()); i++)
        comando += '0'; // first 0

    comando += words;

    words = String(bit_);
    for (int i = 0; i < int(2 - words.length()); i++)
        comando += '0'; // first 0
                        // Serial.println("Size begin=" + String(words.length()));
    comando += words;
    comando += '*';

    return sendToPLC(comando, comando.length(), &buffer);
}

Send_Status HOSTLINK::bitForceSet(MEM_AREA area, int addres_word, int bit_)
{
    return forceCoil(area, addres_word, bit_, 1);
}

Send_Status HOSTLINK::bitForceReset(MEM_AREA area, int addres_word, int bit_)
{
    return forceCoil(area, addres_word, bit_, 0);
}

Send_Status HOSTLINK::cancelAllForced()
{
    Serial1.println(ABORT);
    return sendToPLC("@00KC48*", 8, &buffer);
}

Send_Status HOSTLINK::readWords(MEM_AREA area, int begining_word, int no_of_words, uint16_t ans[])
{
    Send_Status stat = readWords(area, begining_word, no_of_words, &buffer);

    if (stat != NORMAL_COMPLETION)
        return stat;
    // Serial.println("buffer >" + buffer + "<");
    buffer.remove(buffer.length() - 3);
    buffer.remove(0, 7); // remove frist 7 chars
    uint16_t result_ = 0;
    int char_ = 0, bytecount = 0;
    // Serial.print("int> 0x");
    for (int i = 1; i < int(buffer.length() + 1); i++)
    {
        // char__ = (char)buffer[i];
        char_ = char2int((char)buffer[i - 1]);
        result_ |= char_;
        if ((i % 4) == 0)
        {
            ans[bytecount] = result_;
            bytecount++;
            result_ = 0;
        }
        result_ = result_ << 4;
        // result |= char_;
        // result = result << 4;
        // bytecount++;
    }

    return NORMAL_COMPLETION;
}

uint16_t *HOSTLINK::readWords(MEM_AREA area, int begining_word, int no_of_words)
{
    static uint16_t ans[512];
    readWords(area, begining_word, no_of_words, ans);
    return ans;
}

uint8_t HOSTLINK::char2int(char input)
{
    if (input >= '0' && input <= '9')
        return input - '0';
    if (input >= 'A' && input <= 'F')
        return input - 'A' + 10;
    if (input >= 'a' && input <= 'f')
        return input - 'a' + 10;
    return 0;
}

bool HOSTLINK::readBitCoil(MEM_AREA area, int word_number, int bit_number)
{
    uint16_t word_;
    readWords(area, word_number, 1, &word_);
    return bitRead(word_, bit_number);
}

Send_Status HOSTLINK::writeBitCoil(MEM_AREA area, int word_number, int bit_number, bool state /* high(1) or low(0)*/) // falta?
{
    uint16_t word_, word__[1];
    readWords(area, word_number, 1, &word_);
    bitWrite(word_, bit_number, state);
    word__[0] = word_;
    return writeWords(area, word_number, word__, 1);
}

Send_Status HOSTLINK::toggleBitCoil(MEM_AREA area, int word_number, int bit_number)
{
    uint16_t word_;
    readWords(area, word_number, 1, &word_);
    bitWrite(word_, bit_number, !bitRead(word_, bit_number));
    return writeWord(area, word_number, word_);
}

Send_Status HOSTLINK::forcedToggleBitCoil(MEM_AREA area, int word_number, int bit_number)
{
    return forceCoil(area, word_number, bit_number, !readBitCoil(area, word_number, bit_number));
}

String HOSTLINK::int2char(uint16_t input)
{
    char hex_string[4];
    sprintf(hex_string, "%X", input);

    // Serial.println("Char Arr 0x" + String(hex_string));
    // String output = String(hex_string);
    // Serial.println("String 0x" + output);
    return String(hex_string);
}

Send_Status HOSTLINK::writeInt(MEM_AREA area, int begining_word, int data)
{
    char hex_info[5];
    if (data >= -32768 && data <= 0xFFFF) // 32768
    {
        sprintf(hex_info, "%04X", data);
        // Serial.println("El numero "+ String(float(data)) + " = 0x" + String(hex_info));
        if (data < 0)
        {
            char hex_info_[4];
            for (int i = 0; i < 4; i++)
                hex_info_[i] = hex_info[i + 4];
            //    Serial.println("El numero negativo "+ String(float(data)) + " = 0x" + String(hex_info_));
            return writeWords(area, begining_word, String(hex_info_));
        }
        else
            return writeWords(area, begining_word, String(hex_info));
    }
    else
        return OUT_OF_RANGE;
}

Send_Status HOSTLINK::writeFloat(MEM_AREA area, int begining_word, float data)
{
    char hex_info[10];
    sprintf(hex_info, "%08X", *(unsigned long int *)&data);
    char hex_info_[8];
    for (size_t i = 0; i < 4; i++) // invierte el arreglo
    {
        hex_info_[i] = hex_info[i + 4];
        hex_info_[i + 4] = hex_info[i];
    }
    return writeWords(area, begining_word, String(hex_info_));
}

Send_Status HOSTLINK::writeDouble(MEM_AREA area, int begining_word, double data)
{
    union
    {
        long long i;
        double d;
    } value;

    value.d = data;
    char hex_info[17], hex_info_[17];
    snprintf(hex_info, sizeof(hex_info), "%016llX", value.i);
    hex_info[16] = 0; // make sure it is null terminated.

    for (size_t i = 0; i < 4; i++) // invierte el arreglo
    {
        hex_info_[i] = hex_info[i + 12];
        hex_info_[i + 4] = hex_info[i + 8];
        hex_info_[i + 8] = hex_info[i + 4];
        hex_info_[i + 12] = hex_info[i];
    }
    hex_info_[16] = 0;
    return writeWords(area, begining_word, String(hex_info_));
}
