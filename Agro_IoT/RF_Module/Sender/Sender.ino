/*
 * Códido para módulo de transmissão dos dados adquiridos pelos sensores através do HC-12 Sender
 * Placas: Arduino UNO
 * COM: Geralmente COM6 e COM7
 */

// Importacao das bibliotecas utilizadas nessa etapa do projeto
#include <DHT.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <Wire.h>

// Definicao dos pinos para sensores e atuadores no microcontrolador
#define DHTPIN 5    // Pino D5 para conectar o sensor de T e H (DHT11)
#define rele 7      // Pino D7 para conectar o rele
const int SensorPin = A0;   // Pino A0 para sensor capacitivo de umidade do solo
// Pinos para o sensor de fluxo de agua
const int WF_SensorPin = 4; 
const int IWF_SensorPin = 0;  // Pino iterrupt que equivale ao pino 4

// Criar a instancia de classes (objetos)
DHT dht(DHTPIN, DHT11);   
SoftwareSerial HC12(10, 11); // HC-12 TX Pin, HC-12 RX Pin

// Transmissao utilizando o HC-12
String Message = "";

// Variaveis auxiliares 
// 1) Envio de infos
uint8_t count = 0;   // Contagem de pacotes enviados
// 2) Sensor de humidade do solo
// Parametros de limitacao do valores mensurados para o sensor de humidade do solo
const int Air_Value = 600;
const int Water_Value = 450;
uint8_t humidade_solo_valor = 0;
uint8_t humidade_solo = 0;
// 3) Sensor de vazao
unsigned int Wcount = 0;   // Contador de pulso do sensor de vazao
const float FATOR_CALIBRACAO = 4.5; // Fator de calibracao par ao sensor de vazao
float fluxo = 0;
float volume = 0;
unsigned long before_time = 0;
// 4) Warning variables
bool alerta = false;
bool water = false;

// Funcao de inicializacao dos objetos
void setup(){
    Serial.begin(9600);
    Serial.println("HC-12 Sender!\n");
    HC12.begin(9600);
    dht.begin();
    delay(100);
    pinMode(rele, OUTPUT);  // Define o estado do pino como saída
    pinMode(WF_SensorPin, INPUT_PULLUP);   // Estado do sensor de vazao com entrada em nivel logico alto
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
    Serial.print("Enviando um total de " + String(count) + " pacotes\n");
    Serial.print("Humidade do solo: " + String(humidade_solo) + " %\n");
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

    // Transmissao dos dados
    Message = String(count) + "-" + String(humidade_solo) + "#" + String(temperatura) + "@" + String(humidade) + "/" + String(alerta) + "*" + 
              String(water) + "+" + String(fluxo) + "." + String(volume);
  
    // Envio dos pacotes
    HC12.print(Message);

    count++;   // Contagem de pacotes gerados

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
