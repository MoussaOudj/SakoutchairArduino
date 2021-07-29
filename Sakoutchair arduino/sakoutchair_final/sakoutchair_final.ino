#include <ESP8266WiFi.h>
#include <PubSubClient.h>  

//WiFi Connection configuration
char ssid[] = "ESGI";     //  le nom du reseau WIFI
char password[] = "Reseau-GES";  // le mot de passe WIFI
//mqtt server
char mqtt_server[] = "test.mosquitto.org";  //adresse IP serveur (mosquitto publique)
#define MQTT_USER "" 
#define MQTT_PASS ""

WiFiClient espClient;
PubSubClient MQTTclient(espClient);

int pin_trig = D3;
int pin_echo_up = D5;
int pin_echo_mid = D6;
int pin_echo_down = D7;

int const tailleMediane = 10;
int posMediane = tailleMediane / 2;
int counterForMediane = 0;
float mediane;
float tabUp [tailleMediane];
float tabMid [tailleMediane];
float tabDown [tailleMediane];

const int leftSensor = D4; 
const int rightSensor = D1; 
int left = 0; //status button
int right = 0; //status button

String isHere = "false";

//Fonctions blink pour l'envoi
void blinkOnBoardLed(){
   digitalWrite(LED_BUILTIN, LOW);
   delay(200);
   digitalWrite(LED_BUILTIN, HIGH);
}

String intToBool(int integer) {
  if (integer == LOW){
    return "true";
  }else{
    return "false";
  }
}

void sender(float d1, float d2, float d3, int left, int right) {
 //Creation d'un payload au format JSON + envoi dans topic
 String lString = intToBool(left);
 String rString = intToBool(right);
 String payloadTest = "{\"payload\":{\"sonar\":["+(String)d1+","+ (String)d2+","+ (String)d3+"],\"seat_left\":"+lString+",\"seat_right\":"+rString+"}}";
 Serial.println(payloadTest);
 MQTTclient.publish("sakoutcher/test/payload",payloadTest.c_str());
 delay(100);
 blinkOnBoardLed();
}

void sendPresence() {
  String newIsHere;
  if(left == LOW || right == LOW) {
   newIsHere = "true";
  }else {
   newIsHere = "false";
  }
  if(isHere != newIsHere){
    isHere = newIsHere;
    MQTTclient.publish("sakoutcher/test/isHere",isHere.c_str());
  }
}


//Fonction connection MQTT
void MQTTconnect() {
  
  while (!MQTTclient.connected()) {
      Serial.print("Attente  MQTT connection...");
      String clientId = "TestClient-";
      clientId += String(random(0xffff), HEX);

    // test connexion
    if (MQTTclient.connect(clientId.c_str(),"","")) {
      Serial.println("connected");

    } else {  // si echec affichage erreur
      Serial.print("ECHEC, rc=");
      Serial.print(MQTTclient.state());
      Serial.println(" nouvelle tentative dans 5 secondes");
      delay(5000);
    }
  }
}

void initTabs(){
  counterForMediane = 0;
  for(int i=0;i<tailleMediane;i++){
    tabUp[i] = 0;
    tabMid[i] = 0;
    tabDown[i] = 0;
  }
}

void setup() {
  Serial.begin(115200);
  
  pinMode(pin_trig, OUTPUT);
  pinMode(pin_echo_up, INPUT );
  pinMode(pin_echo_mid, INPUT );
  pinMode(pin_echo_down, INPUT );
  digitalWrite(pin_trig, LOW);

  pinMode(leftSensor, INPUT_PULLUP );
  pinMode(rightSensor, INPUT_PULLUP );
  initTabs();
  
  // Conexion WIFI
   WiFi.begin(ssid, password);
   Serial.println("");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
   Serial.println("Connected");
   MQTTclient.setServer(mqtt_server, 1883);
  
}

float getDistance(long timeMeasure){
  if (timeMeasure > 25000 || timeMeasure < 2){
    //Serial.println("echec de la mesure 1");
    return 0.01;
  } else {
    timeMeasure /= 2;
    float distance = timeMeasure*340/10000.0; //en cm
    //Serial.print("Distance = ");
    //Serial.print(distance);
    //Serial.println(" cm");
    return distance;
  }
}

long receptionTime(int echo){
  delay(20);
  digitalWrite(pin_trig, HIGH);
  delayMicroseconds(30);
  digitalWrite(pin_trig, LOW);
  return pulseIn(echo, HIGH);
}

void addDistanceInTab(float* tab, int pin){
  float distance = getDistance(receptionTime(pin));
  for(int i=0;i<tailleMediane;i++){
    if(tab[i] == 0){
      tab[i] = distance;
      break;
    }
  }
}

void sort(float* tab, int t){
  for(int i=0;i<t;i++)
  {   
    for(int j=i+1;j<t;j++)
    {
      if(tab[i]>tab[j])
      {
        float temp = tab[i];
        tab[i]=tab[j];
        tab[j]=temp;
      }
    }
  }
}

void mqttSend(){
   if (!MQTTclient.connected()) {
    MQTTconnect();
  }
  
  left = digitalRead(rightSensor);
  right = digitalRead(leftSensor);
  if((left == LOW)||(right == LOW)) {
    //float d1 = getDistance(receptionTime(pin_echo_up));
    //float d2 = getDistance(receptionTime(pin_echo_mid));
    //float d3 = getDistance(receptionTime(pin_echo_down));
    sender(tabUp[posMediane],tabMid[posMediane],tabDown[posMediane],left,right);
  }else{
    Serial.println("Await");
  }
  
  Serial.print("Distance up:");
  Serial.print(tabUp[posMediane]);
  Serial.println("cm");
  Serial.print("Distance mid:");
  Serial.print(tabMid[posMediane]);
  Serial.println("cm");
  Serial.print("Distance down:");
  Serial.print(tabDown[posMediane]);
  Serial.println("cm");
}

void loop() {
  left = digitalRead(rightSensor);
  right = digitalRead(leftSensor);
  if(right == LOW || left == LOW){
    Serial.print(".");
    counterForMediane ++;

    addDistanceInTab(tabUp, pin_echo_up);
    addDistanceInTab(tabMid, pin_echo_mid);
    addDistanceInTab(tabDown, pin_echo_down);
  } else {
    initTabs();
  }
  
  if(counterForMediane >= tailleMediane){
    sort(tabUp, tailleMediane);
    sort(tabMid, tailleMediane);
    sort(tabDown, tailleMediane);
  
    mqttSend();
    initTabs();
  }

  sendPresence();
  
  delay(200);
}
