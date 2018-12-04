#include <ESP8266WiFi.h>
#include <SPI.h>
#include <SD.h>
#include <ESP8266HTTPClient.h>


const int NUM_GENERATED_SIGNALS = 3;
int doomsday_counter = 0;
const int DOOMSDAY = 50;

//NETWORK STATIC VARIABLES
const char *ssid = "wl-fmat-ccei";    
const char *password = "";

//SAMPLING VARIABLES
const int SAMPLE_PERIOD_MILLIS = 5;
int captured_signals_counter = 0;
const int SAMPLING_TIME_SEC = 20;
const int MIN_YIELD_ITERS = 200;


//STORAGE STATIC VARIABLES
const int SD_digital_IO = 8;
int BUFFER_SIZE = 0;
const int MAX_MEM = 3000;

//SERVER VARIABLES
const String SERVER_URL = "/samples?";
String host =  "192.168.228.216";
const int port = 3003;


void setup() 
{
  Serial.begin(9600);
  begin_wifi_connection();
}

void begin_wifi_connection()
{
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid);
  while (WiFi.status() != WL_CONNECTED) 
  { 
    //Espera a que se conecte a Red WIFI
    delay(500);
    Serial.print(".");
    doomsday_counter++;
    if(doomsday_counter == DOOMSDAY)
     {
       hard_reset();
     }  
  }
  Serial.println("");
  Serial.println("WiFi conectado.");
}




void upload_ramp_signal()
{
  float window_size_millis = SAMPLING_TIME_SEC * 1000;
  int buffer_size = int(ceil(window_size_millis/SAMPLE_PERIOD_MILLIS)); 
  BUFFER_SIZE = buffer_size;
  int spike_magnitude = 100;
  int ramp_counter = 0;
  int yield_counter = 0;
  int send_counter = 0;
  String buffer_ = "";
  
  for(int i = 0; i < buffer_size; ++i)
  {
    ramp_counter++;
    yield_counter++;
    send_counter++;
     if(ramp_counter >= spike_magnitude)
     { 
        ramp_counter = 0;
     }
    if(ramp_counter < spike_magnitude/2)
     { 
       buffer_ = buffer_ + String(0) + ",";
     }else
     {
        buffer_ = buffer_ + String(ramp_counter) + ",";
     }
     if(send_counter == MAX_MEM)
     {
        GET(buffer_, "false");
        buffer_ = "";
        send_counter = 0;
     }
     if(yield_counter == MIN_YIELD_ITERS)
     {
        yield_counter = 0;
        yield();
     }
  }
   //POST(buffer_, "true");
    GET(buffer_, "true");
}


String format_request(String body, String isLast)
{
  String request;
  char quotationMark = '"';
  String equalsSpace = "= ";
  String ampersand = "& ";
  request.concat("data=");
  request.concat("{");
  request.concat(quotationMark);
  request.concat("signal");
  request.concat(quotationMark);
  request.concat(":");
  request.concat(quotationMark);
  request.concat(body);
  request.concat(quotationMark);
  request.concat(",");
  request.concat(quotationMark);
  request.concat("last");
  request.concat(quotationMark);
  request.concat(":");
  request.concat(quotationMark);
  request.concat(isLast);
  request.concat(quotationMark);
  request.concat("}");
  return request;
}

void GET(String stored_data, String is_last)
{

  String server_response = "";
  int yield_counter = 0;
  WiFiClient client;
  while(true)
  {
    
  while(!client.connect(host, port)) {
    Serial.println("Couldnt connect to the server");
    yield_counter++;
    if(yield_counter == MIN_YIELD_ITERS)
       {
          yield_counter = 0;
          yield();
       }
  }
  Serial.println("Connected to server!");
  String host_dir = SERVER_URL + "&signal=" + stored_data + "&last=" + is_last;
  client.print(String("GET ") + host_dir + " HTTP/1.1\r\n" +
             "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  
  //Espera respuesta del servidor
  while (client.available() == 0) {   
    if(yield_counter == MIN_YIELD_ITERS)
      {
         yield_counter = 0;
         yield();
      }
  }

  // Lee la respuesta del servidor y la muestra por el puerto serie
  while (client.available()) {
    server_response = client.readStringUntil('\r');
    yield_counter++;
    if(yield_counter == MIN_YIELD_ITERS)
       {
          yield_counter = 0;
          yield();
       }
  }
  Serial.println("Server_response = " + server_response);
  if(server_response.equals("\nok"))
  {
    break;
  }else{
     Serial.println("Failed request");
     Serial.println("Trying again...\n\n");
  }
  server_response = "";
   if(yield_counter == MIN_YIELD_ITERS)
     {
          yield_counter = 0;
          yield();
      }
  }
}

void POST(String stored_data, String is_last)
{
  HTTPClient http;
  String server_response = "";
  int yield_counter = 0;
  while(true)
  {
        int begin_code = http.begin(SERVER_URL);
        Serial.println("begin= " + String(begin_code));
        http.addHeader("Content-Type", "text/plain");
        int http_code = http.POST(format_request(stored_data, "false"));
        server_response = http.getString();
        http.end();
        Serial.println("Server response: " + server_response);
        if(server_response == "ok"){
          break;
        }else{
           Serial.println("Failed request");
           Serial.println("Trying again...\n\n");
        }
        yield_counter++;
        if(yield_counter == MIN_YIELD_ITERS)
        {
          yield_counter = 0;
          yield();
        }
        doomsday_counter++;
        if(doomsday_counter == DOOMSDAY)
        {
          hard_reset();
        }
     }
  doomsday_counter = 0;
}




void hard_reset()
{
  Serial.println('\n');
  Serial.println("Got some bad news for ya...");
  Serial.println("\nDoomsday counter has already ticked " + String(DOOMSDAY) + " times");
  Serial.println("There seems to be a problem with either the WiFi connection or server connectivity...");
  Serial.println("\nIssuing a hard reset...");
  Serial.println("Good Bye.");
  ESP.reset();
}


void loop() 
{
  if(captured_signals_counter < NUM_GENERATED_SIGNALS)
  {
    upload_ramp_signal();
    Serial.print("Signal successfully uploaded...\n");
    captured_signals_counter++;
 }

}
