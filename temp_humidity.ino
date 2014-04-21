// vim:ts=4:tw=80:expandtab:syntax=c

#include <Ethernet.h>
#include <Exosite.h>
#include <EEPROM.h>
#include <SPI.h>

// for bitbang i2c tmp102 reading
#define TMP	0x49
#define TMP_W (TMP << 1)
#define TMP_R (TMP_W | 1)
#define TEMP_REG 0

/* 
In my makefile I have this
CPPFLAGS+=-DCIK2=\"${CIK2}\"

and in my ~/.bashrc I have 
export CIK2=674e....4367
*/

char cikData[] = CIK2;          // <-- FILL IN YOUR CIK HERE! (https://portals.exosite.com -> Add Device)
byte macData[] = { 0x90, 0xA2, 0xDA, 0x00, 0xF4, 0xAA };

#define REPORT_TIMEOUT 60000    //milliseconds period for reporting to Exosite.com
// Pin 9 has a led on the 'ethernet' board
int led = 9;
unsigned long sendPrevTime;
char line[40];
int v16;
char celsius[40] = "97.6";
char writeParam[80] = "";
char readParam[50] = "";
char *r = (char *)malloc(1);

class EthernetClient client;
Exosite exosite(cikData, &client);

unsigned char I2C_Read(unsigned char ack);
void I2C_WriteBit(unsigned char c);
unsigned char I2C_ReadBit();
void I2C_Init();
void I2C_Start();
void I2C_Stop();
unsigned char I2C_Write(unsigned char c);
unsigned char I2C_Read(unsigned char ack);

// the setup routine runs once when you press reset:
void setup()
{
    pinMode(led, OUTPUT);
    I2C_Init();
    Ethernet.begin(macData);
    Serial.begin(9600);
    Serial.println("Boot");
    delay(1000);
    Serial.println(Ethernet.localIP());

    I2C_Start();
    I2C_Write(TMP_W);
    I2C_ReadBit();
    I2C_Write(TEMP_REG);
    I2C_ReadBit();
    I2C_Stop();

    Serial.println(line);
    sendPrevTime = millis();
    delay(100);
}

void read_tmp102()
{
    unsigned char l, h;

    I2C_Start();
    I2C_Write(TMP_R);
    I2C_ReadBit();
    h = I2C_Read(1);
    delay(10);
    l = I2C_Read(0);
    v16 = h << 4 | ((l >> 4) & 0xf);
    dtostrf(v16 * 0.0625, 0, 2, celsius);
    Serial.print(celsius);
    Serial.print(", ");
    I2C_Stop();
}

// the loop routine runs over and over again forever:
void loop()
{
    digitalWrite(led, HIGH);    // turn the LED on (HIGH is the voltage level) 
    delay(200);                 // wait for a second
    digitalWrite(led, LOW);     // turn the LED off by making the voltage LOW 
    delay(50);                  // wait for a second
    r[0] = 0;

    read_tmp102();
    // Send to Exosite every defined timeout period
    if (millis() - sendPrevTime > REPORT_TIMEOUT) {
        Serial.println("Sending to exosite");
        sprintf(writeParam, "T=%s", celsius);
        Serial.print("'");
        Serial.print(writeParam);
        Serial.println("'");
        if (exosite.writeRead(writeParam, readParam, &r)) {
            Serial.println("Return");
            if (r[0] != 0) {
                Serial.println(r);
            }
        } else {
            Serial.println("Exosite Error");
        }
        sendPrevTime = millis();        //reset report period timer
    }
    delay(500);
}

// Port for the I2C
#define I2C_DDR DDRD
#define I2C_PIN PIND
#define I2C_PORT PORTD

// Pins to be used in the bit banging
#define I2C_CLK 7
#define I2C_DAT 6

#define I2C_DATA_HI()\
    I2C_DDR &= ~ (1 << I2C_DAT); I2C_PORT |= (1 << I2C_DAT);
#define I2C_DATA_LO()\
    I2C_DDR |= (1 << I2C_DAT); I2C_PORT &= ~ (1 << I2C_DAT);

#define I2C_CLOCK_HI()\
    I2C_DDR &= ~ (1 << I2C_CLK); I2C_PORT |= (1 << I2C_CLK);
#define I2C_CLOCK_LO()\
    I2C_DDR |= (1 << I2C_CLK); I2C_PORT &= ~ (1 << I2C_CLK);

void I2C_WriteBit(unsigned char c)
{
    if (c > 0) {
        I2C_DATA_HI();
    } else {
        I2C_DATA_LO();
    }

    I2C_CLOCK_HI();
    while ((I2C_PIN & (1 << I2C_CLK)) == 0) {
    }
    delay(1);

    I2C_CLOCK_LO();
    delay(1);

    if (c > 0) {
        I2C_DATA_LO();
    }

    delay(1);
}

unsigned char I2C_ReadBit()
{
    I2C_DATA_HI();

    I2C_CLOCK_HI();
    while ((I2C_PIN & (1 << I2C_CLK)) == 0) {
    }
    delay(1);

    unsigned char c = I2C_PIN;

    I2C_CLOCK_LO();
    delay(1);

    return (c >> I2C_DAT) & 1;
}

// Inits bitbanging port, must be called before using the functions below
//
void I2C_Init()
{
    I2C_PORT &= ~((1 << I2C_DAT) | (1 << I2C_CLK));

    I2C_CLOCK_HI();
    I2C_DATA_HI();

    delay(1);
}

// Send a START Condition
//
void I2C_Start()
{
    // set both to high at the same time
    I2C_DDR &= ~((1 << I2C_DAT) | (1 << I2C_CLK));
    delay(1);

    I2C_DATA_LO();
    delay(1);

    I2C_CLOCK_LO();
    delay(1);
}

// Send a STOP Condition
//
void I2C_Stop()
{
    I2C_CLOCK_HI();
    delay(1);

    I2C_DATA_HI();
    delay(1);
}

// write a byte to the I2C slave device
//
unsigned char I2C_Write(unsigned char c)
{
    for (char i = 0; i < 8; i++) {
        I2C_WriteBit(c & 128);

        c <<= 1;
    }

    //return I2C_ReadBit();
    return 0;
}

// read a byte from the I2C slave device
//
unsigned char I2C_Read(unsigned char ack)
{
    unsigned char res = 0;

    for (char i = 0; i < 8; i++) {
        res <<= 1;
        res |= I2C_ReadBit();
    }

    if (ack > 0) {
        I2C_WriteBit(0);
    } else {
        I2C_WriteBit(1);
    }

    delay(1);

    return res;
}
