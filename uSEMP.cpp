/*
  uSEMP.h - uSEMP library Basic implementation of SEMP Protocol
  Copyright (c) 2020 Udo Hessenauer  All right reserved.
 */

// include this library's description file
#include "uSEMP.h"

const char* uSEMP::scheme_tmpl PROGMEM = "<?xml version=\"1.0\"?>\n"
			"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">\n"
			" <specVersion>\n"
			"   <major>1</major>\n"
			"   <minor>0</minor>\n"
			" </specVersion>\n"
			" <device>\n"
			"   <deviceType>urn:schemas-simple-energy-management-protocol:device:Gateway:1</deviceType>\n"
			"   <friendlyName>%s</friendlyName>\n"
			"   <manufacturer>%s</manufacturer>\n"
			"   <manufacturerURL>%s</manufacturerURL>\n"
			"   <modelDescription>%s</modelDescription>\n"
			"   <modelName>%s</modelName>\n"
			"   <modelNumber>%s</modelNumber>\n"
			"   <modelURL>%s</modelURL>\n"
			"   <UDN>uuid:%s</UDN>\n"
			"   <serviceList>\n"
			"     <service>\n"
			"       <serviceType> urn:schemas-simple-energy-management-protocol:service:NULL:1:service:NULL:1 </serviceType>\n"
			"       <serviceId>urn:schemas-simple-energy-management-protocol:serviceId:NULL:serviceId:NULL </serviceId>\n"
			"       <SCPDURL>/XD/NULL.xml</SCPDURL>\n"
			"       <controlURL>/UD/?0</controlURL>\n"
			"       <eventSubURL></eventSubURL>\n"
			"     </service>\n"
			"   </serviceList>\n"
			"   <presentationURL>%s</presentationURL>\n"
			"   <semp:X_SEMPSERVICE xmlns:semp=\"urn:schemas-simple-energy-management-protocol:service-1-0\">\n"
			"     <semp:server>http://%s:%d</semp:server>\n"
			"     <semp:basePath>/semp</semp:basePath>\n"
			"     <semp:transport>HTTP/Pull</semp:transport>\n"
			"     <semp:exchangeFormat>XML</semp:exchangeFormat>\n"
			"     <semp:wsVersion>1.1.5</semp:wsVersion>\n"
			"     </semp:X_SEMPSERVICE>\n"
			" </device>\n"
			"</root>\n";


const char* uSEMP::resp_tmpl   = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<Device2EM xmlns=\"http://www.sma.de/communication/schema/SEMP/v1\">\r\n%s</Device2EM>";
const char* uSEMP::resp_header = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n<Device2EM xmlns=\"http://www.sma.de/communication/schema/SEMP/v1\">\r\n";
const char* uSEMP::resp_footer = "</Device2EM>";


const char* uSEMP::deviceInfo_tmpl PROGMEM =
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

const char* uSEMP::deviceStatus_tmpl PROGMEM =
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

const char* uSEMP::planningRequest_tmpl PROGMEM =
		" <PlanningRequest>\r\n"
		"   <Timeframe>\r\n"
		"     <DeviceId>%s</DeviceId>\r\n"
		"     <EarliestStart>%d</EarliestStart>\r\n"
		"     <LatestEnd>%d</LatestEnd>\r\n"
		"     <MinRunningTime>%u</MinRunningTime>\r\n"
		"     <MaxRunningTime>%u</MaxRunningTime>\r\n"
		"   </Timeframe>\r\n"
		" </PlanningRequest>\r\n";


const char* uSEMP::time2str( unsigned long theTime, unsigned i_fmt){
	static char timestr[9];
	// print the hour, minute and second:
	switch (i_fmt){
	case 2:	sprintf_P( timestr, PSTR("%2lu:%02lu"), ((theTime  % 86400L) / 3600), ((theTime  % 3600) / 60)); break;
	default:
		sprintf_P( timestr, PSTR("%2lu:%02lu:%02lu"), ((theTime  % 86400L) / 3600), ((theTime  % 3600) / 60), theTime % 60);
	}
	return timestr;
}

// Constructor /////////////////////////////////////////////////////////////////
// Function that handles the creation and setup of instances


#ifdef USE_TINYXML
uSEMP* uSEMP::g_activeSEMP; // active SEMP Object for this request...  Ugly but as long as i use TinyXML...
void uSEMP::xml_cb(uint8_t statusflags, char* tagName,
		uint16_t tagNameLen, char* data, uint16_t dataLen)
{
	if (g_activeSEMP)	g_activeSEMP->XML_callback( statusflags, tagName, tagNameLen, data, dataLen);
}
#endif

uSEMP::uSEMP( const char* i_udn_uuid,const char* i_deviceID, const char* i_deviceName
		, const char* i_deviceSerial, const char* i_deviceType, const char* i_vendor, unsigned i_maxConsumption, unsigned long (*i_getTime)()
		, void (*i_setPwr)( bool i_state ), ESP8266WebServer* i_server, unsigned i_port  )
: stat( i_deviceID, true, i_maxConsumption )
,info( i_udn_uuid, i_deviceID, i_deviceName, i_deviceSerial, i_deviceType, i_vendor)
{
	// initialize this instance's variables
	get__time = i_getTime;
	m_setPwrState = i_setPwr;
	m_server = i_server;
	m_port = i_port;
	m_schemaS = 0;
	size_devInfo            = strlen(deviceInfo_tmpl) + strlen(info.deviceID()) + strlen(info.deviceName()) + strlen(info.deviceSerial() + 3);
	size_deviceStatus	    = strlen(deviceStatus_tmpl) + strlen(info.deviceID()) + 12*32;
	size_planningRequest	= strlen(planningRequest_tmpl) + strlen(info.deviceID()) + 7*8;
	size_SempRequest	= size_devInfo + size_deviceStatus + size_planningRequest + strlen(resp_header)+strlen(resp_footer);
	m_sizeRespBuffer = strlen(resp_header)+strlen(resp_footer)+size_devInfo+size_deviceStatus+ NR_OF_REQUESTS* size_planningRequest;
	m_respBuffer = (char*) malloc( m_sizeRespBuffer + 3);
	stat.m_activePlan = 0; // todo support many plans
#ifdef USE_TINYXML
	m_xml.init((uint8_t *)m_xml_buffer, sizeof(m_xml_buffer), xml_cb );
#endif
}



int uSEMP::dumpPlans(char* o_wp)
{
  char* wp = o_wp;
  PlanningData* activePlan = getActivePlan();
  for ( unsigned idx = 0; idx< NR_OF_REQUESTS; ++idx  )
  {
    PlanningData* plan = getPlan(idx);
    if (plan){
      unsigned est = plan->m_earliestStart;
      unsigned let = plan->m_latestEnd;
      char act = plan->used() ? ((plan == activePlan) ? '*' : '-') : ' ';
     // Serial.printf("%d-- plan %p vs active %p  -> %s\n", idx, plan, activePlan, (plan == activePlan) ? "match!!" : "---");
      wp += sprintf_P(wp,PSTR("%u:%5s-"), idx, time2str( est, 2) );
      wp += sprintf_P(wp,PSTR("%5s%c%u\n"), time2str( let, 2), act, plan->m_maxOnTime ); // , plan->m_requestedEnergy, plan->m_optionalEnergy);
    }
  }
  wp += sprintf_P(wp, PSTR("\n--------------------\n"));
  wp += sprintf_P(wp, PSTR("* %s *"),  time2str( getTime() ) );
  return wp - o_wp;
}

const char* uSEMP::makeSsdpScheme( ssdp_cfg* i_ssdpcfg)
{
	size_t sizeOfScheme = strlen_P(scheme_tmpl);
	sizeOfScheme += strlen( i_ssdpcfg->deviceName);
	sizeOfScheme += strlen( i_ssdpcfg->manufacturer);
	sizeOfScheme += strlen( i_ssdpcfg->manufacturererURL);
	sizeOfScheme += strlen( i_ssdpcfg->description);
	sizeOfScheme += strlen( i_ssdpcfg->modelName);
	sizeOfScheme += strlen( i_ssdpcfg->modelNr);
	sizeOfScheme += strlen( i_ssdpcfg->modelURL);
	sizeOfScheme += strlen( i_ssdpcfg->udn_uuid);
	sizeOfScheme += strlen( i_ssdpcfg->presentationURL);
	sizeOfScheme += strlen( i_ssdpcfg->IP);
	sizeOfScheme += 5;  /* len of portNr */
	sizeOfScheme += 1 - 10 * 2;  /* minus format-strings(10 a 2 chars) terminating NULL */


	if (m_schemaS ) delete [] m_schemaS;
	m_schemaS = new char[sizeOfScheme];

	snprintf_P( m_schemaS, sizeOfScheme, scheme_tmpl
			,i_ssdpcfg->deviceName
			,i_ssdpcfg->manufacturer
			,i_ssdpcfg->manufacturererURL
			,i_ssdpcfg->description
			,i_ssdpcfg->modelName
			,i_ssdpcfg->modelNr
			,i_ssdpcfg->modelURL
			,i_ssdpcfg->udn_uuid
			,i_ssdpcfg->presentationURL
			,i_ssdpcfg->IP
			,m_port);

	return m_schemaS;
}

#ifdef USE_TINYXML
void uSEMP::XML_callback(uint8_t statusflags, char* tagName,
		uint16_t tagNameLen, char* data, uint16_t dataLen)
{
	char dbuf[dataLen+1];
	char tbuf[tagNameLen+1];
	memcpy(dbuf, data,    dataLen );      dbuf[dataLen] = 0;
	memcpy(tbuf, tagName, tagNameLen );   tbuf[tagNameLen] = 0;

	if(statusflags & STATUS_TAG_TEXT) {
		if (!strcasecmp_P(tagName, PSTR("/EM2Device/DeviceControl/On"))) {
			if ( strncmp(dbuf, "true", 4) == 0 ) {
				setPwrState( HIGH );
			} else {
				setPwrState( LOW );
			}
			if ( m_setPwrState ) m_setPwrState( stat.EM_On );
		}
	}
}

#endif

#ifdef USE_ASYNC
void uSEMP::handlePowerCtl(AsyncWebServerRequest *request)
{
	//Serial.printf_P(PSTR("uSEMP PWR CTL request /\n"));
	request->getParam(PARAM_INPUT_1)->value();
	for( int n = 0; n < m_server->args(); ++n)
	{
		String p1Name = m_server->argName(n);
		String p1Val = m_server->arg(n);
		//Serial.printf_P(PSTR("p%dName: %s  val: %s\n"),n, p1Name.c_str(), p1Val.c_str() );
		if (p1Name == "plain")
		{
#ifdef USE_TINYXML
			g_activeSEMP = this; //this is nasty stateful code, but w/o modifying TinyXML.....
			const char* ch = p1Val.c_str();
			while(*ch) {
				m_xml.processChar(*ch);
				++ch;
			}

#else
			int idx = p1Val.indexOf("<On>",sizeof(resp_header));
			if ( idx >= 0) {
				String cmd =  p1Val.substring(idx+4,idx+4+4);
				if ( cmd == "true" ) {
					setPwrState( HIGH );
				} else {
					setPwrState( LOW );
				}
				if ( m_setPwrState ) m_setPwrState( stat.EM_On );
			}

#endif
			m_server->send ( 200, "application/xml", "<Device2EM></Device2EM>"  );
			return;
		} else {

		}
	}
	m_server->send ( 400, "application/xml", "<Device2EM></Device2EM>"  );
	return;
}

void uSEMP::handleNotFound()
{
	m_server->send ( 404, "text/plain", "not found" );
}
#else
void uSEMP::handlePowerCtl()
{
	//Serial.printf_P(PSTR("uSEMP PWR CTL request /\n"));
	for( int n = 0; n < m_server->args(); ++n)
	{
		String p1Name = m_server->argName(n);
		String p1Val = m_server->arg(n);
		//Serial.printf_P(PSTR("p%dName: %s  val: %s\n"),n, p1Name.c_str(), p1Val.c_str() );
		if (p1Name == "plain")
		{
#ifdef USE_TINYXML
			g_activeSEMP = this; //this is nasty stateful code, but w/o modifying TinyXML.....
			const char* ch = p1Val.c_str();
			while(*ch) {
				m_xml.processChar(*ch);
				++ch;
			}

#else
			int idx = p1Val.indexOf("<On>",sizeof(resp_header));
			if ( idx >= 0) {
				String cmd =  p1Val.substring(idx+4,idx+4+4);
				if ( cmd == "true" ) {
					setPwrState( HIGH );
				} else {
					setPwrState( LOW );
				}
				if ( m_setPwrState ) m_setPwrState( stat.EM_On );
			}

#endif
			m_server->send ( 200, "application/xml", "<Device2EM></Device2EM>"  );
			return;
		} else {

		}
	}
	m_server->send ( 400, "application/xml", "<Device2EM></Device2EM>"  );
	return;
}

void uSEMP::handleNotFound()
{
	m_server->send ( 404, "text/plain", "not found" );
}

#endif
void uSEMP::startService( ) {

	m_server->on("/semp/", HTTP_GET, [this]() {
		Serial.println("----------------------------------------------------------------------SEMP request /\n");

		unsigned wp = 0;

		//#define WR_RESP( args...)

		wp += snprintf_P(&m_respBuffer[wp], m_sizeRespBuffer-wp, "%s", resp_header );
		wp += snprintf_P(&m_respBuffer[wp], m_sizeRespBuffer-wp, deviceInfo_tmpl, info.deviceID(), info.deviceName()
				, info.deviceSerial(), info.deviceType(), info.vendor()
				, stat.m_maxPwr );
		wp += makeDeviceStatusRequest(&m_respBuffer[wp]);
		wp += makePlanningRequests( getTime(), &m_respBuffer[wp]);
		wp += snprintf_P(&m_respBuffer[wp], m_sizeRespBuffer-wp, "%s", resp_footer );


		m_server->send ( 200, "application/xml", m_respBuffer );
	});
	m_server->on("/semp/DeviceInfo", HTTP_GET, [this]() {
		unsigned wp = 0;
		wp += snprintf_P(&m_respBuffer[wp], m_sizeRespBuffer-wp, "%s", resp_header );
		wp += snprintf_P(&m_respBuffer[wp], m_sizeRespBuffer-wp, deviceInfo_tmpl, info.deviceID(), info.deviceName()
				, info.deviceSerial(), info.deviceType(),  info.vendor(), stat.m_maxPwr );
		wp += snprintf_P(&m_respBuffer[wp], m_sizeRespBuffer-wp, "%s",resp_footer );

		m_server->send ( 200, "application/xml", m_respBuffer );
	});
	m_server->on("/semp/DeviceStatus", HTTP_GET, [this]() {
		unsigned wp = 0;
		wp += snprintf_P(&m_respBuffer[wp], m_sizeRespBuffer-wp, "%s",resp_header );
		wp += makeDeviceStatusRequest(&m_respBuffer[wp]);
		wp += snprintf_P(&m_respBuffer[wp], m_sizeRespBuffer-wp, "%s",resp_footer );

		m_server->send ( 200, "application/xml", m_respBuffer );
	});

	m_server->on("/semp/PlanningRequest", HTTP_GET, [this]() {
		unsigned wp = 0;
		wp += snprintf_P(&m_respBuffer[wp], m_sizeRespBuffer-wp, "%s",resp_header );
		wp += makePlanningRequests(getTime(), &m_respBuffer[wp]);
		wp += snprintf_P(&m_respBuffer[wp], m_sizeRespBuffer-wp, "%s",resp_footer );
		m_server->send ( 200, "application/xml", m_respBuffer );
	});
	m_server->on("/semp/", HTTP_POST,  [this]() {
		Serial.println(F("SEMP control /\n"));
		handlePowerCtl(); } );

	//m_server->onNotFound( handleNotFound );
	m_server->begin();
}


int uSEMP::makeDeviceStatusRequest(char *o_buf)
{
	return snprintf_P(o_buf, size_deviceStatus, deviceStatus_tmpl
			, info.deviceID()
			, /*EMSignalsAccepted*/ "true"
			, /*Status*/ stat.EM_On ? "On" : "Off"
					, /* Power*/ stat.EM_On ? stat.m_averagePwr : 0
							, /* MinPower */ stat.m_minPwr
							, /* MaxPower */ stat.m_maxPwr
							, /* Timestamp */ 0 );
}

PlanningData *uSEMP::getActivePlan()
{

	unsigned _now = getTime();
	if ( stat.m_activePlan )  {
		if ( !stat.m_activePlan->updateEnergy(_now) )  {
			setPwrState( LOW );
			stat.m_activePlan = 0;
		}
	}

	if ( !stat.m_activePlan ) {
		PlanningData* pl_e = 0;
		for ( unsigned idx = 0; idx< NR_OF_REQUESTS; ++idx  ){
			PlanningData* pl = &m_plans[idx];
			if ( pl->updateEnergy(_now )) {
				//if ( pl_e) Serial.printf("used plan %d  check %u vs %u\n", idx, pl_e->m_earliestStart, pl->m_earliestStart);
				if ( !pl_e || (pl_e->m_earliestStart > pl->m_earliestStart) ) {
					pl_e = pl;
				}
			}
		}
		stat.m_activePlan = pl_e;
	}
	return stat.m_activePlan;
}


int uSEMP::modifyPlan(unsigned i_plan, unsigned long i_now, unsigned i_req, unsigned i_opt, unsigned i_est, unsigned i_let )
{
	int usedPlan = i_plan < NR_OF_REQUESTS ? int(i_plan) : -1;
	//Serial.printf("modifyPlan uSEMP: %s   req:%u  opt: %u\n", time2str(i_now), i_req, i_opt);

	PlanningData* plan = &m_plans[usedPlan];
	plan->requestEnergy(i_now, i_req, i_opt, i_est, i_let, stat.m_maxConsumption);
	// if no active plan, then let t getActivePlan() determine the next active plan
	if ( !stat.EM_On ) {
		stat.m_activePlan = 0;
	} // else the active Plan shouldn't be cancelled.  SEMP spec demands planning request must not overlap
	return usedPlan;
}


int uSEMP::requestEnergy(unsigned long i_now, unsigned i_req, unsigned i_opt,
		unsigned i_est, unsigned i_let)
{
	for ( unsigned idx = 0; (idx< NR_OF_REQUESTS); ++idx  ){
		if ( !m_plans[idx].used() ) {
			return modifyPlan(idx, i_now, i_req, i_opt, i_est, i_let );
		}
	}

	return -1;
}


int uSEMP::resetPlan(int i_plan)
{
	unsigned idx = unsigned(i_plan);
	if ( idx < NR_OF_REQUESTS) {
		m_plans[idx].reset();
		return 0;
	}
	return -1;
}


void uSEMP::deleteAllPlans()
{
	for ( unsigned idx = 0; idx < NR_OF_REQUESTS; ++idx) {
		m_plans[idx].reset();
	}
}


/**
 * make a planning request fron an existing plan
 */
int uSEMP::makeRequestFromPlan(unsigned long i_now, PlanningData* i_plan, char* o_wp)
{
	int ret=0;
	if ( i_plan ) {
		//		Serial.printf("makeRequestFromPlan %p\n", i_plan);
		if( i_plan->m_requestedEnergy || i_plan->m_optionalEnergy  ) {
//			unsigned _now = getTime();
			int est = i_plan->m_earliestStart - i_now;  if (est <0) { est =0;  }
			int let = i_plan->m_latestEnd - i_now;     if (let <0) { let =0;  }
			i_plan->m_maxOnTime = min( i_plan->m_maxOnTime, (unsigned)let );
			i_plan->m_minOnTime = min( i_plan->m_minOnTime, i_plan->m_maxOnTime );

			ret = snprintf_P( o_wp, size_planningRequest, uSEMP::planningRequest_tmpl
					, info.deviceID()
					, est /*Earliest Begin*/
					, let /* LatestEnd */
					, i_plan->m_minOnTime  /* MinTime */
					, i_plan->m_maxOnTime  /* MaxTime */ );
		} else {
			o_wp[0] =0;
		}

		//Serial.printf("PLAN: %s\n-------------\n", wp);

	}
	return ret;
}

void uSEMP::updateEnergy(unsigned long i_now, int i_req, int i_optional)
{
	if ( stat.m_activePlan ) {
//		Serial.printf_P(PSTR("Update: %s  req:%d->%uWh(%us)  opt:%d->%uWh(%us) \n"), time2str(i_now)
//				, i_req,      stat.m_activePlan->m_requestedEnergy, stat.m_activePlan->m_minOnTime
//				, i_optional, stat.m_activePlan->m_optionalEnergy,  stat.m_activePlan->m_maxOnTime);

		if (!stat.m_activePlan->updateEnergy(i_now, i_req, i_optional ))
		{
			setPwrState( LOW );
			stat.m_activePlan = 0;
		}
	}

}

void uSEMP::loop() {
	m_server->handleClient();
}

int uSEMP::makePlanningRequests(unsigned long i_now, char* o_buf)
{
	int wp = 0;
	o_buf[wp]=0;

	for ( unsigned idx = 0; idx< NR_OF_REQUESTS; ++idx  )
	{
		PlanningData* pl = &m_plans[idx];
		if( pl->used() ) {
			if (pl->updateEnergy(i_now)) {
				wp += makeRequestFromPlan( i_now, pl, &o_buf[wp]);
			} else {
				// just passed... maybe we should switch OFF?
				if ( pl == stat.m_activePlan ) {
					setPwrState( LOW );
					stat.m_activePlan = 0;
				}
			}
		}
	}
	return wp;
}

bool PlanningData::updateEnergy(unsigned long i_now, int i_req, int i_opt)
{
	if( m_used ) {
		int newReq = m_requestedEnergy + i_req;
		int newOpt = m_optionalEnergy  + i_opt;
		if( newReq>0)  m_requestedEnergy = newReq;  else m_requestedEnergy = 0;
		if( newOpt>0)  m_optionalEnergy  = newOpt;  else m_optionalEnergy  = 0;


		m_minOnTime = Wh2Ws(m_requestedEnergy) / m_maxPwr;
		m_maxOnTime = Wh2Ws(m_optionalEnergy)  / m_maxPwr;

		int let = m_latestEnd - i_now;
		//		Serial.printf("------> let: %s\n", time2str(let));
		if (let <0) {
			let =0;
		}

		m_maxOnTime = max(m_minOnTime, m_maxOnTime);
		m_maxOnTime = min(m_maxOnTime, (unsigned)let);
		m_minOnTime = min(m_maxOnTime, m_minOnTime);
		// reset if this plan is past time

		if (m_maxOnTime == 0 ) {
			reset();
		}
	}
	return used();
}

int PlanningData::show(char* o_wp, size_t i_maxlen)
{
	return i_maxlen ? snprintf_P(o_wp, i_maxlen-1, PSTR(" %s\nmin: %u\nmax: %u\nest: %s\nlst: %s\npwr: %u\nreq: %u\nopt: %u\n"), m_used ? "used" :"free"
			, m_minOnTime, m_maxOnTime, time2str(m_earliestStart)
			, time2str(m_latestEnd), m_maxPwr, m_requestedEnergy, m_optionalEnergy) : 0;

}

PlanningData* PlanningData::requestEnergy(unsigned long i_now, unsigned i_req, unsigned i_opt,
		unsigned i_est, unsigned i_let, unsigned i_maxPwr)
{
	m_used = true;		// now this plan is used
	m_earliestStart = i_est;
	m_latestEnd     = i_let;
	m_maxPwr = i_maxPwr;
	m_requestedEnergy = 0 KWh; // update works differentially so set inital
	m_optionalEnergy  = 0 KWh; // value to 0 and let updateEnergy do the Rest
	updateEnergy(i_now, i_req, i_opt); // diff

	return this;
}

PlanningData* PlanningData::requestTime(unsigned long i_now,
		unsigned i_minOnTime, unsigned i_maxOnTime, unsigned i_est,
		unsigned i_let, unsigned i_maxPwr) {

return 0;

}
