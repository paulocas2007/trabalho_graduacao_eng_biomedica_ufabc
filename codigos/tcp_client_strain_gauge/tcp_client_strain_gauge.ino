/*
 *  Funcao: Cliente TCP/IP no ESP32 
 *  Autor: Paulo Cesar Menegon de Castro
 *  Data de criacao: 27.04.2025
 *  Data de modificacao: 30.05.2025
 *  Referencias: https://portal.vidadesilicio.com.br/comunicacao-wireless-esp-protocolo-tcp/
 *               https://www.geeksforgeeks.org/tcp-ip-model/ 
 *               https://randomnerdtutorials.com/esp32-access-point-ap-web-server/ 
 *               https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/            
 */

// Constantes
#define NETWORK "esp32_AP"          // Nome da rede do Access Point
#define PWD "12345678"              // Senha da rede
#define PORT 1972                   // Porta do servidor TCP/IP
#define LED 2                       // LED da placa
#define ADC_STRAIN_G 35             // Conversao AD da leitura do conversor AD
#define ADC_BATTERY 34              // Leitura da voltagem da bateria

// Bibliotecas
#include <WiFi.h>

// Instanciacao
IPAddress serverIP;
WiFiClient client;
bool connected = false;
int strain_gauge;

void tcp() {
  if (!connected) {
    if (client.connect(serverIP, PORT)) {
      Serial.print("Conectado ao Gateway IP = ");
      Serial.println(serverIP);
      connected = true;
    } else {
      Serial.print("Nao foi possivel conectar ao Gateway IP = ");
      Serial.println(serverIP);
      delay(500);
    }
  } else {
    while (client.available())
      Serial.write(client.read());
    while (Serial.available()) {
      char r = Serial.read();
      Serial.write(r);
      //client.write(r);
    }
  }
  //Leitura do sinal condicionado do strain gauge
  strain_gauge = analogRead(ADC_STRAIN_G);
  Serial.println(strain_gauge);
  client.println(strain_gauge);
  client.flush();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED,OUTPUT);          // LED como pino de saida
  digitalWrite(LED, HIGH);      // LED apagado
  Serial.println();
  Serial.println("*********************************************************************************************************");
  Serial.print("Conectando a rede: ");
  Serial.print(NETWORK);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(NETWORK, PWD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.write('.');
  }
  Serial.println(".\nConectado!");
  Serial.print("IP local = ");
  Serial.println(WiFi.localIP());
  serverIP = WiFi.gatewayIP();
  Serial.flush();
}

void loop() {
  tcp();
}
