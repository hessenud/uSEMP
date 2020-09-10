void setupSSDP() 
{

  String myIP = WiFi.localIP().toString();
    ssdp_cfg ssdpcfg = {
      /* description URL  */  "description.xml",
      /* udn_uuid         */  udn_uuid,
      /* IP               */  myIP.c_str(),
      
      /* deviceName       */  DEVICE_NAME,
      /* modelName        */  HOSTNAME,
      
      /* description      */  "Sonoff POW reloaded",
      /* modelNr          */  "0.1",
      /* modelURL         */  "http://www.marihessenauer.de/TPOW_V1",

  
      /* manufacturer     */  "vendor.local",
      /* manufacturererURL*/  "http://www.vendor.local/modell",
      /* presentationURL  */  "/" // "index.html"
  
    };



    Serial.printf("Starting SSDP...\n");
    SSDP.setSchemaURL( ssdpcfg.descriptionURL);
    SSDP.setURL(ssdpcfg.descriptionURL);
    SSDP.setDeviceType("urn:schemas-simple-energy-management-protocol:device:Gateway:1");  
    SSDP.setHTTPPort(80);

    //
    SSDP.setTTL(9);
    /** SSDP schema has to be reaced, as the SSDP Library Scheme can't be sufficiently expanded **/
    //-------  Schema overwrite BEGIN
    const char* ssdpScheme = g_semp->makeSsdpScheme( &ssdpcfg);
    Serial.printf(" SSDPC Scheme:\n%s\n", ssdpScheme );        

    http_server.on(String("/") + ssdpcfg.descriptionURL, HTTP_GET, [ssdpScheme]() {
      Serial.printf("SSDP request /description.xml\n%s", ssdpScheme ); //schemaS.c_str()); 
     Serial.printf("SSDP request /description.xml\n%s", ssdpScheme ); //schemaS.c_str()); 
    // SSDP.schema(http_server.client());
      http_server.send(200, "text/xml", ssdpScheme ); //schemaS.c_str() );
    });
    //-------  Schema overwrite END    
    SSDP.begin();

    Serial.printf("Ready!\n");
}
