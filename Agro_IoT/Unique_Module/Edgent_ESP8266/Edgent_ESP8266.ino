/*
 * Código para controle e envio de dados utilizando apenas o ESP8266
 * Placas: ESP8266
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
#include <DHT.h>
#include <SPI.h>
#include <Wire.h>
#include "BlynkEdgent.h"

// Definicao dos pinos para sensores e atuadores no microcontrolador
#define DHTPIN 5    // Pino GPIO5 (D1) para conectar o sensor de T e H (DHT11 ou DHT22)
#define rele 7      // Pino D7 para conectar o rele
const int SensorPin = A0;   // Pino A0 para sensor capacitivo de umidade do solo
// Pinos para o sensor de fluxo de agua
const int WFSensorPin = 4; 
const int IWFSensorPin = 0;  // Pino interrupt que equivale ao pino 4

// Criar a instancia de classes (objetos)
DHT dht(DHTPIN, DHT22);   // Pode ser DHT11 tambem

// Variaveis auxiliares 
unsigned long count = 0;
uint16_t Wcount = 0;
const int Air_Value = 600;
const int Water_Value = 450;
const float FATOR_CALIBRACAO = 4.5;
uint8_t humidade_solo_valor = 0;
uint8_t humidade_solo = 0;
float temperatura, humidade;
bool alerta = false;
bool water = false;
float fluxo = 0;
float volume = 0;
unsigned long before_time = 0;

void setup(){
    Serial.begin(115200);
    dht.begin();
    pinMode(rele, OUTPUT);  // Define o estado do pino como saída
    pinMode(WFSensorPin, INPUT_PULLUP);   // Estado do sensor de vazao com entrada em nivel logico alto
    delay(100);
    BlynkEdgent.begin();
}

void loop() {
    // Execucao da comunicacao com dashboard
    BlynkEdgent.run();
    // Leitura dos sensores
    humidade_solo_valor = analogRead(SensorPin); 
    humidade_solo = map(humidade_solo_valor, Air_Value, Water_Value, 0, 100);
    // A leitura do sensores DHT possuem atraso de no máximo 250 ms
    humidade = dht.readHumidity();
    temperatura = dht.readTemperature();

    // Verificacao dos dados
    if (isnan(humidade) || isnan(temperatura)){
        Serial.println("Failed to read from DHT sensor!");
        return;
    }
    // Apresentacao dos dados no Monitor Serial
    Serial.print("Humidade do solo: " + String(humidade_solo)+ "\n");
    Serial.print("Temperatura do ambiente: " + String(temperatura) + " °C\n");
    Serial.print("Humidade do ambiente: " + String(humidade) + " %\n");

    // Avaliacao das condicoes de humidade e temperatura
    // Atribuir o valor limite em litros para a capacidade do tanque de agua utilizada para irrigacao
    if (volume == 2){
        Serial.println("Tanque de agua em estado critico!\n");
        digitalWrite(rele, HIGH);
        water = true;
    }
    else if ((humidade_solo >= 0 && humidade_solo < 60) || (humidade < 50 || humidade > 75) ){
        // Condicao de irrigacao necessaria para plantacao
        Serial.println("Notificacao para irrigacao automatica\n");
        // Ligar o motor para irrigacao automatica
        Serial.println("Motor DC foi ligado\n");
        digitalWrite(rele, LOW);
        water_flow_measure();
    }
    else if (temperatura < 15 || temperatura > 30){
        // Condicoes fora do limite para plantacoes
        Serial.println("Notificacao: temperatura fora do limite recomendado\n");
        alerta = true;
    }
    else{
        // Condicao de irrigacao desnecessaria para plantacao
        Serial.println("Notificacao para desligar a irrigacao automatica\n");
        // Desligar o motor para irrigacao automatica
        Serial.println("Motor DC foi desligado\n");
        digitalWrite(rele, HIGH);
        alerta = false;
    }
    dashboard_communication();
    count++;
    delay(10000);
   
}

void IRAM_ATTR pulseCounter(){
    Wcount++;
}

// Funcao para obter os dados do sensor de vazao
void water_flow_measure(){
    // Medicao a partir de contagem de pulsos/segundo
    if ((millis() - before_time)>1000){
        detachInterrupt(IWFSensorPin);
        fluxo = ((1000.0 / (millis() - before_time)) * Wcount) / FATOR_CALIBRACAO;  // L/min
        volume += fluxo/60;
        Serial.print("Fluxo de: " + String(fluxo) + " L/min\n");
        Serial.print("Volume utilizado de: " + String(volume) + " L\n");
        Serial.println();

        Wcount = 0;
        before_time = millis();
        attachInterrupt(IWFSensorPin, pulseCounter, FALLING);
    }
}

void dashboard_communication(){
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
    if (alerta == true || water == true){
        Serial.println("Notificacao: temperatura fora do limite recomendado ou falta de agua no tanque\n");
        Blynk.logEvent("preocupacao", "As condicões de temperatura sao críticas ou falta agua no tanque, o sistema deve estar sendo prejudicado!");
        delay(1000);
    }
    else if ((humidade_solo >= 0 && humidade_solo < 60) || (humidade < 50 || humidade > 75)){
         // Condicao de irrigacao necessaria para plantacao
         Serial.println("Notificacao para irrigacao automatica\n");
         // Notificacao no dashboard
         Blynk.logEvent("irrigacao_automatica", "Sistema em estado crítico, modo de irrigacao automatica será ligado");
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
