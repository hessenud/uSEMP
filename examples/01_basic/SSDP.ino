void setupSSDP() 
{
  // the used SSDP Library is so simple, not everything demanded by the spec is implemented
  // I'll work on that soon
  String myIP = WiFi.localIP().toString();
    ssdp_cfg ssdpcfg = {
      /* description URL  */  "description.xml",
      /* udn_uuid         */  udn_uuid,
      /* IP               */  myIP.c_str(),
      
      /* deviceName       */  DEVICE_NAME,
      /* modelName        */  HOSTNAME,
      
      /* description      */  "uSEMP basic example",
      /* modelNr          */  "0.1",
      /* modelURL         */  "http://www.vendor.local/SSDPbasic01",

  
      /* manufacturer     */  "vendor.local",
      /* manufacturererURL*/  "http://www.vendor.local/modell",
      /* presentationURL  */  "/" // "index.html"
  
    };



    Serial.printf("Starting SSDP...\n");
    SSDP.setSchemaURL( ssdpcfg.descriptionURL);
    SSDP.setURL(ssdpcfg.descriptionURL);
    SSDP.setDeviceType("urn:schemas-simple-energy-management-protocol:device:Gateway:1");  
    SSDP.setHTTPPort(SEMP_PORT);

    //
    SSDP.setTTL(9);
    /** SSDP schema has to be recreated, as the SSDP Library Scheme can't be sufficiently expanded **/
    //-------  Schema overwrite BEGIN
    const char* ssdpScheme = g_semp->makeSsdpScheme( &ssdpcfg);
    //Serial.printf(" SSDPC Scheme:\n%s\n", ssdpScheme );        

    semp_server.on(String("/") + ssdpcfg.descriptionURL, HTTP_GET, [ssdpScheme]() {
      Serial.printf(" SSDP Scheme requested:\n%s\n", ssdpScheme ); 
      semp_server.send(200, "text/xml", ssdpScheme ); 
    });
    //-------  Schema overwrite END    
    SSDP.begin();

    Serial.printf("Ready!\n");
}
