/*
 * Código para o módulo de transmissão dos dados adquiridos pelos sensores atraves do LoRa
 * Placas: Arduino UNO ou Arduino NANO
 * COM: Geralmente COM6 ou COM7
*/

// Importacao das bibliotecas utilizadas nessa etapa do projeto
#include <DHT.h>
#include <LoRa.h>
#include <SPI.h>
#include <Wire.h>

// Definicao dos pinos para sensores e atuadores no microcontrolador
#define DHTPIN 5    // Pino D5 para conectar o sensor de T e H (DHT11 ou DHT22)
#define rele 7      // Pino D7 para conectar o rele
// Pinos para modulo de transmissao LoRa (Arduino UNO ou Nano)
#define ss_pin 13   // Pino D13 (Arduino UNO) ou D10 (Arduino Nano)
#define rst_pin 9   // Pino D9
#define dio0_pin 2  // Pino D2
const int SensorPin = A0;   // Pino A0 para sensor capacitivo de umidade do solo
// Pinos para o sensor de fluxo de agua
const int WF_SensorPin = 4; 
const int IWF_SensorPin = 0;  // Pino iterrupt que equivale ao pino 4

// Criar a instancia de classes (objetos)
DHT dht(DHTPIN, DHT11);   // Pode ser DHT11 tambem

// Transmissao utilizando o LoRa
String LoRaMessage = "";
#define HIGH_GAIN_LORA 20 // dBm
#define BAND 951E6  // Frequencia de operacao 915 MHz

// Variaveis auxiliares 
uint8_t count = 0;
unsigned int Wcount = 0;
const int Air_Value = 600;
const int Water_Value = 450;
const float FATOR_CALIBRACAO = 4.5;
uint8_t humidade_solo_valor = 0;
uint8_t humidade_solo = 0;
bool alerta = false;
bool water = false;
float fluxo = 0;
float volume = 0;
unsigned long before_time = 0;

// Funcao de inicializacao dos objetos
void setup(){
    Serial.begin(9600);
    dht.begin();
    pinMode(rele, OUTPUT);  // Define o estado do pino como saída
    pinMode(WF_SensorPin, INPUT_PULLUP);   // Estado do sensor de vazao com entrada em nivel logico alto
    Serial.println("LoRa Sender\n");
    LoRa.setPins(ss_pin, rst_pin, dio0_pin);
    if (!LoRa.begin(BAND)){
      Serial.println("Starting Lora failed!\n");
      delay(1000);
    }
    else{
        LoRa.setTxPower(HIGH_GAIN_LORA);
        Serial.println("LoRa correct initialization!\n");
    }
}

void loop(){
    // Leitura dos sensores
    humidade_solo_valor = analogRead(SensorPin); 
    humidade_solo = map(humidade_solo_valor, Air_Value, Water_Value, 0, 100);
    // A leitura do sensores DHT possuem atraso de no máximo 250 ms
    float humidade = dht.readHumidity();
    float temperatura = dht.readTemperature();

    // Verificacao dos dados
    if (isnan(humidade) || isnan(temperatura)){
        Serial.println("Failed to read from DHT sensor!");
        return;
    }
    // Apresentacao dos dados no Monitor Serial
    Serial.print("Humidade do solo: " + String(humidade_solo) + " %\n");
    Serial.print("Temperatura do ambiente: " + String(temperatura) + " °C\n");
    Serial.print("Humidade do ambiente: " + String(humidade) + " %\n");
    Serial.print("Enviando um total de " + String(count) + " pacotes\n");

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

    // Transmissao dos dados
    LoRaMessage = String(count) + "-" + String(humidade_solo) + "#" + String(temperatura) + "@" + String(humidade) + "/" + String(alerta) + "*" + 
                  String(water) + "+" + String(fluxo) + "." + String(volume);
  
    // Envio dos pacotes
    LoRa.beginPacket();
    LoRa.print(LoRaMessage);
    LoRa.endPacket();

    count++;

    delay(10000);
}

// Funcao para obter os dados do sensor de vazao
void water_flow_measure(){
    // Medicao a partir de contagem de pulsos/segundo
    if ((millis() - before_time)>1000){
        detachInterrupt(IWF_SensorPin);
        fluxo = ((1000.0 / (millis() - before_time)) * Wcount) / FATOR_CALIBRACAO;  // L/min
        volume += fluxo/60;
        Serial.print("Fluxo de: " + String(fluxo) + " L/min\n");
        Serial.print("Volume utilizado de: " + String(volume) + " L\n");
        Serial.println();

        Wcount = 0;
        before_time = millis();
        attachInterrupt(IWF_SensorPin, Wcount++, FALLING);
    }
}
