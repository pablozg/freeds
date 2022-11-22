// Mode Definitions

#define MODE_WIDTH 80
#define MODE_STEP 20
#define MODBUS_RTU 1
#define HTTP_API 21
#define MQTT_MODE 41
#define MODBUS_TCP 61

// MODBUS_RTU 1 - 20
#define DDS238_METER 1
#define DDSU666_METER 2  
#define SDM_METER 3
#define MUSTSOLAR 4

// HTTP API 21 - 40
#define SOLAX_V2 21
#define SOLAX_V2_LOCAL 22
#define SOLAX_V1 23
#define WIBEEE 24
#define SHELLY_EM 25
#define FRONIUS_API 26
#define SLAVE_MODE 27
#define GOODWE 28

// MQTT_MODE 41 - 60
#define MQTT_BROKER 41
#define ICC_SOLAR 42

// MODBUS_TCP 61 - 80
#define SMA_BOY 61
#define VICTRON 62
#define FRONIUS_MODBUS 63
#define HUAWEI_MODBUS 64
#define SMA_ISLAND 65
#define SCHNEIDER 66
#define WIBEEE_MODBUS 67
#define INGETEAM 68
#define SOLAREDGE 80 // Must be always the last mode because it use the 1502 port (I will add modbus port selection soon).