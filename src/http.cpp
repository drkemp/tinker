#include "http.h"
#include "application.h"

bool hostalive=false;
TCPClient client;
String hosturl="";

void Log(String s); 

void sethosturl(String url) {
    hosturl=url;
    Log("Set host to "+url);
}

bool hostconnect() {
   return hostconnect(hosturl);
}

bool hostconnect(String url){
    char hostbuffer[512];
    hosturl=url;
    hosturl.toCharArray(hostbuffer,512);
    if(client.connect(hostbuffer,80)) {
        Log("Client connected");
        Log(String(Network.localIP()[3]));
        hostalive=false;
    } else {
        Log("Client url connect failed");
        hostalive=false;
    }
    return hostalive;
}

bool host_hasdata(){
    return (client.available()>0);
}

String readhost(){
    String s;
    while(client.available()>0){
        char c = client.read();
        s.concat(c);
    }
    return s;
}

void sendh( String s) {
        byte buf[100];
        int len=s.length();
        if(len>99) len =99;
        s.getBytes(buf,99);
        buf[len+1]=0;
        client.write(buf,len);
        Log(s);
}

String getHosturl() {
    String s="HOST: ";
    s.concat(hosturl);
    s.concat("\r\n");
    return s;
}

bool httpsend(String s){
    if(!client.connected()){
        hostconnect();
    }
    if(client.connected()){
        s.concat(" HTTP/1.1");
        s.concat("\r\n");
        sendh(s);
        s = getHosturl();
        sendh(s);
        s="Accept: text/html, text/plain\r\n";
        sendh(s);
        s="\r\n";
        sendh(s);
    } else {
        Log("Host not connecting");
        client.stop();
        hostalive=false;
    }
    return hostalive;
}

