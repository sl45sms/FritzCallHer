/**
    fritzcallme.ino
    Created on: 04.01.2021
    replace XXXXXXX with actual names and passwords
*/

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#include <MD5Builder.h>

ESP8266WiFiMulti WiFiMulti;
MD5Builder _md5;

//1 Byte size variable (UTF-8 Chars)
typedef uint8_t utf8_char;
//2 Byte size variable (UTF-16 Chars)
typedef uint16_t utf16_char;
//Pointers of UTF-8 chars (UTF8-String)
typedef utf8_char *utf8_string;
//Pointers of UTF-16 chars (UTF16-String)
typedef utf16_char *utf16_string;
//4 Byte size variable (TEMP Conversion)
typedef uint32_t utf32_char;




//////////////////////////////////////////////////////////////////////

size_t utf8_string_length(utf8_string s)
{
    size_t length = 0;
    char c;
    while ((c = *s++) != '\0')
    {
        if (c < 0 || c > 127)
        {
            if ((c & 0xE0) == 0xC0)
                s += 1;
            else if ((c & 0xF0) == 0xE0)
                s += 2;
            else if ((c & 0xF8) == 0xF0)
                s += 3;
        }

        length++;
    }
    return length;
}
//////////////////////////////////////////////////////////////////////
String md5(utf8_string str,uint16_t lng) {
  _md5.begin();
  _md5.add(str,lng);
  _md5.calculate();
  return _md5.toString();
}

//////////////////////////////////////////////////////////////////////
utf16_string IsoString_to_utf16(utf8_string chars)
{

    size_t length = utf8_string_length(chars);
    utf16_char *str = (utf16_char *)malloc(2 * (length + 1));
    str[length] = 0;

    size_t n = 0;
    size_t i = 0;
     
    while (true)
    {
        utf8_char ch =  chars[i++];

        if (ch == '\0')
            break;

        if (ch >> 7 == 0)
        {
            str[n] = ch;
            n++;
            continue;
        }

        //ERROR CHECK:
        /*
        if (c >> 6 == 0x02)
        {
            continue;
        }
        */

        // Get byte length
        char extraChars = 0;
        if (ch >> 5 == 0x06)
        {
            extraChars = 1;
        }
        else if (ch >> 4 == 0x0e)
        {
            extraChars = 2;
        }
        else if (ch >> 3 == 0x1e)
        {
            extraChars = 3;
        }
        else if (ch >> 2 == 0x3e)
        {
            extraChars = 4;
        }
        else if (ch >> 1 == 0x7e)
        {
            extraChars = 5;
        }
        else
        {
            continue;
        }

        utf32_char mask = (1 << (8 - extraChars - 1)) - 1;
        utf32_char res = ch & mask;
        utf8_char nextChar;
        size_t count;

        for (count = 0; count < extraChars; count++)
        {
            nextChar = chars[i++];

            if (nextChar >> 6 != 0x02)
                break;

            res = (res << 6) | (nextChar & 0x3f);
        }

        if (count != extraChars)
        {
            i--;
            continue;
        }

        if (res <= 0xffff)
        {
            str[n] = res;
            n++;
            continue;
        }

        res -= 0x10000;

        utf16_char high = ((res >> 10) & 0x3ff) + 0xd800;
        utf16_char low = (res & 0x3ff) + 0xdc00;

        str[n] = high;
        str[n + 1] = low;
        n += 2;
    }

    str[n] = 0;

    return str;
}



//////////////////////////////////////////////////////////////////////

String getChalenge(){
  String chalenge ="";
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    
    WiFiClient client;

    HTTPClient http;

    Serial.print("[HTTP] get chalenge...\n");
    if (http.begin(client, "http://192.168.178.1/login_sid.lua?version=1")) {  // HTTP
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
       int chalengeIndex = payload.indexOf("</Challenge>");
        chalenge = payload.substring(chalengeIndex-8,chalengeIndex);
        }
      } else {
        Serial.printf("[HTTP] GET... chalenge failed, error: %s\n", http.errorToString(httpCode).c_str());
        return "";
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect on get chalenge\n");
      return "";      
    }

Serial.println("\nchalenge="+chalenge+"\n");
return chalenge;

  }
Serial.println("unknown error on getChalenge");
return "";

}


String login(String chalenge,String username,String password){
   String sid="";
  // wait for WiFi connection
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    
    WiFiClient client;

    HTTPClient http;
 

  //POST to http://fritz.box/fon_num/fonbook_list.lua 
  String rawCompine = String(chalenge+"-"+password);

  utf8_char * utf16le = (utf8_string)IsoString_to_utf16((unsigned char*)rawCompine.c_str());


  String md5_token = md5(utf16le,rawCompine.length()*2);

  String login_token = chalenge + "-" + md5_token;
  String body="response="+login_token+"&username="+username;
  Serial.println(body);

    Serial.print("[HTTP] login...\n");

    if (http.begin(client, "http://192.168.178.1/login_sid.lua?version=1")) {  // HTTP
      // start connection and send HTTP header
      	
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      int httpCode = http.POST(body);

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] POST... code: %d\n", httpCode);
        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);
        int sidIndex = payload.indexOf("</SID>");
        sid = payload.substring(sidIndex-16,sidIndex);
        }
      } else {
        Serial.printf("[HTTP] GET... chalenge failed, error: %s\n", http.errorToString(httpCode).c_str());
        return "";
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect on get chalenge\n");
      return "";      
    }

Serial.println("\sid="+sid+"\n");
return sid;

  }

Serial.println("unknown error on login");
return "";
}



//////////////////////////////////////////////////////////////////////
void setup() {

  Serial.begin(115200);
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  for (uint8_t t = 8; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("XXXXXXX", "XXXXXXX");

}
///////////////////////////////////////////////////////////////////////
void loop() {
  String password = "XXXXXXX";
  String user="fritz3641";
  String chalenge = getChalenge();//c4395b03-82332a1c5d2606119c3921af7d2ec316
  String sid = login(chalenge,user,password);
  Serial.println("SID="+sid);
  delay(10000);
}

