#include <Max3421e.h>
#include <Usb.h>
#include <AndroidAccessory.h>
#include "commands.h"

///////////////////////// ADK CONFIG /////////////////////////////
#define BUFFER_SIZE_FOR_IO 256
byte bufferI[BUFFER_SIZE_FOR_IO];
byte bufferO[BUFFER_SIZE_FOR_IO];

//("Manufacturer", "Model", "Description","Version", "URI", "Serial");
AndroidAccessory acc("UPC", "ArduinoADK", "Description","1.0", "URI", "Serial");
//////////////////////////////////////////////////////////////////

///////////////////////// RC CONFIG //////////////////////////////
#define chanel_number 8  // Number of chanels
#define PPM_FrLen 27000  // PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 300 // Pulse length
#define onState 0        // Polarity of the pulses: 1 is positive, 0 is negative
#define ppmOutPin 10     // PPM signal output pin on the arduino
#define ppmInPin 4       // PPM signal input pin on the arduino

// RC PPM values are stored here
int ppm[chanel_number];
//////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);


  // ADK
  acc.powerOn();


  // PPM
  pinMode(ppmInPin, INPUT);
  pinMode(ppmOutPin, OUTPUT);
  digitalWrite(ppmOutPin, !onState);  //set the PPM signal pin to the default state (off)

  // Setup timer1 (ppm output)
  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;

  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

long readSensor() {
  //return random(-2147483648, 2147483647); // whole long range
  return random(-1024000, 1024000);
}

void sendSensorData(byte sensor, long value) {
  bufferO[0] = sensor;

  // long to bytes (4)
  bufferO[4] = (byte) value;
  bufferO[3] = (byte) (value >> 8);
  bufferO[2] = (byte) (value >> 16);
  bufferO[1] = (byte) (value >> 24);

  acc.write(bufferO, 5);
}

void attendCommand(byte command) {
  long sensorData;

  switch(command) {
  case READ_SENSOR_1:
    Serial.println("Reading sensor 1 ...");
    sensorData = readSensor();
    Serial.print("Sensor 1 value: ");
    Serial.println(sensorData);

    sendSensorData(DATA_SENSOR_1, sensorData);
    break;

  case READ_SENSOR_2:
    Serial.println("Reading sensor 2 ...");
    sensorData = readSensor();
    Serial.print("Sensor 2 value: ");
    Serial.println(sensorData);

    sendSensorData(DATA_SENSOR_2, sensorData);
    break;

  case READ_SENSOR_3:
    Serial.println("Reading sensor 3 ...");
    sensorData = readSensor();
    Serial.print("Sensor 3 value: ");
    Serial.println(sensorData);

    sendSensorData(DATA_SENSOR_3, sensorData);
    break;

  default:
    break;
  }
}

void loop() {
  byte receivedCommand;


  // Read PPM frame
  if(pulseIn(ppmInPin, HIGH) > 3000) // New PPM frame starts after this long pulse
  {
    for(int i = 0; i <= chanel_number-1; i++) // Read channels
    {
      ppm[i]=pulseIn(ppmInPin, HIGH)+400;
    }
  }

  // Manage ADK connection
  if (acc.isConnected()) {
    // Read command
    int len = acc.read(bufferI, sizeof(bufferI), 1);

    if (len == 1) {
      // Is a command (1 byte)
      receivedCommand = bufferI[0];

      attendCommand(receivedCommand);
    }
    else {
      // Not a command, ignore
    }
  }
}

// Do not modify (PPM signal generation)
ISR(TIMER1_COMPA_vect){
  static boolean state = true;

  TCNT1 = 0;

  if(state) {  // Start pulse
    digitalWrite(ppmOutPin, onState);
    OCR1A = PPM_PulseLen * 2;
    state = false;
  }
  else{  // End pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;

    digitalWrite(ppmOutPin, !onState);
    state = true;

    if(cur_chan_numb >= chanel_number){
      cur_chan_numb = 0;
      calc_rest = calc_rest + PPM_PulseLen;// 
      OCR1A = (PPM_FrLen - calc_rest) * 2;
      calc_rest = 0;
    }
    else{
      OCR1A = (ppm[cur_chan_numb] - PPM_PulseLen) * 2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}
