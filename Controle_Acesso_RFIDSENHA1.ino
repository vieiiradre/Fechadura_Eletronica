/**************************************************************
 *                       Autor                                *
 *    André Vieira dos Santos                                 *
 *    Graduando em Ciência da Computação                      *
 *    Universidade do Estado do Rio Grande do Norte (UERN)    *
 *    Santa Cruz - 2017                                       *
 **************************************************************/

#include <SPI.h>
#include <Ethernet.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

#define RST_PIN 9
#define SS_PIN 3

//==========================================================================================================//
//                                                  Variáveis                                               //
//==========================================================================================================//

int count = 0;
String conteudoRFID = "";
String Acesso = "";                                                                   // String de acesso para o histórico
String CPF = "";                                                                      // String CPF
String ID_PORT = "1";
char c;
char inString[32];                                                                    // String para dados em série recebidos
int stringPos = 0;                                                                    // Contador de String
boolean leitura = false;                                                              // Está lendo?


//==========================================================================================================//
//                                                  Teclado                                                 //
//==========================================================================================================//

const byte ROWS = 4;                                                                  //Quatro linhas
const byte COLS = 3;                                                                  //Três colunas
char keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};// definição do layout do teclado
byte rowPins[ROWS] = {31, 33, 35, 37};                                                    // Definição de pinos das linhas
byte colPins[COLS] = {39, 41, 43};                                                     // Definição de pinos das colunas
Keypad teclado = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

//==========================================================================================================//
//                                                  Servidor                                                //
//==========================================================================================================//
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };                                  // Endereço MAC
IPAddress ip(10, 0, 0, 101);                                                          // Endereço de ip do arduino
IPAddress server(10, 0, 0, 103);                                                      // Endereço do servidor onde vai estar instalado o wampserver
EthernetClient client;                                                                // Cliente para se conectar com o servidor


MFRC522 mfrc522(SS_PIN, RST_PIN);                                                     // Definicoes pino modulo RC522
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);                        // Inicializa o display no endereço 0x3F

//==========================================================================================================//
//                                          Protótipo das Funções                                           //
//==========================================================================================================//

void RFID();
void SSenha();
void AcionarFecho();
void LendoServidor();
void InserirHistorico();
void mensagemInicial();

void setup() {
  pinMode(22, OUTPUT);                                                                 // Define o pino para o rele como saída
  Serial.begin(9600);                                                                 // Inicializa a  comunicação serial
  SPI.begin();                                                                        // Inicializa o barramento SPI
  mfrc522.PCD_Init();                                                                 // Inicializa o módulo MFRC522
  lcd.begin(16, 2);                                                                   // Inicializa a bibloteca do LCD
  lcd.setCursor(1, 0);                                                                // Define a posição do cursor do LCD
  lcd.print("4cess Security");
  lcd.setCursor(2, 1);
  lcd.print("INICIANDO...");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Falha ao configurar Ethernet usando DHCP");                       // Comunicação serial, somente para depuração
    Ethernet.begin(mac, ip);
  }
  Serial.println(Ethernet.localIP());
  Serial.println();
}

void loop() {
  char tecla_pressionada = teclado.getKey();
  mensagemInicial();

  switch (tecla_pressionada) {
    case '*':
      RFID();
      break;
    case '#':
      SSenha();
      break;
  }
}

//==========================================================================================================//
//                                                  Funções                                                 //
//==========================================================================================================//

void mensagemInicial() {
  lcd.clear();
  lcd.setCursor(0, 0);                                                                // Define a posição do cursor do LCD
  lcd.print("* - RFID");
  lcd.setCursor(0, 1);
  lcd.print("# - Senha");
  delay(1000);
}

void RFID() {
  conteudoRFID = "";
  c = ' ';
  Acesso = "";
  CPF = "";
  memset( &inString, 0, 32 );                                                         // Limpando a memória inString
  
  lcd.clear();
  lcd.setCursor(1, 0);                                                                // Define a posição do cursor do LCD
  lcd.print("A proxime sua");
  lcd.setCursor(1, 1);
  lcd.print("tag ou cartao");
  delay(2000);

  // Prepare key - all keys are set to FFFFFFFFFFFFh at chip delivery from the factory.
  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;      
  
  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  Serial.print("Card UID:");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    conteudoRFID += (String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : ""));
    conteudoRFID +=(String(mfrc522.uid.uidByte[i], HEX));
    mfrc522.PICC_HaltA();
  }  
    
  conteudoRFID.toUpperCase();                                                           // Coloca o conteúdo da tag lida em maiúsculo
  Serial.println(conteudoRFID);
  
  //Condição de conexão, caso conectado faz a requisição, caso contrário retorna erro na conexão.
  if (client.connect(server, 80)) {
    Serial.println();
    Serial.println("Conectado");
    // Faça uma solicitação HTTP:
    client.print("GET /controle_acesso/selecionarRFID.php?ID_RFID=");
    client.print(conteudoRFID);
    client.println(" HTTP/1.1");
    client.println("Host: 10.0.0.103");
    client.println("Connection: close");
    client.println();
  } else {
    // Se você não tiver uma conexão com o servidor:
    Serial.println("Erro na conexão.");
  }
  
  LendoServidor();                                                                      // Chama a função que lê a resposta do servidor
  
  // Condição da validação da tag
  if(strlen (inString) == 4){
    Acesso = "Concedido";
    
    InserirHistorico();                                                                 // Chama a função que insere no histórico de acesso
    
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print(conteudoRFID);
    lcd.setCursor(0, 2);
    lcd.print("Acesso Permitido");
    delay(2000);
    AcionarFecho();
  
  }else if(strlen(inString)== 5){  
  
    Acesso = "Negado";
    
    InserirHistorico();                                                                 // Chama a função que insere no histórico de acesso
    
    lcd.clear();
    lcd.setCursor(4, 0);
    lcd.print(conteudoRFID);
    lcd.setCursor(2, 2);
    lcd.print("Acesso Negado");
    delay(5000);

  }else{
  
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("ERRO");
    lcd.setCursor(0, 1);
    lcd.print("Em manutencao");
    delay(2000);
  }
}

void SSenha(){
  conteudoRFID = "";
  CPF = "";
  String Senha = "";
  count = 0;
  char tecla;
  int entrada = 0;                                                                    // variável de apoio; números de entradas feitas via teclado
  
  // ------------------------------------ Capturando o CPF ------------------------------------ //
  lcd.clear();
  lcd.setCursor(0, 0);                                                                // Define a posição do cursor do LCD
  lcd.print("Digite o seu CPF");
  
  while (count <= 11 ){                                                               // enquanto não entrou os 11 números necessários para a senha
    tecla = teclado.getKey();                                                         // obtém informação do teclado
    if (tecla != NO_KEY){                                                             // se foi teclado algo
      CPF.concat(tecla);
      entrada += 1;                                                                   // aumenta contrador de entradas
      lcd.setCursor(entrada, 1);
      lcd.print(tecla);
      if((tecla == '#') || (entrada == 11)){                                          // foi teclado # ou 11 entradas incorretas
        break;                                                                        // interrompe loop
      }
    }
  }
  delay(1000);
  // --------------------------------- Fim da Captura do CPF--------------------------------- //

  // ************************************ Captura da senha ************************************ //
  count = 0;
  entrada = 0;                                                                        // variável de apoio; números de entradas feitas via teclado
  
  lcd.clear();
  lcd.setCursor(0, 0);                                                                // Define a posição do cursor do LCD
  lcd.print("Digite a senha:");
  
  while (count <= 8){                                                                 // enquanto não entrou os 8 números necessários para a senha
    tecla = teclado.getKey();                                                         // obtém informação do teclado
    if (tecla != NO_KEY){                                                             // se foi teclado algo
      Senha.concat(tecla);
      entrada += 1;                                                                   // aumenta contrador de entradas
      lcd.setCursor(entrada, 1);
      lcd.print("*");
      if((tecla == '#') || (entrada == 8)){                                           // foi teclado # ou 8 entradas incorretas
        break;                                                                        // interrompe loop
      }
    }
  }
  delay(1000);
  // ********************************** Fim da captura da senha ********************************** //
  
  
  //Condição de conexão, caso conectado faz a requisição, caso contrário retorna erro na conexão.
  if (client.connect(server, 80)) {
    Serial.println();
    Serial.println("Conectado");
    // Faça uma solicitação HTTP:
    client.print("GET /controle_acesso/selecionarSENHA.php?CPF=");
    client.print(CPF);
    client.print("&Senha=");
    client.print(Senha);
    client.println(" HTTP/1.1");
    client.println("Host: 10.0.0.103");
    client.println("Connection: close");
    client.println();
  } else {
    // Se você não tiver uma conexão com o servidor:
    Serial.println("Erro na conexão.");
  }
  
  LendoServidor();                                                                      // Chama a função que lê a resposta do servidor

  // Condição da validação da tag
  if(strlen (inString) == 4){
    Acesso = "Concedido";
    
    InserirHistorico();
    
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print(CPF);
    lcd.setCursor(0, 2);
    lcd.print("Acesso Permitido");
    delay(2000);
    AcionarFecho();                                                                     // Chama a função que aciona o fecho
  
  }else if(strlen(inString)== 5){  
  
    Acesso = "Negado";
    
    InserirHistorico();                                                                 // Chama a função que insere no histórico de acesso
    
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print(CPF);
    lcd.setCursor(2, 2);
    lcd.print("Acesso Negado");
    delay(5000);

  }else{
  
    lcd.clear();
    lcd.setCursor(6, 0);
    lcd.print("ERRO");
    lcd.setCursor(0, 1);
    lcd.print("Em manutencao");
    delay(2000);
  }
}

void InserirHistorico(){
  //Condição de conexão, caso conectado faz a requisição, caso contrário retorna erro na conexão.
  if (client.connect(server, 80)) {
    Serial.println();
    Serial.println("Conectado");
    // Faça uma solicitação HTTP:
    client.print("GET /controle_acesso/InserirHistorico.php?ID_PORT=");
    client.print(ID_PORT);
    client.print("&ID_RFID=");
    client.print(conteudoRFID);
    client.print("&CPF=");
    client.print(CPF);
    client.print("&Acesso=");
    client.print(Acesso);
    client.println(" HTTP/1.1");
    client.println("Host: 10.0.0.103");
    client.println("Connection: close");
    client.println();
    client.stop();
    client.flush();
  } else {
    // Se você não tiver uma conexão com o servidor:
    Serial.println("Erro na conexão.");
  }
}

void LendoServidor(){
  stringPos = 0;
  memset( &inString, 0, 32 );                                                           // Limpando a memória inString
  
  while(client.connected()){
    if (client.available()) {
      c = client.read();
      if (c == '<' ) {                                                                  // '<' caractere inicial
        leitura = true;                                                                 // Pronto para começar a ler a parte
      }else if(leitura){
        if(c != '>'){                                                                   // '>' caractere final
          inString[stringPos] = c;
          stringPos ++;
        }else{
          // conseguimos o que precisamos aqui! Podemos desligar agora
          leitura = false;
          client.stop();
          client.flush();
        }
      }
    }
  }  
}

void AcionarFecho(){
  digitalWrite(22, HIGH);                                                                // Liga o relé e aciona o fecho eletromagnético
  delay(2000);                                                                          // Proporciona 2 segundos para o usuário entrar 
  digitalWrite(22, LOW);                                                                 // Desliga o relé e fecha o fecho.
}

