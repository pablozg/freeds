/*
  FreeDS.ino - Main source file
  Derivador de excedentes para ESP32 DEV Kit // Wifi Kit 32

  Inspired in opends+ (https://github.com/iqas/derivador)
  
  Copyright (C) 2020 Pablo Zerón (https://github.com/pablozg/freeds)

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#define eepromVersion 0x17

#define sizeOfArray(x)  (sizeof(x) / sizeof((x)[0]))

#include <workingmode.h>
#include <Update.h>
#include <Wire.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <TickerScheduler.h>
#include <HardwareSerial.h>

#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <ESPmDNS.h>
#include <SPIFFS.h>
#include <bitmap.h>
#include <DNSServer.h>
#include <esp32ModbusTCP.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <PID_v1.h>

extern "C"
{
#include <crypto/base64.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <rom/rtc.h>
#include <driver/dac.h>
#include <driver/adc.h>
#include <esp_system.h>
#include <time.h>
#include <esp_adc_cal.h>
}

#include "fauxmoESP.h"

#define OLED

#define MAX_SCREENS 5

#define DS18B20 2

#ifdef OLED
  // PWM Pin
  #define pin_pwm 25 // Cambiar a 25

  // ESP Serial
  #define pin_rx 17
  #define pin_tx 5

  // RELAY PINS
  #define PIN_RL1 13
  #define PIN_RL2 12
  #define PIN_RL3 14
  #define PIN_RL4 27

  // ADC PIN
  #define ADC_INPUT 34

  #include "SSD1306.h"
  SSD1306 display(0x3c, 4, 15);
#endif

// PARTIAL UNSUPPORTED, you must define your own pins to use this module.
#ifndef OLED
  // PWM Pin
  uint8_t pin_pwm = 2;

  // ESP Serial
  uint8_t pin_rx = 16;
  uint8_t pin_tx = 17;

  // RELAY PINS
  #define PIN_RL1 5
  #define PIN_RL2 18
  #define PIN_RL3 19
  #define PIN_RL4 21
#endif

// OVERRIDE DEFAULT CONFIG FOR UART1
#define RX1 19
#define TX1 23

// Variables Globales
const char compile_date[] PROGMEM = __DATE__ " " __TIME__;
const char version[] PROGMEM = "1.0.7";
const char beta[]  PROGMEM = "Rev 2";

const char *www_username = "admin";

// Debounce control
struct BUTTON_CONTROL
{
  unsigned long lastDebounceTime = 0;
  unsigned long debounceDelay = 100;
  bool ButtonState = false;
  bool ButtonLongPress = false;
  uint8_t screen = 0;
} button;

// Variables Globales calculo PWM
struct PWM_CONFIG
{
  uint16_t invert_pwm = 0; // Up 1023 with 10 bits resolution.
  uint16_t targetPwm = 0;
  uint8_t pwmValue = 0;
} pwm;

struct SLAVE
{
  uint8_t workingMode;
  uint8_t masterMode = 0;
  uint8_t masterPwmValue = 0;
} slave;

// Temporizadores
struct {
    unsigned long ErrorConexionWifi = 0;
    unsigned long ErrorVariacionDatos = 0;
    unsigned long ErrorRecepcionDatos = 0;
    unsigned long ErrorLecturaTemperatura[3] = {0};
    unsigned long FlashDisplay = 0;
    unsigned long OledAutoOff = 0;
    unsigned long printDebug = 0;
    unsigned long KwToday = 0;
} timers;

// Flags
union {
  uint32_t data;
  struct {
    uint32_t firstInit : 1;       // Bit 0
    uint32_t Updating : 1;        // Bit 1
    uint32_t flash : 1;           // Bit 2
    uint32_t reboot : 1;          // Bit 3
    uint32_t RelayTurnOn : 1;     // Bit 4
    uint32_t RelayTurnOff : 1;    // Bit 5
    uint32_t Relay01Auto : 1;     // Bit 6
    uint32_t Relay02Auto : 1;     // Bit 7
    uint32_t Relay03Auto : 1;     // Bit 8
    uint32_t Relay04Auto : 1;     // Bit 9
    uint32_t Relay01Man : 1;      // Bit 10
    uint32_t Relay02Man : 1;      // Bit 11
    uint32_t Relay03Man : 1;      // Bit 12
    uint32_t Relay04Man : 1;      // Bit 13
    uint32_t setBrightness : 1;   // Bit 14
    uint32_t weblogConnected : 1; // Bit 15
    uint32_t ntpTime : 1;         // Bit 16
    uint32_t timerSet : 1;        // Bit 17
    uint32_t pwmIsWorking : 1;    // Bit 18
    uint32_t pwmManAuto : 1;      // Bit 19
    uint32_t showClampCurrent : 1;// Bit 20
    uint32_t bootCompleted : 1;   // Bit 21
    uint32_t tempShutdown : 1;    // Bit 22
    uint32_t spare : 10;          // Bit 23 - 31
  };
} Flags;

// Flags Errores
union {
  uint16_t data;
  struct {
    uint16_t ConexionWifi : 1;       // Bit 0
    uint16_t RecepcionDatos : 1;     // Bit 1
    uint16_t ConexionMqtt : 1;       // Bit 2
    uint16_t VariacionDatos : 1;     // Bit 3
    uint16_t RemoteApi : 1;          // Bit 4
    uint16_t temperaturaTermo : 1;   // Bit 5
    uint16_t temperaturaTriac : 1;   // Bit 6
    uint16_t temperaturaCustom : 1;  // Bit 7
    uint16_t spare : 8;              // Bit 8 - 15
  };
} Error;

// Estructuras
typedef union {
  uint32_t data;
  struct {
    uint32_t wifi : 1;               // Bit 0
    uint32_t dhcp : 1;               // Bit 1
    uint32_t mqtt : 1;               // Bit 2
    uint32_t pwmEnabled: 1;          // Bit 3
    uint32_t pwmMan : 1;             // Bit 4
    uint32_t oledPower : 1;          // Bit 5
    uint32_t oledAutoOff : 1;        // Bit 6
    uint32_t potManPwmActive : 1;    // Bit 7
    uint32_t serial : 1;             // Bit 8
    uint32_t debug1 : 1;             // Bit 9
    uint32_t weblog : 1;             // Bit 10
    uint32_t timerEnabled : 1;       // Bit 11
    uint32_t debug2 : 1;             // Bit 12
    uint32_t sensorTemperatura : 1;  // Bit 13
    uint32_t alexaControl : 1;       // Bit 14
    uint32_t domoticz : 1;           // Bit 15
    uint32_t dimmerLowCost : 1;      // Bit 16
    uint32_t changeGridSign : 1;     // Bit 17
    uint32_t debug3 : 1;             // Bit 18
    uint32_t debug4 : 1;             // Bit 19
    uint32_t flipScreen : 1;         // Bit 20
    uint32_t offGrid : 1;            // Bit 21
    uint32_t showEnergyMeter : 1;    // Bit 22
    uint32_t offgridVoltage : 1;     // Bit 23
    uint32_t debug5 : 1;             // Bit 24
    uint32_t useClamp : 1;           // Bit 25
    uint32_t useSolarAsMPTT : 1;     // Bit 26
    uint32_t useBMV : 1;             // Bit 27
    uint32_t useExternalMeter : 1;   // Bit 28
    uint32_t spare : 2;              // Bit 29 - 30
    uint32_t debugPID : 1;           // Bit 31
  };
} SysBitfield;

typedef union {
  uint32_t data;
  struct {
    uint32_t R01Man : 1;   // Bit 0
    uint32_t R02Man : 1;   // Bit 1
    uint32_t R03Man : 1;   // Bit 2
    uint32_t R04Man : 1;   // Bit 3
    uint32_t spare : 28;   // Bit 4 - 31
  };
} RelayFlags;

union {
  uint32_t data;
  struct {
    uint32_t energyTotal : 1;       // Bit 0
    uint32_t voltage : 1;           // Bit 1
    uint32_t current : 1;           // Bit 2
    uint32_t activePower : 1;       // Bit 3
    uint32_t aparentPower : 1;      // Bit 4
    uint32_t reactivePower : 1;     // Bit 5
    uint32_t powerFactor : 1;       // Bit 6
    uint32_t frequency : 1;         // Bit 7
    uint32_t importActive : 1;      // Bit 8
    uint32_t exportActive : 1;      // Bit 9
    uint32_t importReactive : 1;    // Bit 10
    uint32_t exportReactive : 1;    // Bit 11
    uint32_t phaseAngle : 1;        // Bit 12
    uint32_t pv1c : 1;              // Bit 13
    uint32_t pv2c : 1;              // Bit 14
    uint32_t pv1v : 1;              // Bit 15
    uint32_t pv2v : 1;              // Bit 16
    uint32_t pw1 : 1;               // Bit 17
    uint32_t pw2 : 1;               // Bit 18
    uint32_t gridv : 1;             // Bit 19
    uint32_t wsolar : 1;            // Bit 20
    uint32_t wtoday : 1;            // Bit 21
    uint32_t wgrid : 1;             // Bit 22
    uint32_t wtogrid : 1;           // Bit 23
    uint32_t temperature : 1;       // Bit 24
    uint32_t batteryWatts : 1;      // Bit 25
    uint32_t batterySoC : 1;        // Bit 26
    uint32_t loadWatts : 1;         // Bit 27
    uint32_t spare : 4;             // Bit 28 - 31
  };
} webMonitorFields;

struct CONFIG
{
  byte eeinit;
  uint8_t wversion;

  // STATIC IP
  char ip[16];
  char gw[16];
  char mask[16];
  char dns1[16];
  char dns2[16];

  // SALIDAS RELÉS
  int16_t potTarget;
  int16_t free16_1;
 
  uint16_t R01Min;
  int16_t R01PotOn;
  int16_t R01PotOff;

  uint16_t R02Min;
  int16_t R02PotOn;
  int16_t R02PotOff;

  uint16_t R03Min;
  int16_t R03PotOn;
  int16_t R03PotOff;

  uint16_t R04Min;
  int16_t R04PotOn;
  int16_t R04PotOff;
  RelayFlags relaysFlags; // Guarda el estado de los relés
  
  // DATOS SOLAX V2
  char ssid_esp01[30];
  char password_esp01[30];
  
  // ORIGEN DATOS
  char sensor_ip[30];

  // MQTT
  char MQTT_broker[25];
  char MQTT_user[20];
  char MQTT_password[20];
  uint16_t MQTT_port;
  char R01_mqtt[50];
  char R02_mqtt[50];
  char R03_mqtt[50];
  char R04_mqtt[50];
  char password[30];
  char Solax_mqtt[50];
  char Meter_mqtt[50];
  unsigned long publishMqtt;

  // WIFI
  char ssid1[30];
  char pass1[30];
  char ssid2[30];
  char pass2[30];
  char hostServer[12];

  // PANTALLA
  uint8_t oledBrightness;

  // TEMPORIZADORES
  unsigned long oledControlTime;
  unsigned long freeTemp;
  unsigned long maxErrorTime;
  unsigned long getDataTime;

  // PWM
  uint8_t manualControlPWM;
  uint8_t autoControlPWM;
  uint16_t pwmFrequency;
  uint16_t potManPwm;
  
  // METER
  uint16_t baudiosMeter;
  uint8_t idMeter;

  // PROGRAMADOR
  uint16_t timerStart;
  uint16_t timerStop;

  // CONTROL TEMPERATURA
  uint8_t temperaturaEncendido;
  uint8_t temperaturaApagado;
  uint8_t modoTemperatura;
  uint8_t termoSensorAddress[8];
  uint8_t triacSensorAddress[8];
  char nombreSensor[30];
  uint8_t customSensorAddress[8];

  // PWM ACTIVACIÓN ESCLAVO
  uint8_t pwmSlaveOn;
  
  // BANDERAS SISTEMA
  SysBitfield flags;
  
  uint16_t domoticzIdx[3];
  uint16_t attachedLoadWatts;
  uint16_t maxPwmLowCost;

  // CONTROL DE CONSUMO Y VERTIDO
  float KwToday;
  float KwExportToday;
  float KwYesterday;
  float KwExportYesterday;
  float KwTotal;
  float KwExportTotal;

  // OFFGRID
  uint8_t soc;
  int16_t battWatts;

  // TIMEZONE
  // Use https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv to get the correct tz config.
  char tzConfig[30];

  // User language
  char language[5];
  
  // Maximum watts in user tariff
  uint16_t maxWattsTariff;

  // MQTT SoC Topic ICC Solar
  char SoC_mqtt[30];
  float batteryVoltage;

  // NTP Server
  char ntpServer[30];

  // Calibration Value
  float clampCalibration;

  // Clamp Voltage
  float clampVoltage;
  
  // Store PIDValues
  float PIDValues[3];

  // Battery Offset
  float voltageOffset;

  // Phase to be used
  uint8_t gridPhase;

  // Solax Version
  uint8_t solaxVersion;

  // FREE MEMORY
  uint8_t free[1042];
} config;

struct METER
{
  float energyTotal = 0;
  float voltage = 0;
  float current = 0;
  float activePower = 0;
  float aparentPower = 0;
  float reactivePower = 0;
  float powerFactor = 0;
  float frequency = 0;
  float importActive = 0;
  float exportActive = 0;
  float importReactive = 0;
  float exportReactive = 0;
  float phaseAngle = 0;

  uint8_t read_state = 0;
  uint8_t send_retry = 5;
} meter;

struct INVERTER
{
  float pv1c;              // Corriente string 1
  float pv2c;              // Corriente string 2
  float pv1v;              // Tension string 1
  float pv2v;              // Tension string 2
  float pw1;               // Potencia string 1
  float pw2;               // Potencia string 2
  float gridv;             // Tension de red
  float wsolar;            // Potencia solar
  float wtoday;            // Potencia solar diaria
  float wgrid;             // Potencia de red (Negativo: de red - Positivo: a red)
  float wtogrid;           // Potencia enviada a red
  float wgrid_control = 0; // Control del valor de wgrid
  float temperature;       // Temperatura del Inversor
  float batteryWatts = 0;  // Consumo de las Baterías 
  float batterySoC;
  float loadWatts = 0;
  float currentCalcWatts = 0;
  float acIn = 0;
  float acOut = 0;
} inverter;

struct UPTIME
{
  long Day = 0;
  int Hour = 0;
  int Minute = 0;
  int Second = 0;
  int HighMillis = 0;
  int Rollover = 0;
} uptime;

struct
{
  char _GRID_[8];
  char _SOLAR_[8];
  char _BATTERY_[10];
  char _INVERTERINFO_[18];
  char _METERINFO_[18];
  char _OLEDPOWER_[12];
  char _VOLTAGE_[12];
  char _CURRENT_[12];
  char _IMPORT_[12];
  char _EXPORT_[12];
  char _OLEDTODAY_[8];
  char _START_[16];
  char _CONNECTING_[16];
  char _RELAY_[12];
  char _CONNECTSSID_[22];
  char _CONFIGPAGE_[18];
  char _PRGRESTORE_[32];
  char _WAIT_[32];
  char _UPDATING_[32];
  char _LOSTWIFI_[32];
  char _TEMPERATURES_[22];
  char _INVERTERTEMP_[22];
  char _TERMOTEMP_[22];
  char _TRIACTEMP_[22];
  char _DERIVADOR_[32];
  char _COMPILATION_[22];
} lang;

struct tm timeinfo;

#define LOGGINGSIZE 20 //30
struct LOGGING
{
  char loggingMessage[LOGGINGSIZE][1024];
  int logcount = 0;
} logMessage;

uint8_t webMessageResponse = 0;
boolean processData = false;

String scanNetworks[15];
int32_t rssiNetworks[15];
uint8_t scanDoneCounter = 0;

char jsonResponse[768];

// Definiciones Conexiones
WiFiMulti wifiMulti;

fauxmoESP fauxmo;
AsyncWebServer server(80);
AsyncEventSource events("/events");
AsyncEventSource webLogs("/weblog");
AsyncMqttClient mqttClient;

static esp32ModbusTCP *modbustcp = NULL;
IPAddress modbusIP;

HardwareSerial SerieEsp(2);   // RX, TX para esp-01
HardwareSerial SerieMeter(1); // RX, TX para los Meter rs485/modbus
DynamicJsonDocument root(4096); // 3072

TickerScheduler Tickers(7);

DNSServer dnsServer;

TimerHandle_t relayOnTimer;
TimerHandle_t relayOffTimer;
TimerHandle_t startTimer;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(DS18B20);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

struct TEMPERATURE_CONFIG
{
  uint8_t tempSensorAddress[15][8];
  float temperaturaTermo = -127.0;
  float temperaturaTriac = -127.0;
  float temperaturaCustom = -127.0;
} temperature;


// Energy Monitor (Functions from emonLib) https://github.com/openenergymonitor/EmonLib
#define ADC_BITS    12
#define ADC_COUNTS  (1<<ADC_BITS)

uint8_t inPinI;
double ICAL;
double sampleI;
double filteredI;
double offsetI;
double sqI, sumI, Irms;

// PID Declaration
float Setpoint, PIDInput, PIDOutput;

PID myPID(&PIDInput, &PIDOutput, &Setpoint, 0.05, 0.06, 0.03, PID::DIRECT); // Probando 0.10 0.07 0.04

// GoodWe UDP Config
WiFiUDP inverterUDP;
unsigned int localUdpPort = 8899;  // local port to listen on
uint8_t incomingPacket[512];  // buffer for incoming packets

//////////////////// CAPTIVE PORTAL ////////////////

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    return true;
  }
  
  void handleRequest(AsyncWebServerRequest *request) {
    
    if(request->url() == "/savedata"){
        strcpy(config.ssid1, request->urlDecode(request->arg("wifis")).c_str());
        strcpy(config.pass1, request->urlDecode(request->arg("password")).c_str());
        config.flags.wifi = true;

        EEPROM.put(0, config);
        EEPROM.commit();
        Serial.print("DATA SAVED!!!!, RESTARTING!!!!");
        request->redirect("/");
        delay(500); // Only to be able to redirect client
        ESP.restart();

    } else {
      
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      response->print("<!DOCTYPE html><html><head><meta http-equiv='Content-Type' content='text/html; charset=UTF-8'><title>FREEDS</title></head>");
      response->print("<body id='main_body'><div id='form_container'><h1><a>FREEDS</a></h1>");
      response->print("<form id='captive' method='get' action='/savedata'><div class='form_description'><p>1º Seleccione una SSID</p><p>2º Indroduzca el password</p>");
      response->print("<p>3º Pulse Guardar</p><p>4º Espere a que reinicie y acceda a http://" + String(config.hostServer) + ".local/ para una configuración más avanzada.</p></div><ul>");
      response->print("<label class='description' for='wifis'>SSID:</label>");
      response->print("<div><select id='wifis' name='wifis'>");
      response->print("<option disabled selected>Seleccione una red</option>");
      
      char tmp[80];
      for (int i = 0; i < 15; ++i) {
        if (scanNetworks[i] == "") { break; }
        sprintf(tmp,"%s (%d dBm)", scanNetworks[i].c_str(), rssiNetworks[i]);
        response->print(" <option value='" + scanNetworks[i] + "'>" + String(tmp) + "</option>");
      }
      response->print("</select></div>");
      response->printf("<br><label class='description' for='password'>Password:</label>");
      response->printf("<div><input id='password' name='password' class='element text medium' type='text' maxlength='30' value=''/></div>");
      response->printf("<br><input id='saveForm' class='button_text' type='submit' name='submit' value='Guardar' /></ul>");
      response->printf("</form></div></body></html>");
      request->send(response);
    }
  }
};

// Declaration of default values
void shutdownPwm(boolean = false, const char* = "PWM: disabling PWM\n");

////////// WATCHDOG FUNCTIONS //////////

const int loopTimeCtl = 0;
hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule()
{
  INFOV("Reboot by Watchdog\n");
  ESP.restart();
}

////////// DEFAULT CONFIG //////////
void defaultValues()
{
  static char tmpTopic[33];

  byte mac[6];
  WiFi.macAddress(mac);
  sprintf(config.hostServer, "freeds_%02x%02x", mac[4], mac[5]);
  
  config.wversion = SOLAX_V2;
  config.flags.mqtt = false;
  strcpy(config.MQTT_broker, "192.168.0.2");
  strcpy(config.MQTT_user, "MQTT_user");
  strcpy(config.MQTT_password, "MQTT_password");
  config.MQTT_port = 1883;
  strcpy(config.ssid1, "MIWIFI1");
  strcpy(config.ssid2, "MIWIFI2");
  strcpy(config.pass1, "DSPLUSWIFI1");
  strcpy(config.pass2, "DSPLUSWIFI2");
  strcpy(config.ssid_esp01, "SOLAXX");
  strcpy(config.password_esp01, "");
  strcpy(config.sensor_ip, "IPORIGENDATOS");
  config.flags.dhcp = true;
  strcpy(config.ip, "192.168.0.99");
  strcpy(config.gw, "192.168.0.1");
  strcpy(config.mask, "255.255.255.0");
  strcpy(config.dns1, "8.8.8.8");
  strcpy(config.dns2, "1.1.1.1");
  config.potTarget = 60;
  config.flags.pwmEnabled = true;
  config.relaysFlags.R01Man = false;
  config.relaysFlags.R02Man = false;
  config.relaysFlags.R03Man = false;
  config.relaysFlags.R04Man = false;
  config.R01Min = 999;
  config.R01PotOn = 9999;
  config.R01PotOff = 9999;
  config.R02Min = 999;
  config.R02PotOn = 9999;
  config.R02PotOff = 9999;
  config.R03Min = 999;
  config.R03PotOn = 9999;
  config.R03PotOff = 9999;
  config.R04Min = 999;
  config.R04PotOn = 9999;
  config.R04PotOff = 9999;
  sprintf(tmpTopic, "%s/relay/1/STATUS", config.hostServer);
  strcpy(config.R01_mqtt, tmpTopic);
  sprintf(tmpTopic, "%s/relay/2/STATUS", config.hostServer);
  strcpy(config.R02_mqtt, tmpTopic);
  sprintf(tmpTopic, "%s/relay/3/STATUS", config.hostServer);
  strcpy(config.R03_mqtt, tmpTopic);
  sprintf(tmpTopic, "%s/relay/4/STATUS", config.hostServer);
  strcpy(config.R04_mqtt, tmpTopic);
  strcpy(config.Solax_mqtt, "solaxX1/tele/SENSOR");
  strcpy(config.Meter_mqtt, "meter/tele/SENSOR");
  strcpy(config.SoC_mqtt, "Inverter/BatterySOC");
  strcpy(config.password, "YWRtaW4=");
  config.flags.oledPower = true;
  config.flags.oledAutoOff = false;
  config.oledControlTime = 30000;
  config.getDataTime = 1500;
  config.maxErrorTime = 20000;
  config.manualControlPWM = 50;
  config.autoControlPWM = 60;
  config.flags.pwmMan = false;
  config.pwmFrequency = 30000;
  config.oledBrightness = 255;
  config.baudiosMeter = 9600;
  config.idMeter = 1;
  config.pwmSlaveOn = 0;
  config.potManPwm = 0;
  config.flags.potManPwmActive = false;
  config.flags.serial = true;
  config.flags.debug1 = false;
  config.flags.debug2 = false;
  config.flags.debug3 = false;
  config.flags.debug4 = false;
  config.flags.debug5 = false;
  config.flags.debugPID = false;
  config.flags.weblog = true;
  config.publishMqtt = 10000;
  config.flags.timerEnabled = false;
  config.timerStart = 500;
  config.timerStop = 700;
  config.flags.sensorTemperatura = false;
  config.temperaturaEncendido = 55;
  config.temperaturaApagado = 65;
  config.modoTemperatura = 0;
  memset(config.termoSensorAddress, 0, sizeof config.termoSensorAddress);
  memset(config.triacSensorAddress, 0, sizeof config.triacSensorAddress);
  memset(config.customSensorAddress, 0, sizeof config.customSensorAddress);
  strcpy(config.nombreSensor, "Temp. Ambiente");
  config.flags.alexaControl = false;
  config.flags.dimmerLowCost = false; 
  config.flags.domoticz = false;
  config.flags.changeGridSign = false;
  config.domoticzIdx[0] = 0;
  config.domoticzIdx[1] = 0;
  config.domoticzIdx[2] = 0;
  config.maxPwmLowCost = 1073; // Max 1232
  config.attachedLoadWatts = 2000;

  config.KwToday = 0;
  config.KwExportToday = 0;
  config.KwYesterday = 0;
  config.KwExportYesterday = 0;
  config.KwTotal = 0;
  config.KwExportTotal = 0;

  config.flags.flipScreen = true;
  config.flags.offGrid = false;
  config.soc = 100;
  config.battWatts = -60; // Sólo para ongrid
  config.flags.offgridVoltage = false;
  config.batteryVoltage = 51.0;
  config.voltageOffset = 0.30;
  config.maxWattsTariff = 3450;
  config.flags.showEnergyMeter = true;
  config.flags.useExternalMeter = false;
  config.flags.useClamp = false;
  config.clampCalibration = 22.3;
  config.clampVoltage = 230.0;
  config.PIDValues[0] = 0.05;
  config.PIDValues[1] = 0.06;
  config.PIDValues[2] = 0.03;
  config.gridPhase = 1;
  config.flags.useBMV = false;
  config.flags.useSolarAsMPTT = false;
  config.solaxVersion = 2;
  strcpy(config.tzConfig, "CET-1CEST,M3.5.0,M10.5.0/3");
  strcpy(config.ntpServer, "pool.ntp.org");
  strcpy(config.language, "es");

  config.eeinit = eepromVersion;
  config.flags.wifi = false;
}

void configureTickers(void)
{
  Tickers.add(0, 400,  [&](void *) { showOledData(); }, nullptr, true);                 // OLED loop
  Tickers.add(1, 500,  [&](void *) { every500ms(); }, nullptr, false);                  // 500ms functions loop
  Tickers.add(2, 5000, [&](void *) { connectToMqtt(); }, nullptr, false);               // Reconnect mqtt every 5 seconds
  Tickers.add(3, 5000, [&](void *) { connectToWifi(); }, nullptr, false);               // Reconnect Wifi  every 5 seconds
  Tickers.add(4, config.getDataTime, [&](void *) { getSensorData(); }, nullptr, false); // Sensor data adquisition time
  Tickers.add(5, config.publishMqtt, [&](void *) { publishMqtt(); }, nullptr, false);   // Publish Mqtt messages
  Tickers.add(6, 1000, [&](void *) { every1000ms(); }, nullptr, false);                 // 1000ms functions loop
  Tickers.disableAll();
}

void setup()
{
  ////////////////////// WATCHDOG //////////////////
  pinMode(loopTimeCtl, INPUT_PULLUP);
  delay(1000);
  timer = timerBegin(0, 240, true); // Timer 0, div 240
  timerAttachInterrupt(timer, &resetModule, true);
  timerAlarmWrite(timer, 30000000, false); // Set time in us (30 seconds)
  timerAlarmEnable(timer); // Enable interrupt
  /////////////////////////////////////////////////

  // ENERGY MONITOR
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
  
  Flags.data = 0; // All Bits to false
  Flags.RelayTurnOn = true;

  Serial.begin(115200); // Se inicia la UART0 para debug1
  Serial.setDebugOutput(true);
  
  verbosePrintResetReason(0);
  verbosePrintResetReason(1);
  
  // Configuramos las salidas
  pinMode(PIN_RL1, OUTPUT);
  pinMode(PIN_RL2, OUTPUT);
  pinMode(PIN_RL3, OUTPUT);
  pinMode(PIN_RL4, OUTPUT);
  digitalWrite(PIN_RL1, LOW);
  digitalWrite(PIN_RL2, LOW);
  digitalWrite(PIN_RL3, LOW);
  digitalWrite(PIN_RL4, LOW);

  //// Comprobación del estado de la configuración
  Serial.printf("Testing EEPROM Library\n");
  Serial.printf("EEPROM Size: %d bytes\n", sizeof(config));

  if (!EEPROM.begin(sizeof(config)))
  {
    Serial.printf("Failed to initialise EEPROM\nRestarting...\n");
    shutdownPwm(true);
    delay(1000);
    ESP.restart();
  }

  EEPROM.get(0, config);
  checkEEPROM();

  // Initialize SPIFFS
  if (!SPIFFS.begin(true, "/spiffs", 40))
  {
    INFOV("An Error has occurred while mounting SPIFFS\n");
    return;
  } else { readLanguages(); }

  // Initialize channels
  ledcAttachPin(pin_pwm, 2);
  ledcSetup(2, (double)config.pwmFrequency / 10.0, 10); // Frecuencia según configuración, 10-bit resolution
  ledcWrite(2, 0); // turn off

  dac_output_enable(DAC_CHANNEL_2); // Salida Analógica /// DAC_CHANNEL_1 -> PIN 25 /// DAC_CHANNEL_2 -> PIN 26

  // OLED
#ifdef OLED
  pinMode(16, OUTPUT);
  digitalWrite(16, LOW); // set GPIO16 low to reset OLED
  delay(50);
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high

  display.init();
  display.setBrightness(config.oledBrightness);
  if (config.flags.flipScreen) { display.flipScreenVertically(); }
  showLogo(lang._START_, true);
#endif

  if (config.eeinit != eepromVersion)
  {
    INFOV("Configuration Not Found, initializing\n");
    showLogo("Config Error", true);
    defaultValues();
    saveEEPROM();
  }

  current(ADC_INPUT, config.clampCalibration); // 2000 Turns / 62 Ohms burden resistor SCT-013 30A/1V

  // Si no está confgurada la wifi, creamos un Punto de acceso con el SSID FreeDS
  if (!config.flags.wifi)
  {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("FreeDS");
    IPAddress myIP = WiFi.softAPIP();
    INFOV("Local IP address: %s\n", myIP.toString().c_str());
    INFOV("SSID: FreeDS - 192.168.4.1\n");
    INFOV("Hostname: %s\n", config.hostServer);
    buildWifiArray();
    Flags.firstInit = true;
    dnsServer.start(53, "*", myIP);
  }
  else
  {
    // Relay Timers
    relayOnTimer = xTimerCreate("relayOnTimer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(enableRelay));
    relayOffTimer = xTimerCreate("relayOffTimer", pdMS_TO_TICKS(5000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(disableRelay));
    startTimer = xTimerCreate("startTimer", pdMS_TO_TICKS(45000), pdFALSE, (void *)0, reinterpret_cast<TimerCallbackFunction_t>(bootTimer));

    WiFi.onEvent(WiFiEvent);

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setClientId(config.hostServer);
    mqttClient.setKeepAlive(30);
    mqttClient.setCredentials(config.MQTT_user, config.MQTT_password);
    mqttClient.setServer(config.MQTT_broker, config.MQTT_port);

    // Configuración de los tickers
    configureTickers();
    
    connectToWifi();

    if (wifiMulti.run() == WL_CONNECTED) // Si conecta continuamos
    {
      Error.ConexionWifi = false;

#ifdef OLED
      showLogo("IP: " + WiFi.localIP().toString(), true);
#endif
      
      buildWifiArray();

      // Init, Configure and get the ntp time
      // if (config.wversion != SOLAX_V2_LOCAL) {
      if (strcmp("5.8.8.8", config.sensor_ip) != 0) {
        configTzTime(config.tzConfig, config.ntpServer);
        updateLocalTime();
      }
      Flags.timerSet = false;
      Flags.pwmIsWorking = true;

      // SERIAL
      SerieMeter.begin(config.baudiosMeter, SERIAL_8N1, RX1, TX1); // UART1 para meter
      SerieEsp.begin(115200, SERIAL_8N1, pin_rx, pin_tx); // UART2 para ESP01
      
      if (config.wversion == SOLAX_V2) { SerieEsp.printf("SSID: %s\n", config.ssid_esp01); }

      if (config.wversion >= MODBUS_TCP && config.wversion <= (MODBUS_TCP + MODE_STEP - 1))
      {
        modbusIP.fromString((String)config.sensor_ip);

        if (config.wversion == SOLAREDGE) {
          modbustcp = new esp32ModbusTCP(modbusIP, 1502);
        } else { modbustcp = new esp32ModbusTCP(modbusIP, 502); }

        configModbusTcp();
      }

      INFOV("Welcome to FreeDS\n");
      INFOV("Hostname: %s\n", config.hostServer);

      // Use mdns for host name resolution
      if (!MDNS.begin(config.hostServer))
      {
        INFOV("Error setting up MDNS responder!\n");
      }
      else
      {
        INFOV("mDNS responder started\n");
      }

      // Inicialización de Temporizadores
      timers.ErrorVariacionDatos = millis();
      timers.ErrorRecepcionDatos = millis();
      timers.OledAutoOff = millis();
      timers.ErrorLecturaTemperatura[0] = millis() - config.maxErrorTime;
      timers.ErrorLecturaTemperatura[1] = millis() - config.maxErrorTime;
      timers.ErrorLecturaTemperatura[2] = millis() - config.maxErrorTime;
      timers.printDebug = millis();
      timers.KwToday = millis();

      if (config.flags.pwmEnabled && !config.flags.pwmMan)
      {
        slave.workingMode = 0;
      } // AUTO
      else if (config.flags.pwmEnabled && config.flags.pwmMan)
      {
        slave.workingMode = 1;
      } // MANUAL
      else
      {
        slave.workingMode = 2;
      } // OFF

      Error.RecepcionDatos = true;
      Error.VariacionDatos = true;
      Error.ConexionMqtt = true;
      Error.temperaturaTermo = true;
      sensors.begin();
      sensors.setResolution(9);
      buildSensorArray();

      setWebConfig();
      defineWebMonitorFields(config.wversion);
    }

    // Esperamos 45 segundos a que se estabilicen las lecturas de la pinza antes de mostrarlas en la web.
    xTimerStart(startTimer, 0);

    // PID Config
    myPID.SetSampleTime(1000);
    config.flags.dimmerLowCost ? myPID.SetOutputLimits(209, config.maxPwmLowCost) : myPID.SetOutputLimits(0, 1023);
    config.flags.pwmMan ? myPID.SetMode(PID::MANUAL) : myPID.SetMode(PID::AUTOMATIC);
    config.flags.changeGridSign ? myPID.SetControllerDirection(PID::DIRECT) : myPID.SetControllerDirection(PID::REVERSE);
    myPID.SetTunings(config.PIDValues[0], config.PIDValues[1], config.PIDValues[2], PID::P_ON_M);
    Setpoint = config.potTarget;

    // GOODWE UDP SETUP
    if (config.wversion == GOODWE) {
      inverterUDP.begin(localUdpPort);
      INFOV("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
    }
  }

  // HTTP server
  if (!config.flags.wifi) { server.addHandler(new CaptiveRequestHandler).setFilter(ON_AP_FILTER); server.begin();}
  
  if (Flags.firstInit) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(64, 0, lang._CONNECTSSID_);
    display.drawString(64, 20, "FreeDS");
    display.drawString(64, 40, lang._CONFIGPAGE_);
    display.display();
  }
  
  // DEBUG
  memset(config.free, 255, sizeof config.free);
}

long tme = millis();

void loop()
{
  // Serial.printf("\nLoop Time: %lu\n", millis() - tme);
  // tme = millis();
  
  //////// Watchdog ////////////
  timerWrite(timer, 0); // reset timer (feed watchdog)
  /////////////////////////////////////

  updateUptime();

  Flags.firstInit ? dnsServer.processNextRequest() : fauxmo.handle();

  if (config.flags.wifi && !Flags.firstInit)
  {
    Tickers.update(); // Actualiza todas los tareas Temporizadas

    if (processData) { processingData(); }
    
    changeScreen();
    
    if (config.wversion == GOODWE) { parseUDP(); }

    // PID Check Loop
    if (config.flags.pwmEnabled && !Error.VariacionDatos && !Error.RecepcionDatos && Flags.pwmIsWorking && myPID.GetMode() == PID::AUTOMATIC && myPID.Compute()) {
      pwm.targetPwm = pwm.invert_pwm = (uint16_t)PIDOutput;
      if (config.flags.dimmerLowCost && pwm.invert_pwm <= 210) { pwm.invert_pwm = 0; }
      writePwmValue(pwm.invert_pwm);
    }
    
    if (config.flags.sensorTemperatura) { checkTemperature(); }
    
    if (config.flags.debug4) { 
      if (millis() - timers.printDebug > 2000){
        INFOV("\n");
        INFOV("Error Recepción Datos: %s, Error Variación Datos: %s, Error Conexión Mqtt: %s\n", Error.RecepcionDatos ? "true" : "false", Error.VariacionDatos ? "true" : "false", Error.ConexionMqtt ? "true" : "false");
        INFOV("Timer Recepción Datos: %ld, Timer Variación Datos: %ld\n", millis() - timers.ErrorRecepcionDatos, millis() - timers.ErrorVariacionDatos);
        INFOV("Modo Manual: %d, Modo Manual Automático: %d, PwmIsWorking: %d, pwm.invert_pwm: %d, pwm.targetPwm: %d, battery: %.02f\n", config.flags.pwmMan, Flags.pwmManAuto, Flags.pwmIsWorking, pwm.invert_pwm, pwm.targetPwm, inverter.batteryWatts);
        INFOV("Rele 1: %s, Rele 2: %s, Rele 3: %s, Rele 4:%s\n", digitalRead(PIN_RL1) ? "ON" : "OFF", digitalRead(PIN_RL2) ? "ON" : "OFF", digitalRead(PIN_RL3) ? "ON" : "OFF", digitalRead(PIN_RL4) ? "ON" : "OFF");
        //INFOV("Error Temp 1: %ld, Error Temp 2: %ld, Error Temp 3: %ld", millis() - timers.ErrorLecturaTemperatura[0], millis() - timers.ErrorLecturaTemperatura[1], millis() - timers.ErrorLecturaTemperatura[2]);
        timers.printDebug = millis();
      }
    }
    
    // if (config.wversion != SOLAX_V2_LOCAL && Flags.ntpTime) {
    if (strcmp("5.8.8.8", config.sensor_ip) != 0 && Flags.ntpTime) {
      checkTimer();
      updateLocalTime();
    }
    
    if (Flags.setBrightness) {
      saveEEPROM();
      display.setBrightness((uint8_t)((config.oledBrightness * 255) / 100));
      display.resetDisplay();
      Flags.setBrightness = false;
    }

    if (webLogs.count() == 0) { Flags.weblogConnected = false; }

    if ((millis() - timers.OledAutoOff) > config.oledControlTime)
    {
      if (config.flags.oledAutoOff) {
        config.flags.oledPower = false;
        turnOffOled();
      }
    }

    if (config.flags.potManPwmActive) {
      if (inverter.wsolar < config.potManPwm && !Flags.pwmManAuto) {
        Flags.pwmManAuto = true;
      } 
      
      if (inverter.wsolar > config.potManPwm && Flags.pwmManAuto) {
        Flags.pwmManAuto = false;
      }
    }
  }
} // End loop
