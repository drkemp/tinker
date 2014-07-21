#include "http.h"
#include "application.h"

bool hostalive=false;
TCPClient client;
byte hostserver[] = {0,0,0,0};

bool hostconnect(byte host[]){
    hostserver[0]=host[0];
    hostserver[1]=host[1];
    hostserver[2]=host[2];
    hostserver[3]=host[3];
    if(client.connect(hostserver,8080)) {
//        Log("Client connected");
//        Log(String(Network.localIP()[3]));
        hostalive=false;
    } else {
//        Log("Client connect failed");
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
//        Log(s);
}

bool httpsend(byte host[],String s){
    if(!client.connected()){
        hostconnect(host);
    }
    if(client.connected()){
        s.concat(" HTTP/1.1");
        s.concat("\r\n");
        sendh(s);
        s = "HOST: ";
        s.concat(hostserver[0]);
        s.concat('.');
        s.concat(hostserver[1]);
        s.concat('.');
        s.concat(hostserver[2]);
        s.concat('.');
        s.concat(hostserver[3]);
        s.concat("\r\n");
        sendh(s);
        s="Accept: text/html, text/plain\r\n";
        sendh(s);
        s="\r\n";
        sendh(s);
    } else {
//        Log("Host not connecting");
        client.stop();
        hostalive=false;
    }
    return hostalive;
}

