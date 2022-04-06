#include <Oregon_NR.h>
#include <stdlib.h>
#//include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "local.h"
#include <SPI.h>
#include <Ethernet3.h>


//# define ESP8266
#if defined ( ESP8266 ) || ( ESP32 )// Для Wemos
  Oregon_NR oregon(4, 4,          // приёмник на выводе D7 (GPIO13) 13 now
                        2, true,    // Светодиод на D2 подтянут к +пит(true). Если светодиод не нужен, то номер вывода - 255
                        50, true);  // Буфер на приём посылки из 50 ниблов, включена сборка пакетов для v2

#else                               // Для AVR
  Oregon_NR oregon(2, 0,            // Приёмник на выводе D2, Прерывание 0,
                     4, false);    // Светодиод приёма на вывод 13
                                    // По умолчанию Буфер на приём посылки стандартный - на 24 нибла, включена сборка пакетов для v2

//Oregon_NR oregon(2, 0); 	        // Конструктор по умолчанию, со стандартным буфером и без светодиода
#endif
#  define SERIAL_BAUD 115200

#define ETHERNET_USE_ESP8266
//WiFiClient wclient;
// PubSubClient client(wclient, mqtt_server);
//PubSubClient client(wclient);

EthernetClient ethClient;
// read IP Addresses from local.h, bire IPADDRESS was defined here


PubSubClient client(ethClient);

byte    quadrant  = 0;     //used to look up 16 positions around the compass rose
double  avWindspeed = 0.0;
double  gustWindspeed = 0.0; //now used for general anemometer readings rather than avWinspeed
float   rainTotal = 0.0;
float   rainRate  = 0.0;
double  temperature = 0.0;
byte     humidity  = 0;
const char windDir[16][4] = {
  "N  ", "NNE", "NE ", "ENE",  "E  ", "ESE", "SE ", "SSE",  "S  ", "SSW", "SW ", "WSW",  "W  ", "WNW", "NW ", "NNW"
};


void setup() {
   //Serial.begin(115200);
   Serial.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY);
   Serial.println("huhu - Fresh tart");

   ReconnectEth();
   Serial.println("First connection done");
   if (oregon.no_memory)
   {
    Serial.println("NO MEMORY");   //Не хватило памяти
    do yield();
    while(true);
   }

  //вкючение прослушивания радиоканала
  oregon.start();
  oregon.receiver_dump = 0;       //true - Включает "осциллограф" - отображение данных, полученных с приёмника
}


void send_measurement(const char* sensor_id, double value) {
    if (!client.connected()) {
      //client.setServer(mqtt_ip,1883);

      // Reconnecct ehternet and mqtt_pass
        ReconnectEth();
      //client.connect(mqtt_client,mqtt_user,mqtt_pass);
      //delay(200);
    }
    if (client.connected()) {
      //Serial.println("ok connected mqtt");
      char val_buf[sizeof("-XXX.XX")];
      dtostrf(value, 7, 2, val_buf);
      Serial.print("mqtt pub done \t");
      client.publish(sensor_id, val_buf);
      Serial.println(sensor_id);
      //client.disconnect();
      //blink(3, 250);
    }  else {
      Serial.println("still not connected to mqtt, something went wront");
      //blink(4, 110);
    }
}

void ReconnectEth(){
  float timefloat;
  timefloat =  (float) millis() / 10;
  Serial.println("...");
  Ethernet.setRstPin(D1);
  Ethernet.setCsPin(D8);
  Serial.println(timefloat);
  Serial.println("\n1) Eth reconnecting was called ");
  //Serial.println("\nIP before");
  //Serial.println(Ethernet.localIP());
  Serial.println("\n2)Before ping mqtt server, result:");
  Serial.println(timefloat);
  uint8_t pingresult;
  pingresult = ethClient.connect(server, 80);
  Serial.println(pingresult);

  // Do initial ethernet hard reset
  Ethernet.hardreset();
  delay(1000);
  Ethernet.begin(mac, ip, subnet, gateway);
  delay(1000);

  Serial.println("\n3)IP after");
  Serial.println(Ethernet.localIP());

  Serial.println("\n4).Ping mqtt server, result:");

 for (int i = 0; i <= 9; i++)
 {
   if (i== 1){
     Ethernet.softreset();
     Serial.println("Soft reset done");
     delay(1000);
   }
   if (i== 4){
     Ethernet.hardreset();
     Serial.println("Hard reset done");
     delay(1000);
   }
   if (i== 8){
     Serial.println("Restarting ESP");
     ESP.restart();
   }
   if (i==2){
     Ethernet.begin(mac, ip, subnet, gateway);
     Serial.println("Ethernet begin done");
   }
   if (i==5){
     Ethernet.begin(mac, ip, subnet, gateway);
     Serial.println("Ethernet begin after hardrset done");
   }

   //pingresult = ethClient.connect(server, 80);
  // client.setServer("192.168.178.26",1883);

   client.setServer(mqtt_ip,1883);
   bool rc;

// overwrite timouts for mqtt, without that many reconnects happened before
   client.setKeepAlive(121);
   client.setSocketTimeout(122);

   //rc=client.connect("oscbridge2","oregon","gate3bt");//,"",0,false,"",true);
   rc=client.connect(mqtt_client,mqtt_user,mqtt_pass);

   // Debug output
   Serial.print(rc);
   Serial.print("\t ");
   Serial.print(client.state());
   Serial.print("\t ");
   client.clearWriteError();
   Serial.print(client.availableForWrite());
   Serial.print("\t ");
   pingresult=client.connected();
   Serial.print(pingresult);
   Serial.print("\t ");
   Serial.print(Ethernet.linkReport());
   Serial.print("\t ");
   Serial.print(Ethernet.duplexReport());
   Serial.print("\t ");
   Serial.print(Ethernet.speedReport());
   Serial.print("\t ");
   Serial.print(Ethernet.localIP());
   Serial.print("\n ");
   if (pingresult > 0) {
     break;
   }
   delay(1000);
 }
  Serial.println("\n5).");
}

void loop() {

  //////////////////////////////////////////////////////////////////////
  //Захват пакета,/////////////////////////////////////////////////////
  oregon.capture(0); // 1 - выводить в Serial сервисную информацию

  //Захваченные данные годны до следующего вызова capture
  //ОБработка полученного пакета//////////////////////////////////////////////
  if (oregon.captured)  {
      if (!client.connected()) {
          ReconnectEth();
      }
      else{
        client.loop();
      }
    //Вывод информации в Serial
    Serial.print ((float) millis() / 1000, 1); //Время
    Serial.print ("s\t");
    //Версия протокола
    if (oregon.ver == 2) Serial.print("v2  ");
    if (oregon.ver == 3) Serial.print("v3 ");

    //Информация о восстановлени пакета, Information zur PAketwiederherstellung
    if (oregon.restore_sign & 0x01) Serial.print("s"); //восстановлены одиночные такты, single error correction
    else  Serial.print(" ");
    if (oregon.restore_sign & 0x02) Serial.print("d"); //восстановлены двойные такты, double error correction
    else  Serial.print(" ");
    if (oregon.restore_sign & 0x04) Serial.print("p "); //исправленна ошибка при распознавании версии пакета, error corrected
    else  Serial.print("  ");
    if (oregon.restore_sign & 0x08) Serial.print("r "); //собран из двух пакетов (для режима сборки в v.2)
    else  Serial.print("  ");

    //Вывод полученного пакета.
    for (int q = 0;q < oregon.packet_length; q++)
      if (oregon.valid_p[q] == 0x0F) Serial.print(oregon.packet[q], HEX);
      else Serial.print("");

    //Время обработки пакета
    Serial.print(" \t");
    Serial.print(oregon.work_time);
    Serial.print("ms ");

    if ((oregon.sens_type == THGN132 ||
    (oregon.sens_type & 0x0FFF) == RTGN318 ||
    (oregon.sens_type & 0x0FFF) == RTHN318 ||
    oregon.sens_type == THGR810 ||
    oregon.sens_type == THN132 ||
    oregon.sens_type == THN800 ||
    oregon.sens_type == BTHGN129 ||
    oregon.sens_type == BTHR968 ||
    oregon.sens_type == THGN500) && oregon.crc_c){
      Serial.print(" ");

      uint8_t nrhumchl = 1;


      if (oregon.sens_type != BTHR968 && oregon.sens_type != THGN500)
      {
        Serial.print(" CHNL: ");
        Serial.print(oregon.sens_chnl);
        nrhumchl = oregon.sens_chnl;
        if (nrhumchl==1){
          send_measurement("oregonscientific/hum1/chnl", oregon.sens_chnl);
        }
        if (nrhumchl==2){
          send_measurement("oregonscientific/hum2/chnl", oregon.sens_chnl);
        }
        if (nrhumchl==3){
          send_measurement("oregonscientific/hum3/chnl", oregon.sens_chnl);
        }
      }
      else Serial.print(" ");

      if (oregon.sens_type == THGN132) Serial.print("THGN132N");
      if (oregon.sens_type == THGN500) Serial.print("THGN500 ");
      if (oregon.sens_type == THGR810) {
        send_measurement("oregonscientific/hum/THGR810", 1);
        Serial.print("THGR810 ");
      }
      if ((oregon.sens_type & 0x0FFF) == RTGN318) Serial.print("RTGN318 ");
      if ((oregon.sens_type & 0x0FFF) == RTHN318) Serial.print("RTHN318 ");
      if (oregon.sens_type == THN132 ) Serial.print("THN132N ");
      if (oregon.sens_type == THN800 ) Serial.print("THN800  ");
      if (oregon.sens_type == BTHGN129 ) Serial.print("BTHGN129");
      if (oregon.sens_type == BTHR968 ) Serial.print("BTHR968 ");


      Serial.print(" BAT: ");
      if (oregon.sens_battery) {
        Serial.print("F ");
        if (nrhumchl==1){
          send_measurement("oregonscientific/hum1/batterylow", 0);
        }
        if (nrhumchl==2){
          send_measurement("oregonscientific/hum2/batterylow", 0);
        }
        if (nrhumchl==3){
          send_measurement("oregonscientific/hum3/batterylow", 0);
        }
      } else {
        Serial.print("e ");
        if (nrhumchl==1){
          send_measurement("oregonscientific/hum1/batterylow", 1);
        }
        if (nrhumchl==2){
          send_measurement("oregonscientific/hum2/batterylow", 1);
        }
        if (nrhumchl==3){
          send_measurement("oregonscientific/hum3/batterylow", 1);
        }
      }
      Serial.print("ID: ");
      Serial.print(oregon.sens_id, HEX);

      if (oregon.sens_tmp >= 0 && oregon.sens_tmp < 10) Serial.print(" TMP:  ");
      if (oregon.sens_tmp < 0 && oregon.sens_tmp >-10) Serial.print(" TMP: ");
      if (oregon.sens_tmp <= -10) Serial.print(" TMP:");
      if (oregon.sens_tmp >= 10) Serial.print(" TMP: ");
      if (nrhumchl==1){
        send_measurement("oregonscientific/hum1/temp_c", oregon.sens_tmp);
      }
      if (nrhumchl==2){
        send_measurement("oregonscientific/hum2/temp_c", oregon.sens_tmp);
      }
      if (nrhumchl==3){
        send_measurement("oregonscientific/hum3/temp_c", oregon.sens_tmp);
      }
      Serial.print(oregon.sens_tmp, 1);
      Serial.print("C ");
      if (oregon.sens_type == THGN132 ||
          oregon.sens_type == THGR810 ||
          oregon.sens_type == BTHGN129 ||
          oregon.sens_type == BTHR968 ||
          (oregon.sens_type & 0x0FFF) == RTGN318 ||
          oregon.sens_type == THGN500 ) {
        Serial.print("HUM: ");
        Serial.print(oregon.sens_hmdty, 0);
        if (nrhumchl==1){
          send_measurement("oregonscientific/hum1/hum_perCent", oregon.sens_hmdty);
        }
        if (nrhumchl==2){
          send_measurement("oregonscientific/hum2/hum_perCent", oregon.sens_hmdty);
        }
        if (nrhumchl==3){
          send_measurement("oregonscientific/hum3/hum_perCent", oregon.sens_hmdty);
        }
        Serial.print("%");
      }
      else Serial.print(" ");

      if (oregon.sens_type == BTHGN129 ||  oregon.sens_type == BTHR968)
      {
      Serial.print(" PRESS: ");
      Serial.print(oregon.get_pressure(), 1);
      Serial.print("Hgmm ");
      }
    }

  if (oregon.sens_type == WGR800 && oregon.crc_c){
      Serial.print("\tWGR800  ");
      send_measurement("oregonscientific/wind/WGR800", 1);
      Serial.print(" ");
      Serial.print(" BAT: ");
      if (oregon.sens_battery) {
        Serial.print("F ");
        send_measurement("oregonscientific/wind/batterylow", 0);
      } else {
        Serial.print("e ");
        send_measurement("oregonscientific/wind/batterylow", 1);
      }
      Serial.print("ID: ");
      Serial.print(oregon.sens_id, HEX);
      send_measurement("oregonscientific/wind/sensorid", oregon.sens_id);

      Serial.print(" AVG: ");
      Serial.print(oregon.sens_avg_ws, 1);
      send_measurement("oregonscientific/wind/avg", oregon.sens_avg_ws);
      Serial.print("m/s  MAX: ");
      Serial.print(oregon.sens_max_ws, 1);
      Serial.print("m/s  DIR: "); //N = 0, E = 4, S = 8, W = 12
      send_measurement("oregonscientific/wind/max", oregon.sens_max_ws);
      send_measurement("oregonscientific/wind/direction", oregon.sens_wdir);

      switch (oregon.sens_wdir)
      {
      case 0: Serial.print("N"); break;
      case 1: Serial.print("NNE"); break;
      case 2: Serial.print("NE"); break;
      case 3: Serial.print("NEE"); break;
      case 4: Serial.print("E"); break;
      case 5: Serial.print("SEEE"); break;
      case 6: Serial.print("SE"); break;
      case 7: Serial.print("SSE"); break;
      case 8: Serial.print("S"); break;
      case 9: Serial.print("SSW"); break;
      case 10: Serial.print("SW"); break;
      case 11: Serial.print("SWW"); break;
      case 12: Serial.print("W"); break;
      case 13: Serial.print("NWW"); break;
      case 14: Serial.print("NW"); break;
      case 15: Serial.print("NNW"); break;
      }

    }

    if (oregon.sens_type == UVN800 && oregon.crc_c){
      Serial.print("\tUVN800  ");
      Serial.print(" ");
      Serial.print(" BAT: ");
      if (oregon.sens_battery) Serial.print("F "); else Serial.print("e ");
      Serial.print("ID: ");
      Serial.print(oregon.sens_id, HEX);

      Serial.print(" UV IDX: ");
      Serial.print(oregon.UV_index);

    }


    if (oregon.sens_type == RFCLOCK && oregon.crc_c){
      Serial.print("\tRF CLOCK");
      Serial.print(" CHNL: ");
      Serial.print(oregon.sens_chnl);
      Serial.print(" BAT: ");
      if (oregon.sens_battery) Serial.print("F "); else Serial.print("e ");
      Serial.print("ID: ");
      Serial.print(oregon.sens_id, HEX);
      Serial.print(" TIME: ");
      Serial.print(oregon.packet[6] & 0x0F, HEX);
      Serial.print(oregon.packet[6] & 0xF0 >> 4, HEX);
      Serial.print(':');
      Serial.print(oregon.packet[5] & 0x0F, HEX);
      Serial.print(oregon.packet[5] & 0xF0 >> 4, HEX);
      Serial.print(':');
      Serial.print(':');
      Serial.print(oregon.packet[4] & 0x0F, HEX);
      Serial.print(oregon.packet[4] & 0xF0 >> 4, HEX);
      Serial.print(" DATE: ");
      Serial.print(oregon.packet[7] & 0x0F, HEX);
      Serial.print(oregon.packet[7] & 0xF0 >> 4, HEX);
      Serial.print(".");
      if ((oregon.packet[8] & 0x0F) ==1 || (oregon.packet[8] & 0x0F) ==3) {
        Serial.print('1');
      }
      else {
        Serial.print('0');
      }
      Serial.print(oregon.packet[8] & 0xF0 >> 4, HEX);
      Serial.print('.');
      Serial.print(oregon.packet[9] & 0x0F, HEX);
      Serial.print(oregon.packet[9] & 0xF0 >> 4, HEX);

    }

    if (oregon.sens_type == PCR800 && oregon.crc_c){
      Serial.print("\tPCR800  ");
      send_measurement("oregonscientific/rain/PCR800", 1);
      Serial.print(" ");
      Serial.print(" BAT: ");
      if (oregon.sens_battery) {
        Serial.print("F ");
        send_measurement("oregonscientific/rain/batterylow", 0);
      } else {
        Serial.print("e ");
        send_measurement("oregonscientific/rain/batterylow", 1);
      }
      Serial.print("ID: ");
      Serial.print(oregon.sens_id, HEX);
      send_measurement("oregonscientific/rain/sensorid", oregon.sens_id);
      Serial.print("   TOTAL: ");
      Serial.print(oregon.get_total_rain(), 1);
      send_measurement("oregonscientific/rain/total_mm", oregon.get_total_rain());
      Serial.print("mm  RATE: ");
      Serial.print(oregon.get_rain_rate(), 1);
      send_measurement("oregonscientific/rain/rate_mm_h", oregon.get_rain_rate()/100.0);
      Serial.print("mm/h");
    }

#if ADD_SENS_SUPPORT == 1
      if ((oregon.sens_type & 0xFF00) == THP && oregon.crc_c) {
      Serial.print("\tTHP     ");
      Serial.print(" CHNL: ");
      Serial.print(oregon.sens_chnl);
      Serial.print(" BAT: ");
      Serial.print(oregon.sens_voltage, 2);
      Serial.print("V");
      if (oregon.sens_tmp > 0 && oregon.sens_tmp < 10) Serial.print(" TMP:  ");
      if (oregon.sens_tmp < 0 && oregon.sens_tmp > -10) Serial.print(" TMP: ");
      if (oregon.sens_tmp <= -10) Serial.print(" TMP:");
      if (oregon.sens_tmp >= 10) Serial.print(" TMP: ");
      Serial.print(oregon.sens_tmp, 1);
      Serial.print("C ");
      Serial.print("HUM: ");
      Serial.print(oregon.sens_hmdty, 1);
      Serial.print("% ");
      Serial.print("PRESS: ");
      Serial.print(oregon.sens_pressure, 1);
      Serial.print("Hgmm");

    }
#endif
    Serial.println();
  }

  yield();
}
