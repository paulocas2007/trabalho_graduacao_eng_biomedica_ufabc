/*
    Funcao: TCP/IP usando ESP32 como Access Point (AP) - Funciona como um servidor de rede
    Autor: Paulo Cesar Menegon de Castro
    Data de criacao: 27.04.2025
    Data de modificacao: 30.05.2025
    Referencias: https://portal.vidadesilicio.com.br/comunicacao-wireless-esp-protocolo-tcp/
                 https://www.geeksforgeeks.org/tcp-ip-model/
                 https://randomnerdtutorials.com/esp32-access-point-ap-web-server/
                 https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
*/

// Constantes
// Declaracao de constantes
#define SCREEN_WIDTH 128  // Largura do display em pixels
#define SCREEN_HEIGHT 64  // Altura do display em pixels
#define NETWORK "esp32_AP"          // Nome da rede do Access Point
#define PWD "12345678"              // Senha da rede
#define LOCAL_IP 192, 168, 171, 1   // Altera o IP local da rede (no AP o padrao e 192.168.4.1)
#define GATEWAY 192, 168, 171, 1
#define SUBNET 255, 255, 255, 0
#define PORT 1972                   // Porta do servidor TCP/IP
#define LED 2                       // LED da placa

// Bibliotecas
#include <Wire.h>              // Comunicacao I2C
#include <Adafruit_GFX.h>      // Controle do display OLED
#include <Adafruit_SSD1306.h>  // Controle do display OLED
#include <WiFiServer.h>
#include <WiFi.h>

// Instanciacao
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); // Display SSD1306 conectado ao I2C (SDA, SCL pins)
WiFiServer server(PORT);  // Cria o objeto servidor na porta designada
WiFiClient client;        // Cria o objeto cliente.
hw_timer_t* timer0 = NULL;

bool intTimer0 = false;  // Flag de interrupcaodo Timer0
String req = "";

// Interrupcao do Timer0
void IRAM_ATTR rotTimer0() {  // Armazena na RAM que e mais rapida que a FLASH
  intTimer0 = true;
}

// Inicializacao do timer
void startTimer() {
  // Frequencia do timer em 1 MHz
  timer0 = timerBegin(1000000);  // Frequencia em Hz (maximo de 80MHz)
  // Funcao a ser chamada pela interrupcao do timer.
  timerAttachInterrupt(timer0, &rotTimer0);
  // Intervalo para chamada da funcao
  timerAlarm(timer0, 1000, true, 0);  // Chama a funcao a cada 1 milissegundo (tempo em us)
  /* Parametros:
     1 - Timer a ser afetado
     2 - Ajusta o alarme para chamar a funcao a cada valor especificado
     3 - Repete o alarme
     4 - Sem limites de valor para a contagem
  */
}

// Gerencia clientes e pacotes TCP
void tcp() {
  if (client.connected())  // Se algum cliente conectado ao servidor
  {
    if (client.available() > 0)  // Se dados foram enviados pelo cliente
    {
      req = "";
      while (client.available() > 0)  //Armazena cada Byte (letra/char) na String para formar a mensagem recebida.
      {
        char z = client.read();
        req += z;
      }
      //Mostra a mensagem recebida do cliente no Serial Monitor.
      //Serial.print("\nUm cliente enviou uma mensagem");
      //Serial.print("\n...IP do cliente: ");
      //Serial.print(cl.remoteIP());
      //Serial.print("\n...IP do servidor: ");
      //Serial.print(WiFi.softAPIP());
      //Serial.print("\n...Mensagem do cliente: " + req + "\n");
      //bps = bps + sizeof(req) * 8;

      //Envia uma resposta para o cliente
      //client.print("\nO servidor recebeu sua mensagem");
      //client.print("\n...Seu IP: ");
      //client.print(client.remoteIP());
      //client.print("\n...IP do Servidor: ");
      //client.print(WiFi.softAPIP());
      //client.print("\n...Sua mensagem: " + req + "\n");
      client.println(req);
      client.flush();
    }
  } else  //  Se nao houver cliente conectado
  {
    client = server.available();  // Disponibiliza o servidor para o cliente se conectar
    delay(1);
  }
}

void setup() {
  Serial.begin(115200);         // Habilita comunicacao serial
  pinMode(LED, OUTPUT);         // LED como pino de saida
  digitalWrite(LED, HIGH);      // LED apagado
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Endereco I2C do display = 0x3C
    Serial.println("Falha alocacao SSD1306");
    for (;;)
      ;
  } else {
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE, 0);
    display.setCursor(0, 25);
    display.println("TCP Server");
    display.display();
  }
  WiFi.mode(WIFI_AP);           // Define o WiFi como AP
  IPAddress localIP(LOCAL_IP);  // IP local
  IPAddress gateway(GATEWAY);   // Gateway
  IPAddress subnet(SUBNET);     // Mascara Subnet
  WiFi.softAPConfig(localIP, gateway, subnet);
  WiFi.softAP(NETWORK, PWD);  //Cria a rede AP
  Serial.println("*********************************************************************************************************");
  Serial.print("IP do servidor: ");
  Serial.println(WiFi.softAPIP());
  server.begin();  //Inicia o servidor TCP.
  startTimer();    // Inicia timers da ESP32
  delay(3000);
}

void loop() {
  tcp();
  if (intTimer0) {
    intTimer0 = false;
    Serial.print("Valor lido do strain gauge: ");
    Serial.println(req);
    display.clearDisplay();
    display.setCursor(0,0);
    display.println(req);
    display.display();
  }
}
