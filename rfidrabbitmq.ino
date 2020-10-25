#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <MFRC522.h>
#include <SPI.h>

#define SS_PIN D4
#define RST_PIN D3
#define LED_MERAH D8
#define LED_HIJAU D1
#define BUZZ D2

const char *ssid = "Di isi dengan SSID wifi";
const char *password = "di isi dengan password wifi";
const char *mqtt_server = "di isi dengan IP server MQTT";
const char *mqtt_user = "di isi dengan user MQTT";
const char *mqtt_password = "di isi dengan password MQTT";

char pesanPUB[256];
String pesanSUB;
String tag;

MFRC522 RFID(SS_PIN, RST_PIN);

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

void setup()
{
  SPI.begin();
  RFID.PCD_Init();
  pinMode(LED_HIJAU, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(BUZZ, OUTPUT);
  digitalWrite(BUZZ, LOW);
  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(9600);
  Serial.setDebugOutput(true);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
}

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Sedang koneksi ke ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Berhasil terhubung ke WiFi");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("MAC Address ");
  Serial.println(WiFi.macAddress());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  pesanSUB ="";
  StaticJsonDocument<200> subjson;     
  Serial.print("Pesan masuk [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    pesanSUB += (char)payload[i];
  }
  Serial.println();
  deserializeJson(subjson, pesanSUB);
  String status_data = subjson["status"];
  String mac_addr = subjson["mac_address"];
  //Serial.println(statusabsen);
  
  if(mac_addr == WiFi.macAddress())
  {
    if(status_data == "F")
    {
      digitalWrite(LED_MERAH, HIGH);
      digitalWrite(BUZZ, HIGH);
      delay(200);
      digitalWrite(BUZZ, LOW);
      delay(200);
      digitalWrite(BUZZ, HIGH);
      delay(200);
      digitalWrite(BUZZ, LOW);
      delay(200);
      digitalWrite(LED_MERAH, LOW);
    }
    else
    {
      digitalWrite(LED_HIJAU, HIGH);
      digitalWrite(BUZZ, HIGH);
      delay(100);
      digitalWrite(LED_HIJAU, LOW);
      digitalWrite(BUZZ, LOW);
    }
  }
  Serial.println();
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Koneksi ke server MQTT ...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password))
    {
      Serial.println(" koneksi berhasil");
      //client.publish("rfid.out", "Halo SMK Kartika");
      client.subscribe("rfid.in");
    }
    else
    {
      Serial.print("gagal, rc=");
      Serial.print(client.state());
      Serial.println(" konek ulang dalam 5 detik");
      delay(5000);
    }
  }
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }

  client.loop();

  if (!RFID.PICC_IsNewCardPresent() || !RFID.PICC_ReadCardSerial())
  {
    return;
  }
  else
  {
    DynamicJsonDocument pubjson(2048);
    for (byte i = 0; i < RFID.uid.size; i++)
    {
      tag += String(RFID.uid.uidByte[i], HEX);
    }
    RFID.PICC_HaltA();
    RFID.PCD_StopCrypto1();
    pubjson["id_rfid"] = tag;
    pubjson["mac"] = WiFi.macAddress();
    serializeJson(pubjson, pesanPUB);
    ++value;
    snprintf(msg, 75, pesanPUB, value);
    Serial.print("Publish pesan: ");
    Serial.println(pesanPUB);
    client.publish("rfid.out", pesanPUB);
    digitalWrite(BUZZ, HIGH);
    digitalWrite(LED_HIJAU, HIGH);
    delay(100);
    digitalWrite(BUZZ, LOW);
    digitalWrite(LED_HIJAU, LOW);
    tag = "";
  }
}
