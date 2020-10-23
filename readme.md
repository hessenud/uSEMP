This is a very basic C++ library for Arduino, Implementing SEMP "Simple Energy Management Protocol" from SMA
Right now it works for Esp8266  only, due to the internal usage of Esp8266WebServer. Something that i will probably never address...

Installation
--------------------------------------------------------------------------------

To install this library, just place this entire folder as a subfolder in your
Arduino/lib/targets/libraries folder.

When installed, this library should look like:

Arduino/lib/targets/libraries/uSEMP              (this library's folder)
Arduino/lib/targets/libraries/uSEMP/uSEMP.cpp     (the library implementation file)
Arduino/lib/targets/libraries/uSEMP/uSEMP.h       (the library description file)
Arduino/lib/targets/libraries/uSEMP/keywords.txt (the syntax coloring file)
Arduino/lib/targets/libraries/uSEMP/examples     (the examples in the "open" menu)
Arduino/lib/targets/libraries/uSEMP/readme.txt   (this file)

Usage
--------------------------------------------------------------------------------
After this library is installed, you just have to start the Arduino application.
You may see a few warning messages as it's built.

To use this library in a sketch, go to the Sketch | Import Library menu and
select uSEMP.  This will add a corresponding line to the top of your sketch:

    #include <uSEMP.h>

To stop using this library, delete that line from your sketch.

The Library lets you create a SEMP Device with it's own EM Gateway

    uSEMP( const char* i_udn_uuid,const char* i_deviceID, const char* i_deviceName, const char* i_deviceType
			, const char* i_seviceSerial, const char* i_vendor, unsigned i_maxConsumption
			, unsigned long (*i_getTime)(), void (*setPwr)( bool i_state ) i_getTime)(), ESP8266WebServer* i_server, unsigned i_port );
	
    uSEMP* g_semp = new uSEMP(...);
    
The Constructor requires a callback to a "time()" function to get access to the device's timebase. This is needed for generating planning requests.
A second callback is feedback of EM Control to the device application "setPwr" and a pointer to the webserver and its port used for serving SEMP requests


To make itself known to the Energy Manager (EM) the device uses SSDP. The Arduino ESP8266SSDP Library is somewhat limited but still usable,
as long as you overwrite the Schema. The uSEMP class provides a _makeSsdpScheme()_ method. I haven't found a satisfying way to put that 
inside the SEMP Library. This is something i will target someday...

	...
	
    SSDP.setSchemaURL( ssdpcfg.descriptionURL); //< this 
	...
    const char* ssdpScheme = g_semp->makeSsdpScheme( &ssdpcfg);
    http_server.on(String("/") + ssdpcfg.descriptionURL, HTTP_GET, [ssdpScheme]() {
      http_server.send(200, "text/xml", ssdpScheme ); 
    });
    
Now the device is effectivly started after the SEMP Service and SSDP is started

		void startService( );   
		g_semp->startService(); 
		SSDP.begin();
		
When operational you may request energy, and update the actual energy-request (plan)		

	/**
	 *  request energy
	 *  @param  i_now		timestamp in seconds ( e.g. unix time seconds since 1970... )
	 *  @param  i_req 		requested energy in [Wh]
	 *  @param  i_optional		optional energy in [Wh]
	 *  @param  i_est		earliest start time in seconds (absolute timestamp like i_now)
	 *  @param  i_let		latest end time in seconds (absolute timestamp like i_now)
	 *
	 *  @return handle to created plan, -1 if creation failed
	 */
	int requestEnergy(unsigned i_now, unsigned i_req, unsigned i_optional, unsigned i_est, unsigned i_let );

	/**
	 *  update runtime/ energy (differentially )
	 *  @param  i_now		timestamp in seconds ( e.g. unix time seconds since 1970... )
	 *  @param  i_req 		change of required  energy in [Wh]
	 *  @param  i_optional		change of optional  energy in [Wh]
	 */
	void updateEnergy(unsigned i_now, int i_req, int i_optional);
	
An energy plan is updated using updateEnergy
	 
	 g_semp->updateEnergy( now, -1, 0);  // reduces required energy of the actual energy request by 1Wh
	 
	 g_semp->getPlan(2)->updateEnergy( now, -1, 0);  // reduces required energy of the plan 2 by 1Wh
	 
	 g_semp->deleteEnergyRequest( 2);  // delete energy request 2



ToDo: 
-----------------------------		
PlanningData is only focused on energy. Sometimes it's more adequate to plan with times only because the actual energy consumption is not known. 
(washing machine, dryer etc..)
    