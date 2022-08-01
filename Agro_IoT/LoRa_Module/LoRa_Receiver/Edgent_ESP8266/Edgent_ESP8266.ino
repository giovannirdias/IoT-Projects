/*
 * Código para o módulo de recepcao dos dados adquiridos pelos sensores atraves do LoRa
 * Placas: ESP8266 ou ESP32
 * COM: Geralmente COM6 ou COM7
*/

// Descricao do dashboard Blynk
#define BLYNK_TEMPLATE_ID "TMPLcasVJahy"  // ID de Usuario
#define BLYNK_DEVICE_NAME "IoT Agro"   // Nome do Dispositivo
#define BLYNK_FIRMWARE_VERSION  "1.1.0"   // Versao do FIRMWARE
#define BLYNK_PRINT Serial
#define BLYNK_DEBUG
#define APP_DEBUG
// Definir o tipo de board (Settings.h)
//#define USE_SPARKFUN_BLYNK_BOARD
#define USE_NODE_MCU_BOARD // (ESP 8266 e ESP32)
//#define USE_WITTY_CLOUD_BOARD
//#define USE_WEMOS_D1_MINI

// Importar as bibliotecasS
#include <Adafruit_Sensor.h>
#include <SPI.h>
#include <LoRa.h>
#include "BlynkEdgent.h"

// Pinos para modulo de recepcao LoRa
#define ss_pin  10    // Pino 
#define rst_pin 5     // Pino 
#define dio0_pin 3    // Pino

// Recepcao utilizando o LoRa
#define HIGH_GAIN_LORA 20 // dBm
#define BAND 951E6  // Frequencia de operacao 915 MHz

// Definir as instancias (objetos) dos dados recebidos da mensagem da estacao de trasmissao
String count;
String humidade_solo;
String temperatura;
String humidade;
String alerta;
String water_warning;
String fluxo;
String volume;

// Funcao de inicializacao dos objetos
void setup(){
    Serial.begin(9600);
    while(!Serial);
    Serial.println("LoRa Receiver\n");
    LoRa.setPins(ss_pin, rst_pin, dio0_pin);
    if (!LoRa.begin(BAND)){
      Serial.println("Starting Lora failed!\n");
      delay(1000);
    }
    else{
        LoRa.setTxPower(HIGH_GAIN_LORA);
        Serial.println("LoRa correct initialization!\n");
    }
    delay(100);
    BlynkEdgent.begin();  //Inicializar a comunicacao com dashboard
}

void loop(){
    // Execucao da comunicacao com dashboard
    BlynkEdgent.run();
    // Verificar as portas dos pacotes recebidos
    uint8_t porta1, porta2, porta3, porta4, porta5, porta6, porta7;
    uint8_t packetSize = LoRa.parsePacket();
    if (packetSize){
        // Pacote nao vazio, prepar para receber os dados
        String LoRaData = LoRa.readString();
        Serial.print("Pacotes recebido: " + LoRaData + "\n");

        // Criar as variaveis auxiliares contendo posicao das substrings da mensagem do modulo transmissor
        porta1 = LoRaData.indexOf('-');
        porta2 = LoRaData.indexOf('#');
        porta3 = LoRaData.indexOf('@');
        porta4 = LoRaData.indexOf('/');
        porta5 = LoRaData.indexOf('*');
        porta6 = LoRaData.indexOf('+');
        porta7 = LoRaData.indexOf('.');

        // Alocacao dos parametros em suas respectivas variaveis
        count = LoRaData.substring(0,porta1);
        humidade_solo = LoRaData.substring(porta1+1, porta2);
        temperatura = LoRaData.substring(porta2+1, porta3);
        humidade = LoRaData.substring(porta3+1, porta4);
        alerta = LoRaData.substring(porta4+1, porta5);
        water_warning = LoRaData.substring(porta5+1, porta6);
        fluxo = LoRaData.substring(porta6+1, porta7);
        volume = LoRaData.substring(porta7+1, LoRaData.length());

        // Estabelecer comunicacao com o dashboard na nuvem
        Blynk.virtualWrite(V1, humidade_solo);
        Blynk.virtualWrite(V2, temperatura);
        Blynk.virtualWrite(V3, humidade);
        Blynk.virtualWrite(V4, fluxo);
        Blynk.virtualWrite(V5, volume);
        
        // Apresentar dados recebidos no monitor Serial para controle de processos
        Serial.print("Pacote n°: " + String(count) + "\n");
        Serial.print("Humidade do solo = " + String(humidade_solo) + " %\n");
        Serial.print("Temperatura ambiente = " + String(temperatura) + " °C\n");
        Serial.print("Humidade do ambiente = " + String(humidade) + " %\n");
        Serial.print("Fluxo de agua no sensor = " + String(fluxo) + " L/min\n");
        Serial.print("Volume de agua utilizado = " + String(volume) + " L\n");
        Serial.println();

        // Apresentacao das condicoes: estado critico, irrigacao automatica ou falta de recursos
        if (alerta == "true" || water_warning == "true"){
            Serial.println("Notificacao: temperatura fora do limite recomendado ou falta de agua no tanque\n");
            Blynk.logEvent("Preocupacao", "As condicões de temperatura sao críticas ou falta agua no tanque, o sistema deve estar sendo prejudicado!");
            delay(1000);
        }
        else if ((humidade_solo.toInt() >= 0 && humidade_solo.toInt() < 60) || (humidade.toInt() < 50 || humidade.toInt() > 75)){
             // Condicao de irrigacao necessaria para plantacao
             Serial.println("Notificacao para irrigacao automatica\n");
             // Notificacao no dashboard
             Blynk.logEvent("Irrigacao_Automatica", "Sistema em estado crítico, modo de irrigacao automatica será ligado");
             Blynk.virtualWrite(V6, 255);  // Motor no estado alto
             delay(1000);
        }
         else{
             // Condicao de irrigacao desnecessaria para plantacao
             Serial.println("Condicoes excelente ou suficientes para o sistema\n");
             Blynk.virtualWrite(V6, 0); // Motor no estado baixo 
             delay(1000);         
         }
    }
    else{
        Serial.println("Pacotes estao vazios, nenhuma informacao foi recebida!\n");
        Blynk.logEvent("Comunicacao", "Não foi recebido informacoes, problemas com a transmissao ou recepcao dos dados");
    }
}
