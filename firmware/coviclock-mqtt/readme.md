Sends some data over MQTT: you can use them to make Alexa Speak like in this video:  
  
  
[![MAX7219sz PIC Lib Demo](https://img.youtube.com/vi/7lhmFS1FO7A/maxresdefault.jpg)](https://www.youtube.com/watch?v=7lhmFS1FO7A) 

In order to achieve the same result: I've a Node-Red server + an MQTT broker (Mosquitto) in my house running on a Raspberry Pi I use for my Home-automation purposes.  
If you know how to use Node-Red + MQTT, you can install the node [node-red-contrib-alexa-remote2](https://flows.nodered.org/node/node-red-contrib-alexa-remote2) and follow [this tutorial](https://www.youtube.com/watch?v=vj9K0O_3zxI).

Edit Wi-Fi and MQTT settings in the code. An example JSON flow is provided for Node-Red.
