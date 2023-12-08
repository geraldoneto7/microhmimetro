#include "ADS1X15.h"
#include <Wire.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Adafruit_BMP085.h>

const char* ssid = "esp32";
const char* password = "12345678"; // Senha da rede WiFi do ESP32
AsyncWebServer server(80);

ADS1115 ADS(0x48);

float Rref = 12;
float v1 = 0;
float v2 = 0;
float v3 = 0;
float v4 = 0;
float I = 0;
float Rx = 0;
float Rcorrig40 = 0;
float Rcorrig20 = 0;
float Rcorrig75 = 0;
float TempRef40 = 40.0;
float TempRef20 = 20.0;
float TempRef75 = 75.0;
float Rmedio20 = 0;
float Rmedio40 = 0;
float Rmedio75 = 0;
float Rxmedio = 0;
float Temperatura = 0;

Adafruit_BMP085 bmp;
int Pressao;
float Altitude;

const char* htmlContent = R"(
<!DOCTYPE html>
<html>

<head>
    <title>Medição de Resistência Rx</title>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
            color: #333;
            margin: 0;
            padding: 0;
            text-align: center;
            display: flex;
            align-items: center;
            justify-content: center;
            height: 100vh;
            flex-direction: column; /* Alinha os elementos verticalmente */
        }

        .valuesContainer {
            background-color: #0074d9;
            color: #fff;
            padding: 10px;
            display: inline-block;
            margin: 10px; /* Adiciona margem entre as caixas */
            width: 300px; /* Defina a largura fixa para todas as caixas */
        }

        .valueText {
            font-size: 18px;
        }

        .value {
            font-size: 24px;
            font-weight: bold;
            margin: 10px 0; /* Adiciona margem entre o texto e o valor */
        }
    </style>
    <script>
        // Função para atualizar os valores da resistência Rx e temperatura
        function updateValues() {
            fetch("/values")
                .then(response => response.json())
                .then(data => {
                    document.getElementById("resistanceValue20").textContent = "" + data.resistance20 + " Ohms";
                    document.getElementById("resistanceValue40").textContent = "" + data.resistance40 + " Ohms";
                    document.getElementById("resistanceValue75").textContent = "" + data.resistance75 + " Ohms";
                    document.getElementById("rxValue").textContent = "" + data.rx + " Ohms";
                    document.getElementById("temperatureValue").textContent = "" + data.temperature + " °C";
                });
        }

        // Atualize os valores periodicamente
        setInterval(updateValues, 2000);
    </script>
</head>

<body>
    <h1>MICROHMÍMETRO</h1>
    <div class="valuesContainer">
        <p class="valueText">Resistência a Temperatura ambiente:</p>
        <p class="value" id="rxValue">Aguardando leitura...</p>
    </div>
    <div class="valuesContainer">
        <p class="valueText">Resistência a 20 graus:</p>
        <p class="value" id="resistanceValue20">Aguardando leitura...</p>
    </div>
    <div class="valuesContainer">
        <p class="valueText">Resistência a 40 graus:</p>
        <p class="value" id="resistanceValue40">Aguardando leitura...</p>
    </div>
    <div class="valuesContainer">
        <p class="valueText">Resistência a 75 graus:</p>
        <p class="value" id="resistanceValue75">Aguardando leitura...</p>
    </div>
    <div class="valuesContainer">
        <p class="valueText">Temperatura atual:</p>
        <p class="value" id="temperatureValue">Aguardando leitura...</p>
    </div>
</body>

</html>


)";

void setup()
{
    Wire.begin();
    ADS.begin();
    Serial.begin(115200);
    if (!bmp.begin())
    {
        Serial.println("Sensor BMP180 não foi identificado! Verifique as conexões.");
        while (1)
        {
        }
    }

    // Configurar o ESP32 como Access Point (AP)
    WiFi.softAP(ssid, password);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("Endereço IP do AP: ");
    Serial.println(IP);

    if (!SPIFFS.begin())
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    ADS.begin();
    ADS.setGain(4);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", htmlContent); });

server.on("/values", HTTP_GET, [](AsyncWebServerRequest *request)
{
    String resistanceValue20 = String(Rmedio20, 3);
    String resistanceValue40 = String(Rmedio40, 3);
    String resistanceValue75 = String(Rmedio75, 3);
    String temperatureValue = String(Temperatura, 1);
    String rxValue = String(Rxmedio, 3);
    String json = "{\"resistance20\":\"" + resistanceValue20 + "\", \"resistance40\":\"" + resistanceValue40 + "\", \"resistance75\":\"" + resistanceValue75 + "\", \"rx\":\"" + rxValue + "\", \"temperature\":\"" + temperatureValue + "\"}";
    request->send(200, "application/json", json);
});

    server.begin();
}

void loop()
{
    const int numMedidas = 10;
    static int contadorMedidas = 0;
    static float somaRx20 = 0.0;
    static float somaRx40 = 0.0;
    static float somaRx75 = 0.0;
    static float somaRx = 0.0;

    Temperatura = bmp.readTemperature();
    Serial.print("Temperatura: ");
    Serial.print(Temperatura);
    Serial.print(" °C");
    Serial.println("\t");

    ADS.setGain(0);

    int16_t val_0 = ADS.readADC(0);
    int16_t val_1 = ADS.readADC(1);
    int16_t val_2 = ADS.readADC(2);
    int16_t val_3 = ADS.readADC(3);

    float f = ADS.toVoltage(1);
    v1 = val_0 * f;
    v2 = val_1 * f;
    v3 = val_2 * f;
    v4 = val_3 * f;
    I = (v2 - v1) / Rref;
    Rx = (v4 - v3) / I;

    if (Rx < 0.01 || Rx > 100 ){
      Rx = 0.0;
      somaRx20 = 0.0;
      somaRx40 = 0.0;
      somaRx75 = 0.0;
      contadorMedidas = 0;
    }
    

//*********** referencia de 40 graus**********************
    Rcorrig40 = Rx * (234.5 + TempRef40) / (234.5 + Temperatura);

//*********** referencia de 40 graus**********************
    Rcorrig20 = Rx * (234.5 + TempRef20) / (234.5 + Temperatura);

//*********** referencia de 40 graus**********************
    Rcorrig75 = Rx * (234.5 + TempRef75) / (234.5 + Temperatura);



    if (contadorMedidas == numMedidas)
    {
        Rxmedio = somaRx / numMedidas;
        Rmedio40 = somaRx40 / numMedidas;
        Rmedio20 = somaRx20 / numMedidas;
        Rmedio75 = somaRx75 / numMedidas;
        contadorMedidas = 0;
        somaRx20 = 0.0;
        somaRx40 = 0.0;
        somaRx75 = 0.0;
        somaRx = 0.0;
        Serial.print("Média temp Ambiente: ");
        Serial.println(Rxmedio, 3);
        Serial.print("Média a 20 graus: ");
        Serial.println(Rmedio20, 3);
        Serial.print("Média a 40 graus: ");
        Serial.println(Rmedio40, 3);
        Serial.print("Média a 75 graus: ");
        Serial.println(Rmedio75, 3);
    }
    somaRx += Rx;
    somaRx20 += Rcorrig20;
    somaRx40 += Rcorrig40;
    somaRx75 += Rcorrig75;
    contadorMedidas++;
}
