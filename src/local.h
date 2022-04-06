const char *wifi_ssid =  "xxxxxx";
const char *wifi_pass =  "******";
const char *mqtt_pass =  "yyyyyy";
const char *mqtt_user =  "zzzzzz";      
const char *mqtt_ip   =  "192.168.178.3";    
const char *mqtt_client=   "vvvvvvv";
const int mqtt_host_ip[4] = {
    192,168,178,3
};

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0x16
};

// Set your Static IP address
IPAddress ip(192, 168, 178, 4);
// Set your Gateway IP address
IPAddress gateway(192, 168, 178, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(192, 168, 178, 1);   //optional
IPAddress server(192, 168, 178, 3);
