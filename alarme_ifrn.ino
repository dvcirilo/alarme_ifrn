/*
  Relógio, Calendário e controle de alarme de aulas para o IFRN - Pau dos Ferros
  Autor: Prof. Diego Cirilo - Mat. 1935729
  Data: 18/01/2013


  Pinos utilizados:
 * RTC - SDA - SCL(AN4, AN5)
 * SW1(da placa) - PIN0
 * SW2(eu botei) - PIN6
 * LED2(azul - PIN2
 * RELE1 - PIN7
 * RELE2 - PIN8
 * JUMPER1 - PIN9
 * LED1(verm) - PIN10
 * BUZZER - PIN13
 * LCD RS - PIN12
 * LCD Enable - PIN11
 * LCD D4 - PIN5
 * LCD D5 - PIN4
 * LCD D6 - PIN3
 * LCD D7 - PIN2

 */

// biblioteca do display
#include <LiquidCrystal.h>
// biblioteca do I2C(para comunicar com o RTC)
#include <Wire.h>

// inicializa o display com os pinos em questão(mencionados acima)
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

// aliases para os pinos
#define SW1 0
#define SW2 6
#define LED2 1
#define RELE1 7
#define RELE2 8
#define JUMPER1 9
#define LED1 10
#define BUZZER 13

// endereço de comunicação com o RTC, hardcoded, não tem como mudar...
#define DS1307_ADDRESS 0x68

//byte zero "de verdade"
byte zero = 0x00;

// variável para temporização
long timer01=0;

// variáveis para os valores de tempo. weekDay é o dia da semana de dom a sab, 0-6.
byte second, minute, hour, weekDay, monthDay, month, year;

// variáveis booleanas para alguns controles no código
bool toggle=false, firstpass=true;

// loop de inicialização, padrão do arduino.
void setup() {

  // setando os pinos de saída
  pinMode(BUZZER, OUTPUT);
  pinMode(RELE1, OUTPUT);
  pinMode(RELE2, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);

  // e os pinos de entrada. notar que eles tem um pull-up de 10k,
  // logo eles são lidos normalmente como HIGH, e se o botão é
  // pressionado, vai para LOW.
  pinMode(SW1, INPUT);
  pinMode(SW2, INPUT);
  pinMode(JUMPER1, INPUT);

  // Inicializar a interface I2C - RTC
  Wire.begin();

  // setando o número de colunas e linhas do display, no caso 8x2
  lcd.begin(8, 2);
}

// converte de decimal para BCD. utilizado já que o RTC trabalha com dígitos BCD
byte decToBcd(byte val){
  return ( (val/10*16) + (val%10) );
}

// converte de BCD para decimal. utilizado já que o RTC trabalha com dígitos BCD
byte bcdToDec(byte val)  {
  return ( (val/16*10) + (val%16) );
}

// atualiza a data e hora no RTC.
void setDateTime(){

  byte second =      55; //0-59
  byte minute =      49; //0-59
  byte hour =        14; //0-23
  byte weekDay =     3; //1-7
  byte monthDay =    21; //1-31
  byte month =       1; //1-12
  byte year  =       13; //0-99

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero); // vai p/ registro 0, onde ficam os segundos. leia o datasheet do DS1307

  Wire.write(decToBcd(second));
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));
  Wire.write(decToBcd(weekDay));
  Wire.write(decToBcd(monthDay));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));

  Wire.write(zero); //volta p/ registro 0 que é o canto certo

  Wire.endTransmission();

}

// lê a data do RTC e escreve nas variáveis globais de data, hora, etc
void getDate(){

  // vai p/ registro 0, dos segundos... p/ garantir
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire.write(zero);
  Wire.endTransmission();

  // pede 7 bytes ao RTC
  Wire.requestFrom(DS1307_ADDRESS, 7);

  // os 7 bytes de resposta.
  second = bcdToDec(Wire.read());
  minute = bcdToDec(Wire.read());
  hour = bcdToDec(Wire.read() & 0b111111); //formato 24hrs
  weekDay = bcdToDec(Wire.read()); //0-6 -> domingo - sábado
  monthDay = bcdToDec(Wire.read());
  month = bcdToDec(Wire.read());
  year = bcdToDec(Wire.read());
}

// atualiza o display, escrevendo as variáveis em questão.
void printDate(byte second_f, byte minute_f, byte hour_f, byte weekDay_f, byte monthDay_f, byte month_f, byte year_f){

  // imprime a data no formato dd/mm/aa e a hora na segunda linha, no formato hh:mm:ss
  lcd.setCursor(0, 0); //posicionando o cursor na posição 0 da primeira linha(0)
  if(monthDay<10){
    lcd.print(0); // necessário no caso do dia só ter um dígito, ele imprime um zero antes para manter o espaçamento correto.
  }
  lcd.print(monthDay);
  lcd.print("/");
  if(month<10){
    lcd.print(0); // mesmo caso do dia, e idem para todos os outros items.
  }
  lcd.print(month);
  lcd.print("/");
  if(year<10){
    lcd.print(0);
  }
  lcd.print(year);
  lcd.setCursor(0, 1); // posicionando o cursor na posição 0 da segunda linha(1).
  if(hour<10){
    lcd.print(0);
  }
  lcd.print(hour);
  lcd.print(":");
  if(minute<10){
    lcd.print(0);
  }
  lcd.print(minute);
  lcd.print(":");
  if(second<10){
    lcd.print(0);
  }
  lcd.print(second);
}

// seletor dos momentos de tocar, retorna true se tiver na hora!
bool timeToStudy(){
  byte jumper=digitalRead(JUMPER1);

  // se não for domingo(0) nem sábado(6) ou se jumper tiver HIGH(passa por cima de sábados e domingos, no caso de sábado letivo)
  // verifica pelas horas se toca 3s(intervalos e inicio/fim) ou 1s(entre aulas)
  if((weekDay!=0 && weekDay!=6) || jumper==HIGH){
    // toque de 3s para 07h00m, 12h00m, 13h00m, 18h00m, 19h00m
    if((hour==7 || hour==12 || hour==13 || hour==18 || hour==19) && minute==0 && (second>=0 && second<=2))
      return true;

    // toque de 3s para 8h30m, 10h30m, 14h30m, 16h30m, 20h30m
    if((hour==8 || hour==10 || hour==14 || hour==16 || hour==20) && minute==30 && (second>=0 && second<=2))
      return true;

    // toque de 3s para 08h50m, 14h50m
    if((hour==8 || hour==14) && minute==50 && (second>=0 && second<=2))
      return true;

    // toque de 3s para 10h20m, 16h20m
    if((hour==10 || hour==16) && minute==20 && (second>=0 && second<=2))
      return true;

    // toque de 3s para 20h40m, 22h10m
    if(((hour==20 && minute==40) || (hour==22 && minute==10)) && (second>=0 && second<=2))
      return true;

    // toque de 1s para 07h45m, 13h45m, 19h45
    if((hour==7 || hour==13 || hour==19) && minute==45 && second==0)
      return true;

    // toque de 1s para 09h35, 15h35m
    if((hour==9 || hour==15) && minute==35 && second==0)
      return true;

    // toque de 1s para 11h15m, 17h15m
    if((hour==11 || hour==17) && minute==15 && second==0)
      return true;

    // toque de 1s para 21h25m
    if(hour==21 && minute==25 && second==0)
      return true;
  } else{
    return false;
  }
  return false;
}

// loop principal do arduino, é padrão. a ação acontece aqui. tipo um "main" do C.
void loop() {

  // Lê a data do RTC e escreve nas variáveis globais lá.
  getDate();

  //teste
  if(digitalRead(SW1)==LOW){
    setDateTime();
  }

  // Escreve a data no display
  printDate(second, minute, hour, weekDay, monthDay, month, year);

  // chama timeToStudy para saber se ta na hora de tocar, se tiver, toca!
  if(timeToStudy()){
    digitalWrite(RELE1, HIGH);
    digitalWrite(LED1, HIGH);
  } else{
    digitalWrite(LED1, LOW);
    digitalWrite(RELE1, LOW);
  }
}
