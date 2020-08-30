/*
  uSEMP.h - uSEMP library Basic implementation of SEMP Protocol
  Copyright (c) 2020 Udo Hessenauer  All right reserved.
*/

// include this library's description file
#include "uSEMP.h"



const char* uSEMP::resp_tmpl   = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<Device2EM xmlns=\"http://www.sma.de/communication/schema/SEMP/v1\">\r\n%s</Device2EM>";
const char* uSEMP::resp_header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<Device2EM xmlns=\"http://www.sma.de/communication/schema/SEMP/v1\">\r\n";
const char* uSEMP::resp_footer = "</Device2EM>";



const char* uSEMP::deviceInfo_tmpl =
		" <DeviceInfo>\r\n"
		"   <Identification>\r\n"
		"     <DeviceId>%s</DeviceId>\r\n"
		"     <DeviceName>%s</DeviceName>\r\n"
		"     <DeviceType>%s</DeviceType>\r\n"
		"     <DeviceSerial>%s</DeviceSerial>\r\n"
		"     <DeviceVendor>%s</DeviceVendor>\r\n"
		"   </Identification>\r\n"
		"   <Characteristics>\r\n"
		"     <MaxPowerConsumption>%d</MaxPowerConsumption>\r\n"
		"     <MinOnTime>60</MinOnTime>\r\n"
		"     <MinOffTime>60</MinOffTime>\r\n"
		"   </Characteristics>\r\n"
		"   <Capabilities>\r\n"
		"     <CurrentPower>\r\n"
		"       <Method>Measurement</Method>\r\n"
		"     </CurrentPower>\r\n"
		"     <Interruptions>\r\n"
		"       <InterruptionsAllowed>true</InterruptionsAllowed>\r\n"
		"     </Interruptions>\r\n"
		"     <Timestamps><AbsoluteTimestamps>false</AbsoluteTimestamps></Timestamps>\r\n"
		"     <Requests>\r\n"
		"       <OptionalEnergy>true</OptionalEnergy>\r\n"
		"     </Requests>\r\n"
		"   </Capabilities>\r\n"
		" </DeviceInfo>\r\n";

const char* uSEMP::deviceStatus_tmpl =
		" <DeviceStatus>\r\n"
		"   <DeviceId>%s</DeviceId>\r\n"
		"   <EMSignalsAccepted>%s</EMSignalsAccepted>\r\n"
		"   <Status>%s</Status>\r\n"
		"   <PowerConsumption>\r\n"
		"     <PowerInfo>\r\n"
		"       <AveragePower>%u</AveragePower>\r\n"
		"       <MinPower>%u</MinPower>\r\n"
		"       <MaxPower>%u</MaxPower>\r\n"
		"       <Timestamp>%u</Timestamp>\r\n"
		"       <AveragingInterval>60</AveragingInterval>\r\n"
		"     </PowerInfo>\r\n"
		"   </PowerConsumption>\r\n"
		" </DeviceStatus>\r\n";

const char* uSEMP::planningRequest_tmpl =
		" <PlanningRequest>\r\n"
		"   <Timeframe>\r\n"
		"     <DeviceId>%s</DeviceId>\r\n"
		"     <EarliestStart>%d</EarliestStart>\r\n"
		"     <LatestEnd>%d</LatestEnd>\r\n"
		"     <MinRunningTime>%u</MinRunningTime>\r\n"
		"     <MaxRunningTime>%u</MaxRunningTime>\r\n"
		"   </Timeframe>\r\n"
		" </PlanningRequest>\r\n";



// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances


//extern unsigned long getTime(void);
uSEMP* g_activeSEMP; // active SEMP Object for this request...  Ugly but as long as i use TinyXML...
void xcb(uint8_t statusflags, char* tagName,
        uint16_t tagNameLen, char* data, uint16_t dataLen)
{
	if (g_activeSEMP)	g_activeSEMP->XML_callback( statusflags, tagName, tagNameLen, data, dataLen);
}

uSEMP::uSEMP( const char* i_udn_uuid,const char* i_deviceID, const char* i_deviceName
		, const char* i_deviceSerial, const char* i_deviceType, const char* i_vendor, unsigned i_maxConsumption, unsigned long (*i_getTime)() )
			: stat( i_deviceID, true, i_maxConsumption )
			,info( i_udn_uuid, i_deviceID, i_deviceName, i_deviceSerial, i_deviceType, i_vendor)
{
	  // initialize this instance's variables
	get_time = i_getTime;
	size_devInfo            = strlen(deviceInfo_tmpl) + strlen(info.deviceID()) + strlen(info.deviceName()) + strlen(info.deviceSerial());
	size_deviceStatus	    = strlen(deviceStatus_tmpl) + strlen(info.deviceID()) + 12*32;
	size_planningRequest	= strlen(planningRequest_tmpl) + strlen(info.deviceID()) + 7*8;

	_respBuffer = (char*) malloc(strlen(resp_header)+strlen(resp_footer)+size_devInfo+size_deviceStatus+ NR_OF_REQUESTS* size_planningRequest )  +3;
	 stat.m_activePlan = &plans[0]; // todo support many plans
	 m_xml.init((uint8_t *)m_xml_buffer, sizeof(m_xml_buffer), xcb ); /*(XMLcallback)([this](uint8_t statusflags, char* tagName,
             uint16_t tagNameLen, char* data, uint16_t dataLen) {XML_callback( statusflags, tagName,
                      tagNameLen,  data,  dataLen);  }));*/

}

void uSEMP::XML_callback(uint8_t statusflags, char* tagName,
                  uint16_t tagNameLen, char* data, uint16_t dataLen)
{
  char dbuf[dataLen+1];
  char tbuf[tagNameLen+1];
  memcpy(dbuf, data,    dataLen );      dbuf[dataLen] = 0;
  memcpy(tbuf, tagName, tagNameLen );   tbuf[tagNameLen] = 0;

  if(statusflags & STATUS_TAG_TEXT) {
    if (!strcasecmp(tagName, "/EM2Device/DeviceControl/On")) {
      if ( strncmp(dbuf, "true", 4) == 0 ) {
        setPwrState( HIGH );
      } else {
        setPwrState( LOW );
      }
      if ( m_setPwrState ) m_setPwrState( stat.EM_On );
    }
  }
}



void uSEMP::handlePowerCtl() {
	Serial.printf("uSEMP PWR CTL request /\n");
	for( int n = 0; n < m_server->args(); ++n)
	{
		String p1Name = m_server->argName(n);
		String p1Val = m_server->arg(n);
		Serial.printf("p%dName: %s  val: %s\n",n, p1Name.c_str(), p1Val.c_str() );
		if (p1Name == String("plain"))
		{
			g_activeSEMP = this; //this is nasty stateful code, but w/o modifying TinyXML.....
			const char* ch = p1Val.c_str();
			while(*ch) {
				m_xml.processChar(*ch);
				++ch;
			}
			g_activeSEMP = this;
			m_server->send ( 200, "application/xml", "<Device2EM></Device2EM>"  );
			return;
		} else {

		}
	}
	m_server->send ( 400, "application/xml", "<Device2EM></Device2EM>"  );
	return;
}

void uSEMP::startService(ESP8266WebServer* i_server, void (*i_setPwr)( bool i_state ) ) {
	m_server = i_server;
	m_setPwrState = i_setPwr;
	m_server->on("/semp/", HTTP_GET, [this]() {
	      Serial.println("SEMP request /\n");
	      snprintf(_respBuffer, size_devInfo, deviceInfo_tmpl, info.deviceID(), info.deviceName()
	    		  , info.deviceType(), info.deviceSerial(), info.vendor()
				  , stat.m_maxPwr );
	      String resp = String(resp_header) + String(_respBuffer)
	    		  + String(makeDeviceStatusRequest()) + String(makePlanningRequest())
				  + String(resp_footer);
//	      Serial.println( resp );
	      m_server->send ( 200, "application/xml", resp );
	    });
	    m_server->on("/semp/DeviceInfo", HTTP_GET, [this]() {
	      snprintf(_respBuffer, size_devInfo, deviceInfo_tmpl, info.deviceID(), info.deviceName(), info.deviceType(), info.deviceSerial(), info.vendor(), stat.m_maxPwr );
	      String resp = String(uSEMP::resp_header) + String(_respBuffer) + String(uSEMP::resp_footer);
	      Serial.println( "/semp/DeviceInfo" );
//	      Serial.println( resp );
	      m_server->send ( 200, "application/xml", resp  );
	    });
	    m_server->on("/semp/DeviceStatus", HTTP_GET, [this]() {
	       String resp = String(uSEMP::resp_header) + String(makeDeviceStatusRequest()) + String(uSEMP::resp_footer) ;
	       Serial.println( "/semp/DeviceStatus" );
//	       Serial.println( resp );
	       m_server->send ( 200, "application/xml", resp  );
	    });

	    m_server->on("/semp/PlanningRequest", HTTP_GET, [this]() {
	      String resp = String(uSEMP::resp_header) + String(makePlanningRequest()) + String(uSEMP::resp_footer) ;
	      Serial.println( "/semp/PlanningRequest" );
//	      Serial.println( resp );
	      m_server->send ( 200, "application/xml", resp );
	    });
	    m_server->on("/semp/", HTTP_POST,  [this]() { handlePowerCtl(); } );
}


const char* uSEMP::makeDeviceStatusRequest()
{

	char *wp =_respBuffer + size_devInfo+1;
	snprintf(wp, size_deviceStatus, deviceStatus_tmpl
          , info.deviceID()
          , /*EMSignalsAccepted*/ "true"
          , /*Status*/ stat.EM_On ? "On" : "Off"
          , /* Power*/ stat.EM_On ? stat.m_averagePwr : 0
          , /* MinPower */ stat.m_minPwr
          , /* MaxPower */ stat.m_maxPwr
          , /* Timestamp */ 0 );
   return wp;
}

PlanningData *uSEMP::getActivePlan()
{
	if ( stat.m_activePlan ) {
		return stat.m_activePlan;
	}
	return 0;
}


const char* uSEMP::makePlanningRequest()
{
  char *wp =_respBuffer+ size_devInfo+size_deviceStatus+2;
  if( stat.m_activePlan->m_requestedEnergy || stat.m_activePlan->m_optionalEnergy  ) {
    PlanningData* plan = getActivePlan();
    unsigned _now = get_time()%(1 DAY);
	int est = stat.m_activePlan->m_earliestStart - _now;  if (est <0) { est =0;  }
	int let = stat.m_activePlan->m_latestEnd - _now;     if (let <0) { let =0;  }
	plan->m_maxOnTime = min( plan->m_maxOnTime, (unsigned)let );
	plan->m_minOnTime = min( plan->m_minOnTime, plan->m_maxOnTime );

	snprintf( wp, size_planningRequest, uSEMP::planningRequest_tmpl
    , info.deviceID()
    , est /*Earliest Begin*/
    , let /* LatestEnd */
    , plan->m_minOnTime  /* MinTime */
    , plan->m_maxOnTime  /* MaxTime */ );
  } else {
	  wp[0] =0;
  }


  //Serial.printf("PLAN: %s\n-------------\n", wp);
  return wp;
}


void PlanningData::updateEnergy(int i_req, int i_optional)
{
	int newReq = m_requestedEnergy + i_req;
	int newOpt = m_optionalEnergy  + i_optional;
	if( newOpt>0)  m_requestedEnergy = newReq;  else m_requestedEnergy = 0;
	if( newReq>0)  m_optionalEnergy  = newOpt;  else m_optionalEnergy  = 0;


	m_minOnTime = KWh2KWs(m_requestedEnergy) / m_maxPwr;
	m_maxOnTime = KWh2KWs(m_optionalEnergy)  / m_maxPwr;

	m_maxOnTime = max( m_minOnTime, m_maxOnTime);
	m_maxOnTime = min(m_maxOnTime, m_latestEnd);
	m_minOnTime = min(m_maxOnTime, m_minOnTime);
}

