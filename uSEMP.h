/*
  uSEMP.h - Basic implementation of SEMP Protocol
  Copyright (c) 2020 Udo Hessenauer  All right reserved.
 */

// ensure this library description is only included once
#ifndef USEMP_H
#define USEMP_H

#include "Arduino.h"
#include <ESP8266WebServer.h>
#include <TinyXML.h>

// --- helper macros -------
#define KWh  *1000
#define KWh2KWs(kwh)  (kwh*3600)
#define DAY *(24*3600)
#define Wh2Wms( e ) ((e)*3600*1000)
#define Wms2Wh( e ) ((e)/Wh2Wms(1))
// ----  config
#define NR_OF_REQUESTS 1

// library interface descriptio
extern const char* getTimeString( unsigned long theTime );


class PlanningData {
public:
	unsigned m_minOnTime;
	unsigned m_maxOnTime;
	unsigned m_earliestStart;
	unsigned m_latestEnd;
	unsigned m_maxPwr;
	unsigned m_requestedEnergy = 0 KWh;
	unsigned m_optionalEnergy  = 0 KWh;

	PlanningData(){
		m_minOnTime     = 0;
		m_maxOnTime    = 0;
		m_earliestStart  = 0;
		m_latestEnd     = 0;
		m_maxPwr = 1;
		m_requestedEnergy = 0 KWh;
		m_optionalEnergy  = 0 KWh;
	}

	PlanningData *set( unsigned i_min, unsigned i_max, unsigned i_est, unsigned i_let, unsigned i_maxPwr=0 )
	{
		m_minOnTime     = i_min;
		m_maxOnTime    	= i_max;
		m_earliestStart = i_est;
		m_latestEnd     = i_let;
		if ( i_maxPwr ) m_maxPwr = i_maxPwr;
		return this;
	}



	void updateEnergy( int i_req, int i_optional);


	PlanningData *requestEnergy(unsigned i_req, unsigned i_optional, unsigned i_est, unsigned i_let, unsigned i_maxPwr){
		//unsigned _now = get_time()%(1 DAY);
		m_earliestStart = i_est;// + _now;
		m_latestEnd     = i_let;// + _now;
		m_maxPwr = i_maxPwr;
		m_requestedEnergy = 0 KWh;
		m_optionalEnergy  = 0 KWh;
		updateEnergy(i_req, i_optional); // diff
		return this;
	}
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

	void updateEnergy( int i_req, int i_optional);

};


void xcb(uint8_t statusflags, char* tagName,
        uint16_t tagNameLen, char* data, uint16_t dataLen);
class uSEMP
{

	TinyXML    m_xml;
	uint8_t    m_xml_buffer[150]; // For XML decoding
	void XML_callback(uint8_t statusflags, char* tagName,
	                  uint16_t tagNameLen, char* data, uint16_t dataLen);
	friend void xcb(uint8_t statusflags, char* tagName,
	        uint16_t tagNameLen, char* data, uint16_t dataLen);
public:
	DeviceStatus  stat;
	DeviceInfo	  info;
	ESP8266WebServer* m_server;
	static const char* resp_tmpl;
	static const char* resp_header;
	static const char* resp_footer;


	static const char* deviceInfo_tmpl;
	static const char* deviceStatus_tmpl;
	static const char* planningRequest_tmpl;

	unsigned size_devInfo;
	unsigned size_deviceStatus;
	unsigned size_planningRequest;

	char* _respBuffer;
	//	 char* deviceInfo;
	//	 char* deviceStatus;
	//	 char* planningRequest;
	unsigned long (*get_time)();
	void (*m_setPwrState)( bool i_state );
	PlanningData plans[NR_OF_REQUESTS];

	// user-accessible "public" interface

	uSEMP( const char* i_udn_uuid,const char* i_deviceID, const char* i_deviceName, const char* i_deviceType
			, const char* i_seviceSerial, const char* i_vendor, unsigned i_maxConsumption
			, unsigned long (*i_getTime)() );

	void handlePowerCtl();
	void startService(ESP8266WebServer* http_server, void (*m_setPwr)( bool i_state ) );
	const char*& udn_uuid()      { return info.udn_uuid(); };
	const char*& deviceID()      { return info.deviceID();}
	const char*& deviceName()    { return info.deviceName();}
	const char*& deviceType()    { return info.deviceType();}
	const char*& deviceSerial()  { return info.deviceSerial();}
	const char*& vendor()        { return info.vendor();};


	/// update runtime/ energy (differentially )
	void updateEnergy(int i_req, int i_optional)
	{
		if ( stat.m_activePlan ) stat.m_activePlan->updateEnergy(i_req, i_optional );
	}

	PlanningData*	getActivePlan();


	PlanningData* requestEnergy(unsigned i_req, unsigned i_optional, unsigned i_est, unsigned i_let ){
		if (stat.m_activePlan) stat.m_activePlan->requestEnergy( i_req, i_optional, i_est, i_let, stat.m_maxConsumption);
		return getActivePlan();
	}

	void setPwrState( bool i_state ) { stat.EM_On = i_state; }
	bool pwrState() { return stat.EM_On; }


	void setPwr( unsigned i_pwr, unsigned i_min, unsigned i_max) {
		stat.m_averagePwr = i_pwr;
		stat.m_minPwr = i_min;
		stat.m_maxPwr = i_max;
	}
	// library-accessible "private" interface
private:
	int value;
	void doSomethingSecret(void);


	const char* 	makeDeviceStatusRequest();
	const char* 	makePlanningRequest();//
	void resetEnergy()
	{
		stat.m_activePlan->m_requestedEnergy = stat.m_activePlan->m_optionalEnergy = 0;
	}
};

#endif

