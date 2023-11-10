#include <DHT.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTTYPE DHT11   // Tipo do Sensor DHT 11
uint8_t DHTPin = 14; 
               
// Inicialização do sensor DHT.
DHT dht(DHTPin, DHTTYPE);                

float Temp1;
float Temp2;
float Temp3;
float Temp4;

int cont0=0; // Contador do time0
int cont1=0; // Contador do time1

const int Button1 = 26; //pino do botão1 (Reset)
const int Button2 = 25; //pino do botão2 (Leia Ja)

#define TEMPO_DEBOUNCE 10 //ms
 
int contador_acionamentos1 = 0;
int ultimo_acionamento1 = 0;
unsigned long timestamp_ultimo_acionamento1 = 0;

int contador_acionamentos2 = 0;
int ultimo_acionamento2 = 0;
unsigned long timestamp_ultimo_acionamento2 = 0;

//Timer https://techtutorialsx.com/2017/10/07/esp32-arduino-timer-interrupts/
hw_timer_t * timer0 = NULL; // configurar o cronômetro 
hw_timer_t * timer1 = NULL; // configurar o cronômetro
// usaremos para cuidar da sincronização entre o loop principal e o ISR
portMUX_TYPE timerMux0 = portMUX_INITIALIZER_UNLOCKED; 
portMUX_TYPE timerMux1 = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer0(){
  //Leitura da temperatura
  portENTER_CRITICAL_ISR(&timerMux0);
  cont0 = 1;
  portEXIT_CRITICAL_ISR(&timerMux0);
}

void IRAM_ATTR onTimer1(){
  // noBacklight
  portENTER_CRITICAL_ISR(&timerMux1);
  cont1 = 1;
  portEXIT_CRITICAL_ISR(&timerMux1);
}

void serial(){
  // imprime as Temperaturas no serial monitor
  Serial.print("1º Temperatura: "); 
  Serial.print(Temp1);
  Serial.println(" *C");
  
  Serial.print("2º Temperatura: ");
  Serial.print(Temp2);
  Serial.println(" *C");
  
  Serial.print("3º Temperatura: ");
  Serial.print(Temp3);
  Serial.println(" *C");
  
  Serial.print("4º Temperatura: ");
  Serial.print(Temp4);
  Serial.println(" *C");
}

void LCD(){
   lcd.backlight();
   lcd.clear();

   lcd.setCursor(0, 0);
   lcd.print(Temp4);

   lcd.setCursor(0, 1);
   lcd.print(Temp3);

   lcd.setCursor(4, 1);
   lcd.print("  ");

   lcd.setCursor(6, 1);
   lcd.print(Temp2);

   lcd.setCursor(10, 1);
   lcd.print("  ");

   lcd.setCursor(12, 1);
   lcd.print(Temp1);

}

void Temp() {
  Temp1 = Temp2;
  Temp2 = Temp3;
  Temp3 = Temp4;
  Temp4 = dht.readTemperature();
}

void LeiaJa(){
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Botao 1 acionado");
  lcd.setCursor(0,1);
  lcd.print("Leia Ja!");
  delay(1000);

  Temp();

  Serial.print("--------Leia Ja--------\n");
  serial();
  LCD();

  // zerando os temporizadores
  timerWrite(timer0, 0); 
  timerWrite(timer1, 0);
}

void Reset() {
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Botao 2 acionado");
  lcd.setCursor(0,1);
  lcd.print("Reset");
  delay(1000);

  Temp1 = 00.0;
  Temp2 = 00.0;
  Temp3 = 00.0;
  Temp4 = dht.readTemperature();

  Serial.print("--------Reset--------\n");
  serial();
  LCD();

  // zerando os temporizadores
  timerWrite(timer0, 0);
  timerWrite(timer1, 0); 
}

void IRAM_ATTR funcao_ISR()
{
/* Conta acionamentos do botão considerando debounce */
  if ( (millis() - timestamp_ultimo_acionamento1) >= TEMPO_DEBOUNCE ){
    contador_acionamentos1++;
    timestamp_ultimo_acionamento1 = millis();
  }
}

void IRAM_ATTR funcao_ISR2()
{
/* Conta acionamentos do botão considerando debounce */
  if ( (millis() - timestamp_ultimo_acionamento2) >= TEMPO_DEBOUNCE ){
    contador_acionamentos2++;
    timestamp_ultimo_acionamento2 = millis();
  }
}
 
void setup() {
  Serial.begin(9600);
  //Define os botoes como entrada
  pinMode(Button1, INPUT_PULLDOWN);
  pinMode(Button2, INPUT_PULLDOWN);

  // inicializa o contador em ordem crescente no temporizador 1 (temos 4 temporizadores de hardware) 
  // com frequência de 80MHz (prescaler)
  timer1 = timerBegin(1, 80, true);  
  // time predefinido anteriomente, função e interrupção de borda
  timerAttachInterrupt(timer1, &onTimer1, true);
  timerAlarmWrite(timer1, 10000000, true); // Tempo em microsegundos 

  timer0 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer0, &onTimer0, true);
  timerAlarmWrite(timer0, 30000000, true);

  // habilitando o cronômetro
  timerAlarmEnable(timer0); 
  timerAlarmEnable(timer1); 

  attachInterrupt(Button1, funcao_ISR, RISING);
  attachInterrupt(Button2, funcao_ISR2, RISING);

  // Inicializando o sensor DHT
  pinMode(DHTPin, INPUT);
  dht.begin();              

  // Inicializando o LCD
  lcd.init();
}

void loop() {

    if (cont0 == 1) { 
      // Ler temperatura
      timerWrite(timer1, 0); //zera o temporizador1 
      lcd.backlight();

      Temp();
      Serial.print("--------Temperatura Atual--------\n");
      serial();
      LCD();
      cont0 = 0;
    }

    if (cont1 == 1) { 
      //interrupção para desligar o display
      lcd.noBacklight();
      lcd.clear();
      cont1 = 0;
    }
  
  //Botões 
  if (contador_acionamentos1 != ultimo_acionamento1){
    ultimo_acionamento1 = contador_acionamentos1;
    Serial.println("Botão 1 acionado = Reset!\n");
    Reset();
  }

  if (contador_acionamentos2 != ultimo_acionamento2){
      Serial.println("Botão 2 acionado = Leia Ja!\n");
      LeiaJa();
      ultimo_acionamento2 = contador_acionamentos2;
  }
}