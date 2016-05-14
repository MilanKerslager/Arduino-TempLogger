Arduino Mega 2560

// Tepmerature chip DHT11
#include <DHT.h> // Arduino library
#define DHTPIN 30
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Clock chip
#include <DS1302RTC.h>  // http://playground.arduino.cc/Main/DS1302RTC
// Set pins:  CE/RST, IO/DATA, CLK
DS1302RTC RTC(40, 41, 42);

#include <Time.h>      // http://playground.arduino.cc/Code/Time
tmElements_t tm;

// MicroSD reader: Arduino SD library
#include <SPI.h>
#include <SD.h>
const int chipSelect = 53;
File dataFile;

// Arduino Multi-function shield (for displaing temperature)
/* Define shift register pins used for seven segment display */
#define LATCH_DIO 4
#define CLK_DIO 7
#define DATA_DIO 8

/* Segment byte maps for numbers 0 to 9 and Blank*/
const byte SEGMENT_MAP[] = {0xC0, 0xF9, 0xA4, 0xB0, 0x99, 0x92, 0x82, 0xF8, 0x80, 0x90, 0xFF};
/* Byte maps to select digit 1 to 4 */
const byte SEGMENT_SELECT[] = {0xF1, 0xF2, 0xF4, 0xF8};

int chk;
long wait = 5000;
long timenow;
String dataString;
float humidity;
float temperature;

void setup() {
  WriteNumber(-1); // blank display
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  dht.begin();

  // Set DIO pins for LCD (Multi-function Shield) to outputs
  pinMode(LATCH_DIO, OUTPUT);
  pinMode(CLK_DIO, OUTPUT);
  pinMode(DATA_DIO, OUTPUT);

  if (RTC.haltRTC())
    Serial.print("Clock stopped!");
  else
    Serial.print("Clock working.");

  Serial.print(" Time (DS1302) read...");
  if (RTC.read(tm)) {
    Serial.println("error!  Please check the circuitry.");
  } else {
    Serial.println("OK.");
  }

  Serial.print("Setting system time from RTC...");
  setSyncProvider(RTC.get); // the function to get the time from the RTC
  if (timeStatus() == timeSet)
    Serial.println(" OK.");
  else
    Serial.println(" FAIL!");
  
  Serial.print("Initializing SD card...");
  if (!SD.begin(chipSelect)) {
    Serial.println("FAILED (or not present)!");
    // don't do anything more:
    while (1) ;
  }
  Serial.println("OK.");

  // Open up the file we're going to log to!
  const String fileName = "teploty.txt";
  dataFile = SD.open(fileName, FILE_WRITE);
  if (! dataFile) {
    Serial.println("error opening datalog.txt");
    // Wait forever since we cant write data
    while (1) ;
  } else {
    Serial.print("Opened file: ");
    Serial.println(fileName);
  }

  timenow = millis();
  while (millis() < timenow + 500) {
    WriteNumber(8888);
  }
  timenow = millis() - wait; // set timer to the expired value
  Serial.println();
}

void loop() {
  if (millis() > timenow + wait) {
    // reading temp is time expensive operation
    // so we go there once per "wait" miliseconds
    WriteNumber(-1);  // blank display
    timenow = millis(); // reset counter
    humidity = dht.readHumidity();
    temperature = dht.readTemperature();
    if (! (isnan(humidity) || isnan(temperature))) {
      dataString = "";
      dataString = printDate() + ";";
      dataString += printTime() + ";";
      dataString += String(humidity) + ";";
      dataString += String(temperature) + "\n";
      Serial.print(dataString);
      dataFile.print(dataString);
      dataFile.flush();
    }
  }
  // write already known temperature to the LCD repeatedly
  WriteNumber(temperature);
}

String printDate() {
  String ret;
  ret = day();
  ret += ".";
  ret += month();
  ret += ".";
  ret += year();
  return ret;
}

String printTime() {
  String ret;
  ret = hour();
  ret += ":";
  ret += print2digits(minute());
  ret += ":";
  ret += print2digits(second());
  return ret;
}

String print2digits(int number) {
  // Output leading zero
  if (number >= 0 && number < 10) {
    return ("0" + String(number));
  }
  return (String(number));
}

void WriteNumber(int Num)
{
  if (Num > 999) WriteNumberToSegment(0 , (Num / 1000));
  if (Num > 99) WriteNumberToSegment(1 , ((Num / 100) % 10));
  if (Num > 9) WriteNumberToSegment(2 , ((Num / 10) % 10));
  if (Num >= 0) WriteNumberToSegment(3 , (Num % 10));
  else WriteNumberToSegment(3, 10);  // blank display, if Num <= 0
}

/* Write a decimal number between 0 and 9 to one of the 4 digits of the display */
void WriteNumberToSegment(byte Segment, byte Value)
{
  digitalWrite(LATCH_DIO, LOW);
  shiftOut(DATA_DIO, CLK_DIO, MSBFIRST, SEGMENT_MAP[Value]);
  shiftOut(DATA_DIO, CLK_DIO, MSBFIRST, SEGMENT_SELECT[Segment] );
  digitalWrite(LATCH_DIO, HIGH);
}
