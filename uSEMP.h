/*
  uSEMP.h - Basic implementation of SEMP Protocol
  Copyright (c) 2020 Udo Hessenauer  All right reserved.
 */

// ensure this library description is only included once
#ifndef USEMP_H
#define USEMP_H

#define DONT_USE_TINYXML

#include <Arduino.h>
#include <ESP8266WebServer.h>
#ifdef USE_TINYXML
#include <TinyXML.h>
#endif

// --- helper macros -------
#define KWh  *1000
#define KWh2KWs(kwh)  (kwh*3600)
#define Wh2Ws(wh) 	  (wh*3600)
#define DAY *(24*3600)
#define Wh2Ws( e ) ((e)*3600*1000)
#define Ws2Wh( e ) ((e)/Wh2Ws(1))
// ----  config
#define NR_OF_REQUESTS 4

// library interface descriptio


const char* time2str( unsigned long theTime );

struct ssdp_cfg {
	const char* descriptionURL;
	const char* udn_uuid;
	const char* IP;

	const char* deviceName;
	const char* modelName;

	const char* description;
	const char* modelNr;
	const char* modelURL;
	const char* manufacturer;
	const char* manufacturererURL;
	const char* presentationURL;

};


class PlanningData {
private:
	bool m_used;
public:
	unsigned m_minOnTime;
	unsigned m_maxOnTime;
	unsigned m_earliestStart;
	unsigned m_latestEnd;
	unsigned m_maxPwr;
	unsigned m_requestedEnergy = 0 KWh;
	unsigned m_optionalEnergy  = 0 KWh;
	PlanningData* m_next;


	void show();

	void reset()
	{
		Serial.printf("reset plan\n");
		m_used = false;
		m_minOnTime     = 0;
		m_maxOnTime    = 0;
		m_earliestStart  = 0;
		m_latestEnd     = 0;
		m_maxPwr = 1;
		m_requestedEnergy = 0 KWh;
		m_optionalEnergy  = 0 KWh;
		m_next = 0;
	}

	PlanningData() {
		reset();
	}

	PlanningData *set( unsigned i_min, unsigned i_max, unsigned i_est, unsigned i_let, unsigned i_maxPwr=0 )
	{
		m_used = true;
		m_minOnTime     = i_min;
		m_maxOnTime    	= i_max;
		m_earliestStart = i_est;
		m_latestEnd     = i_let;
		if ( i_maxPwr ) m_maxPwr = i_maxPwr;

		return this;
	}

	bool used() { return m_used; }


	bool updateEnergy(unsigned i_now, int i_req=0, int i_optional=0);


	PlanningData *requestEnergy(unsigned i_now, unsigned i_req, unsigned i_optional, unsigned i_est, unsigned i_let, unsigned i_maxPwr);
};

class DeviceInfo {

	const char* ChipID;
	const char* Udn_uuid;
	const char* DeviceID;
	const char* DeviceType;
	const char* DeviceName;
	const char* DeviceSerial;
	const char* Vendor;

public:
	DeviceInfo( const char* i_udn_uuid,const char* i_deviceID
			,const char* i_deviceName, const char* i_deviceType
			, const char* i_deviceSerial
			,const char* i_vendor )
{
		Udn_uuid = i_udn_uuid;
		DeviceID = i_deviceID;
		DeviceSerial = i_deviceSerial;
		DeviceName = i_deviceName;
		DeviceType = i_deviceType;
		Vendor = i_vendor;
}

	const char*& udn_uuid()      { return Udn_uuid; };
	const char*& deviceID()      { return DeviceID;}
	const char*& deviceName()    { return DeviceName;}
	const char*& deviceType()    { return DeviceType;}
	const char*& deviceSerial()  { return DeviceSerial;}
	const char*& vendor()        { return Vendor;};

	//friend class uSEMP;
};

class DeviceStatus {
public:

	const char* m_Id;
	bool m_acceptEMS;
	bool EM_On;

	unsigned m_maxConsumption;
	unsigned m_averagePwr;
	unsigned m_minPwr;
	unsigned m_maxPwr;

	PlanningData* m_activePlan;

public:

	DeviceStatus( const char* i_id, bool i_acceptEMS, unsigned i_maxConsumption )
	{
		m_Id = i_id;
		m_acceptEMS = i_acceptEMS;
		m_averagePwr = 0;
		m_minPwr = 0;
		m_maxPwr = 0;
		m_activePlan = 0;
		m_maxConsumption = i_maxConsumption;
	}

};


class uSEMP
{
#ifdef USE_TINYXML
	TinyXML    m_xml;
	uint8_t    m_xml_buffer[150]; // For XML decoding
	void XML_callback(uint8_t statusflags, char* tagName,
	                  uint16_t tagNameLen, char* data, uint16_t dataLen);

	///@todo replace tinyXML to get rid of this state g_activeSEMP
	static uSEMP* g_activeSEMP;
	static void xml_cb(uint8_t statusflags, char* tagName,
		    uint16_t tagNameLen, char* data, uint16_t dataLen);
#endif

	unsigned long (*get__time)();

	unsigned long getTime(){
		if (get__time) 	return get__time();// %(1 DAY);
		else			return 0;
	}
	void handleNotFound();


public:
	DeviceStatus  		stat;
	DeviceInfo	  		info;
	ESP8266WebServer* 	m_server;
	unsigned 			m_port;
	static const char* 	resp_tmpl;
	static const char* 	resp_header;
	static const char* 	resp_footer;

	static const char* 	deviceInfo_tmpl;
	static const char* 	deviceStatus_tmpl;
	static const char* 	planningRequest_tmpl;

    char* 				m_schemaS;

	unsigned size_devInfo;
	unsigned size_deviceStatus;
	unsigned size_planningRequest;
	unsigned size_SempRequest;

	char*  m_respBuffer;
	size_t m_sizeRespBuffer;
	void (*m_setPwrState)( bool i_state );
	PlanningData 	m_plans[NR_OF_REQUESTS];

	// user-accessible "public" interface

	uSEMP( const char* i_udn_uuid,const char* i_deviceID, const char* i_deviceName, const char* i_deviceType
			, const char* i_seviceSerial, const char* i_vendor, unsigned i_maxConsumption
			, unsigned long (*i_getTime)(), void (*setPwr)( bool i_state ),ESP8266WebServer* i_server, unsigned i_port  );

	const char* makeSsdpScheme( ssdp_cfg* i_ssdpcfg);

	void handlePowerCtl();
	void startService( );
	void loop();

	const char*& udn_uuid()      { return info.udn_uuid(); };
	const char*& deviceID()      { return info.deviceID();}
	const char*& deviceName()    { return info.deviceName();}
	const char*& deviceType()    { return info.deviceType();}
	const char*& deviceSerial()  { return info.deviceSerial();}
	const char*& vendor()        { return info.vendor();};



	/**
	 *  request energy
	 *  @param  i_now		timestamp in seconds ( e.g. unix time seconds since 1970... )
	 *  @param  i_req 		requested energy in [Wh]
	 *  @param  i_optional	optional energy in [Wh]
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
	 *  @param  i_optional	change of optional  energy in [Wh]
	 */
	void updateEnergy(unsigned i_now, int i_req, int i_optional);

	PlanningData*	getActivePlan();
	PlanningData*	getPlan(unsigned idx) { return &m_plans[idx]; }


	int deleteEnergyRequest(int i_plan);

	void setPwrState( bool i_state ) { stat.EM_On = i_state;
	Serial.printf("setPwrState(%s)\n",(stat.EM_On ? "ON":"OFF" )); if(m_setPwrState) m_setPwrState( stat.EM_On );}
	bool pwrState() { return stat.EM_On; }


	void setPwr( unsigned i_pwr, unsigned i_min, unsigned i_max) {
		stat.m_averagePwr = i_pwr;
		stat.m_minPwr = i_min;
		stat.m_maxPwr = i_max;
	}
	// library-accessible "private" interface
private:

	int	makeDeviceStatusRequest(char* o_wp);
	int	makeRequestFromPlan( unsigned i_now, PlanningData* i_plan, char *o_wp);//
	int makePlanningRequests( unsigned i_now, char* o_wp);//
	void resetEnergy()
	{
		Serial.printf("resetting plan %p --------\n", this);
		stat.m_activePlan->reset();
		stat.m_activePlan = 0;
	}
};

#endif

