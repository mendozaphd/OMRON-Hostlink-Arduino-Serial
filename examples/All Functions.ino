#include <Arduino.h>
// void setup()
// {
//     Serial.begin(115200);

//     Serial1.begin(9600,SERIAL_7E2);
// }
// void loop()
// {

//     Serial1.println("@00TS0123456789ABCDEFG06*");
//     if (Serial1.available())
//     {
//         int av = Serial1.available();
//         Serial.print("Ans of "+ String(av) + ">>");
//         for (int i = 0; i < av; i++)
//         {
//             Serial.print(char(Serial1.read()));
//         }
//         Serial.println("<<--" + String(av) + "--");
//     }
//     delay(500);
// }

#include <Hostlink.h>
//#include <esp_task_wdt.h> //Watchdog functions

#define RXD2_PIN 5 // Pin RX RS-232
#define TXD2_PIN 4 // Pin TX RS-232

HOSTLINK plc(RXD2_PIN, TXD2_PIN);

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    // Serial1.begin(9600);
    int baudios = 0;
    while (1)
    {
        baudios = plc.autoDetectBauds(); // auto detect the speed of the serial hostlink bus(could be omited if you know the configuration speed)
        if (baudios)
            break;
    }
    Serial.println("PLC response at " + String(baudios) + " bps"); // just print the speed founded

    if (plc.init()) // init the plc communication changing the PLC to monitor mode(Monitor mode allow us to write in the PLC)
    {
        Serial.println("PLC Founded and set it in Monitor Mode");
    }
    else
        Serial.println("PLC doesnt Founded or dont putted in Monitor Mode");
}

void loop()
{

    /*>>>>>>>>>>>>>>>>>>>>>> WRITING SIGNED INT VALUES(of 16 bits) IN MEMORIES (REQUIRES 1 WORD IN THE PLC)<<<<<<<<<<<<<<<<<<<<<< */

    int Signed_Int_Value = random(0, 65535); // the number must be in this range (just 16 bits)
    int ADDRESS = 33;                        // MEM_ADRESS where we are going to write/read

    plc.writeInt(HR_AREA, ADDRESS, Signed_Int_Value); // Write Signed_Int_Value the to H Mem, to the ADDRESS

    /*>>>>>>>>>>>>>>>>>>>>>> READING THE SAME SIGNED INT VALUE(of 16 bits) FROM PLC  <<<<<<<<<<<<<<<<<<<<<< */

    /*READ OPTION 1(read just 1 word and assign it intto uint16_t Var*/
    uint16_t Int_Value_In_PLC = plc.readWord(HR_AREA, ADDRESS); // read the value in the same ADDRESS and save it in Int_Value_In_PLC var

    /*READ OPTION 2(read any words into uint16_t array Var*/
    uint16_t Int_Value_In_PLC_2[3];
    Send_Status error = plc.readWords(HR_AREA, ADDRESS, 1, Int_Value_In_PLC_2); // read 1 word and save it in the uint16_t array named Int_Value_In_PLC_2
    if (!error)
    {
        if (int(Int_Value_In_PLC) == Signed_Int_Value && int(Int_Value_In_PLC_2[0]) == Signed_Int_Value) // check if the value writted is the same that the readed
            Serial.println("The Value Writted In PLC was " + String(Signed_Int_Value) + " and the Readed Value " + String(Int_Value_In_PLC) + " are the same!");
    }
    else
        Serial.println("Someting bad happens while Writting Int Value"); // this should be printed
    /*>>>>>>>>>>>>>>>>>>>>>> WRITING FLOAT VALUES IN MEMORIES (REQUIRES 2 CONTINUOS WORDS OF THE PLC) <<<<<<<<<<<<<<<<<<<<<< */

    float Temperature_Sensor_Value = 1234567.89; // Generate Random Float Value(could be a real value of one sensor)

    error = plc.writeFloat(DM_AREA, 100, Temperature_Sensor_Value); // Send to the DM_AREA memory in the address 100 the float value and get the Send_Status of the operation

    if (error == NORMAL_COMPLETION)                        // Check if the command was successfully
        Serial.println("All OK Writting the FLoat Value"); // this should be printed
    else
        Serial.println("Someting bad Writting the Float Value"); // this should be printed

    /*>>>>>>>>>>>>>>>>>>>>>> WRITING LONG VALUES IN MEMORIES (REQUIRES 4 CONTINUOS WORDS OF THE PLC) <<<<<<<<<<<<<<<<<<<<<< */

    long Any_Sensor_Value = 1234567; // Generate Random Long Value(could be a real value of one sensor)

    error = plc.writeDouble(DM_AREA, 150, Any_Sensor_Value); // Send to the DM_AREA memory in the address 100 the float value and get the Send_Status of the operation

    if (error == NORMAL_COMPLETION)                       // Check if the command was successfully
        Serial.println("All OK Writting the Long Value"); // this should be printed
    else
        Serial.println("Someting bad Writting the Long Value"); // this should be printed

    /*>>>>>>>>>>>>>>>>>>>>>> WRITING CONTUNUOSLY MULTIPLE WORDS FROM AN UINT16_T ARRAY <<<<<<<<<<<<<<<<<<<<<< */
    int array_len = 2;
    uint16_t RandomValue = random(0, 0xffff);
    uint16_t datosW[2] = {RandomValue, 0xAAAA};

    error = plc.writeWords(CIO_AREA, 0, datosW, array_len); // WRITE a RandomValue TO THE CIO AREA 0 AND 0xAAAA TO THE CIO AREA 1

    if (error == NORMAL_COMPLETION)                       // Check if the command was successfully
        Serial.println("All OK to MULTIPLE WORDS FROM AN UINT16_T ARRAY WRITiNG"); // this should be printed
    else
        Serial.println("Someting bad Writing MULTIPLE WORDS FROM AN UINT16_T ARRAY"); // this should be printed
    /*>>>>>>>>>>>>>>>>>>>>>> READING THE SAME ARRAY OF WORDS  <<<<<<<<<<<<<<<<<<<<<< */

    uint16_t datosR[3] = {0};

    plc.readWords(CIO_AREA, 0, array_len, datosR);

    bool are_equal = true;

    for (int i = 0; i < array_len; i++)
        if (datosR[i] != datosW[i])
            are_equal = false;

    if (are_equal)
        Serial.println("The values of the array DatosW are the Same of the values readed in the DatosR aray");
    else
        Serial.println("The readed Value doesnt match whit the Writted Values"); // or one or more coils are forced in this Address MEMORY Area or maybe are being used by the PLC PROGRAM

    /*>>>>>>>>>>>>>>>>>>>>>> FORCED SET/RESET/TOGGLE BIT IN ANY MEMORY AREA  <<<<<<<<<<<<<<<<<<<<<< */

    for (int i = 0; i < 16; i++) // FORCE SET(change to 1) THE CIO 0.0 TO 0.16 OUTPUT
        plc.bitForceSet(CIO_AREA, 0, i);
Serial.print("1,");
    delay(300);
    plc.forcedToggleBitCoil(CIO_AREA, 0, 7); // force toggle(change the bit status to the opositive) CIO 0.7
    delay(300);

    Serial.print("2,");

    for (int i = 15; i >= 0; i--) // FORCE RESET(change to 0) THE CIO 0.0 TO 0.16 OUTPUT
        plc.bitForceReset(CIO_AREA, 0, i);

    delay(300);
    Serial.print("3,");

    plc.forcedToggleBitCoil(CIO_AREA, 0, 7); // force toggle(change the bit status to the opositive state) CIO 0.7

    delay(300);
Serial.print("4,");
    for (int i = 0; i < 16; i++) // FORCE SET(1) THE CIO 0.0 TO 0.16 OUTPUT
        plc.bitForceSet(CIO_AREA, 0, i);

    delay(1000);
Serial.print("5,");
    plc.cancelAllForced(); // Cancel all the forced signals(ALL Signals, even the signals forced from cx-programmer)
Serial.print("77,");

uint16_t empty = 0x0000;
  error =  plc.writeWord(CIO_AREA, 0, empty); // write 0 to the entire word in the same area(turn OFF the 16 coils)  //aqui no lo esta haciendo

    if (error == NORMAL_COMPLETION)                       // Check if the command was successfully
        Serial.println("All OK to MULTIPLE WORDS FROM AN UINT16_T ARRAY WRITiNG"); // this should be printed
    else
        Serial.println("Someting bad Writing MULTIPLE WORDS FROM AN UINT16_T ARRAY"); // this should be printed
    /*>>>>>>>>>>>>>>>>>>>>>> READ AND WRITE INDIVIDUAL COILS IN ANY MEMORY AREA (NOT FORCED)   <<<<<<<<<<<<<<<<<<<<<< */
    Serial.print("60,");
     bool   H5_0 = plc.readBitCoil(HR_AREA, 5, 0); // read the status of the bit tha was written

    Serial.println(H5_0 ? "The Bit H5.0 is NOW ON" : "The Bit H5.0 is NOW OFF");

    plc.writeBitCoil(HR_AREA, 5, 0, 0); // Write 0 to the HR Memory Bit H5.0
Serial.print("6,");

    H5_0 = plc.readBitCoil(HR_AREA, 5, 0); // Read the status of the bit H5.0

    Serial.println(H5_0 ? "The Bit H5.0 is ON" : "The Bit H5.0 is OFF");

    plc.writeBitCoil(HR_AREA, 5, 0, 1); // Write 1 to the HR Memory Bit H5.0

    H5_0 = plc.readBitCoil(HR_AREA, 5, 0); // read the status of the bit tha was written

    Serial.println(H5_0 ? "The Bit H5.0 is NOW ON" : "The Bit H5.0 is NOW OFF");

    delay(1000);
    Serial.print("end,");
}
