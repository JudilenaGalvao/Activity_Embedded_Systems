# include <freertos/FreeRTOS.h>
# include <freertos/task.h>
# include <freertos/semphr.h>
#include <LiquidCrystal_I2C.h>
#include "DHTesp.h"

// Define o tempo de debounce para os botões
#define TEMPO_DEBOUNCE 10

// Pino do sensor DHT22
const int DHT_PIN = 16;

// Instância do sensor DHT22
DHTesp dhtSensor;

// Instância do LCD
LiquidCrystal_I2C lcd(0x27, 25, 4);

// Semáforos para controle de eventos
SemaphoreHandle_t sem;
SemaphoreHandle_t sem2;

// Variáveis globais para controle de estado
unsigned long timestamp_ultimo_acionamento = 0;
volatile boolean welcome = true;
volatile boolean trocou = false;
volatile char tipo = 'c';

// Protótipos das funções
void IRAM_ATTR isr();
void IRAM_ATTR isr2();
void vButton(void * algo);
void aButton(void * algo);
void mostrarLCD(void * algo);

// Configuração inicial
void setup() {
  // Configura os pinos dos botões como interrupções
  attachInterrupt(digitalPinToInterrupt(25), isr, RISING); // Botão vermelho
  attachInterrupt(digitalPinToInterrupt(26), isr2, RISING); // Botão amarelo

  // Inicializa a comunicação serial
  Serial.begin(115200);

  // Configura o sensor DHT22
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

  // Inicializa o LCD
  lcd.init();
  lcd.backlight();

  // Cria os semáforos
  sem = xSemaphoreCreateBinary();
  sem2 = xSemaphoreCreateBinary();

  // Cria as tarefas
  xTaskCreatePinnedToCore(vButton, "vermeio", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(aButton, "amarelu", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(mostrarLCD, "tevelisaum", 4096, NULL, 1, NULL, 0);
}

// Loop principal vazio
void loop() {
  // O loop é vazio pois as operações são realizadas em outras tarefas
}

// Manipulador de interrupção para o botão vermelho
void IRAM_ATTR isr() {
  if ((millis() - timestamp_ultimo_acionamento) >= TEMPO_DEBOUNCE) {
    xSemaphoreGiveFromISR(sem, NULL);
  } 
}

// Manipulador de interrupção para o botão amarelo
void IRAM_ATTR isr2() {
  if ((millis() - timestamp_ultimo_acionamento) >= TEMPO_DEBOUNCE) {
    xSemaphoreGiveFromISR(sem2, NULL);
  }
}

// Tarefa para lidar com o botão vermelho
void vButton(void * algo) {
  for (;;) {
    if (xSemaphoreTake(sem, pdMS_TO_TICKS(200)) == true) {
      // Troca entre 'c' e 'f'
      if (tipo == 'c') {
        tipo = 'f';
      } else if (tipo == 'f') {
        tipo = 'c';
      }
      welcome = false;
      trocou = true;
    }
  }
}

// Tarefa para lidar com o botão amarelo
void aButton(void * algo) {
  for (;;) {
    if (xSemaphoreTake(sem2, pdMS_TO_TICKS(200)) == true) {
      // Troca para o modo de boas-vindas
      welcome = true;
      trocou = true;
    }
  }
}

// Tarefa para exibir informações no LCD
void mostrarLCD(void * algo) {
  while (1) {
    if (trocou) {
      lcd.clear();
      trocou = false;
    }

    // Exibe a mensagem de boas-vindas
    if (welcome) {
      lcd.setCursor(0, 0);
      lcd.print("Bem vindo a ");
      lcd.setCursor(0, 1);
      lcd.print("primeira atividade!");
      lcd.setCursor(0, 2);
      lcd.print("Alunos: ");
      lcd.setCursor(0, 3);
      lcd.print("Judilena e Erick");
    } else {
      // Exibe a temperatura
      TempAndHumidity data = dhtSensor.getTempAndHumidity();
      lcd.setCursor(0, 0);
      lcd.print("Temperatura: ");
      if (tipo == 'f') {
        data.temperature = (data.temperature * 1.8) + 32;
      }
      lcd.setCursor(12, 0);
      lcd.print(String(data.temperature, 2) + " " + tipo);
    }
    vTaskDelay(pdMS_TO_TICKS(500)); // Atraso entre as atualizações do display
  }
}
