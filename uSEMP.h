/*
  uSEMP.h - Basic implementation of SEMP Protocol
  Copyright (c) 2020 Udo Hessenauer  All right reserved.
 */

// ensure this library description is only included once
#ifndef USEMP_H
#define USEMP_H
#define _SEMP_DEBUG
#ifdef SEMP_DEBUG
# define DBG_TRACE(...) Serial.printf(__VA_ARGS__)
# define DBG_TRACE_P(...) Serial.printf_P(__VA_ARGS__)
#else
# define DBG_TRACE(...)
# define DBG_TRACE_P(...)
#endif
#define _DEBUG_PRINT(...)

#include <Arduino.h>
#include "pgmspace.h"

#if defined(ESP8266)
/* ESP8266 Dependencies */
#  include <ESP8266WiFi.h>
#  include <ESP8266mDNS.h>
#  include <ESP8266SSDP.h>
#elif defined(ESP32)
/* ESP32 Dependencies */
#  include <WiFi.h>
#endif

#define _USE_ASYNC
#ifdef USE_ASYNC
#if defined(ESP8266)
/* ESP8266 Dependencies */
#	include <ESP8266WiFi.h>
#	include <ESPAsyncTCP.h>
#	include <ESPAsyncWebServer.h>
#elif defined(ESP32)
/* ESP32 Dependencies */
#	include <WiFi.h>
#	include <AsyncTCP.h>
#	include <ESPAsyncWebServer.h>
#endif
typedef AsyncWebServer WebServer_T;
#else
#	include <ESP8266WebServer.h>
typedef ESP8266WebServer WebServer_T;
#endif

// --- helper macros -------
#define KWh  *1000
#define KWh2KWs(kwh)  (kwh*3600)
#define Wh2Ws(wh) 	  (wh*3600)
#define DAY *(24*3600)
// ----  config
#define NR_OF_REQUESTS 4


// library interface descriptio


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


typedef enum {
     EM_OFFLINE=-1,
     EM_ON,
     EM_OFF
} EM_state_t;


class PlanningData {
protected:
    bool m_used;
    bool m_timeBased;
    unsigned long m_lastTime;    //< timestamp of last modifications
public:
    unsigned m_minOnTime;
    unsigned m_maxOnTime;
    unsigned long m_earliestStart;
    unsigned long m_latestEnd;
    unsigned m_maxPwr;
    unsigned m_requestedEnergy = 0 KWh;
    unsigned m_optionalEnergy  = 0 KWh;

    PlanningData* m_next;


    int show(char *o_wp, size_t i_maxlen);

    void reset()
    {
        DBG_TRACE("reset plan\n");
        m_used = false;
        m_timeBased = false;
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

    /**
     * @return  true if plan is used
     */
    bool used() { return m_used; }


    /**
     * @return  latest end
     */
    unsigned long end() { return m_latestEnd; }


    /**
     * @return true if this plan is still requesting energy
     */
    bool is_active();

    /**
     * @return true if this plan is timebased
     */
    bool is_timebased() { return m_timeBased; }
    /**
     * update Energy of the active (most actual) energy request
     * @param  i_now        the timestamp, for the actualizeation
     * @param  i_pwrOn      true if Powerstate is ON -> timebase requests modify onTime only if PowerSwitch in ON
     * @param  i_req        difference of requested energy - value is added to reguested energy of this request
     * @param  i_optional   difference of optional energy - value is added to optional energy of this request
     * @param  i_prolong    prolong latest end time by i_prolong[s]
     *
     */
    bool updateEnergy(unsigned long i_now, bool i_pwrOn, int i_req=0, int i_optional=0, int i_prolong=0 );



    /**
     *  request energy
     *  @param  i_now		timestamp in seconds ( e.g. unix time seconds since 1970... )
     *  @param  i_req 		requested energy in [Wh]
     *  @param  i_optional		optional energy in [Wh]
     *  @param  i_est		earliest start time in seconds (absolute timestamp like i_now)
     *  @param  i_let		latest end time in seconds (absolute timestamp like i_now)
     *  @param  i_maxPwr    max power drawn by device
     *
     *  @return handle to created plan, -1 if creation failed
     */
    PlanningData *requestEnergy(unsigned long i_now, unsigned i_req, unsigned i_optional, unsigned i_est, unsigned i_let, unsigned i_maxPwr);

    /**
     *  @param  i_now       timestamp in seconds ( e.g. unix time seconds since 1970... )
     *  @param  i_minOnTime requested energy in [s]
     *  @param  i_maxOnTime optional energy in [s]
     *  @param  i_est       earliest start time in seconds (absolute timestamp like i_now)
     *  @param  i_let       latest end time in seconds (absolute timestamp like i_now)
     *  @param  i_maxPwr    max power drawn by device
     */
    PlanningData *requestTime(unsigned long i_now, unsigned i_minOnTime, unsigned i_maxOnTime, unsigned i_est, unsigned i_let, unsigned i_maxPwr);
};



class DeviceInfo {

    //const char* ChipID;
    const char* Udn_uuid;
    const char* DeviceID;
    const char* DeviceType;
    const char* DeviceName;
    const char* DeviceSerial;
    const char* Vendor;
    unsigned    MaxConsumption;
    bool        Interruptible;
    bool        AcceptOptional;
    bool        AbsoluteTimeStamps;


public:
    DeviceInfo( const char* i_udn_uuid,const char* i_deviceID
            ,const char* i_deviceName, const char* i_deviceType
            ,const char* i_deviceSerial
            ,const char* i_vendor
            ,unsigned i_maxConsumption
            ,bool i_interruptible=true
            ,bool i_acceptOptional=true
            ,bool i_absoluteTimeStamps=false )
    {
        Udn_uuid = i_udn_uuid;
        DeviceID = i_deviceID;
        DeviceSerial = i_deviceSerial;
        DeviceName = i_deviceName;
        DeviceType = i_deviceType;
        Vendor = i_vendor;
        MaxConsumption = i_maxConsumption;
        Interruptible = i_interruptible;
        AcceptOptional = i_acceptOptional;
        AbsoluteTimeStamps = i_absoluteTimeStamps;
    }

    const char* udn_uuid()      const { return Udn_uuid; };
    const char* deviceID()      const { return DeviceID;}
    const char* deviceName()    const { return DeviceName;}
    const char* deviceType()    const { return DeviceType;}
    const char* deviceSerial()  const { return DeviceSerial;}
    const char* vendor()        const { return Vendor;}
    unsigned    maxConsumption() const { return MaxConsumption;};
    bool        interruptible() const { return Interruptible; }
    bool        acceptOptional()    const { return AcceptOptional; }
    bool        absoluteTimeStamps()    const { return AbsoluteTimeStamps; }

    //friend class uSEMP;
};

class DeviceStatus {
public:

    const char* m_Id;
    bool m_acceptEMSignal;
    EM_state_t EM_stat;

    unsigned m_averagePwr;
    unsigned m_minPwr;
    unsigned m_maxPwr;

    PlanningData* m_activePlan;

public:

    DeviceStatus( const char* i_id, bool i_acceptEMS )
    {
        m_Id = i_id;
        m_acceptEMSignal = i_acceptEMS;
        m_averagePwr = 0;
        m_minPwr = 0;
        m_maxPwr = 0;
        m_activePlan = 0;
        EM_stat = EM_OFFLINE;
    }

};

class uSEMP
{
    static const char*  scheme_tmpl;
    static const char* 	resp_tmpl;
    static const char* 	resp_header;
    static const char* 	resp_footer;

    static const char* 	deviceInfo_tmpl;
    static const char* 	deviceStatus_tmpl;
    static const char* 	planningRequest_tmpl;


    unsigned long (*get__time)();

    unsigned long getTime(){
        if (get__time) 	return get__time();// %(1 DAY);
        else			return 0;
    }
    void handleNotFound();
protected:


public:

    enum DevType {
        AirConditioning=0,
        ElectricVehicle,
        Charger,
        DishWasher,
        Dryer,
        EVCharger,
        Fridge,
        Heater,
        HeatPump,
        Motor,
        Pump,
        WashingMachine,
        Other,
        //---------
        NrOfDevTypes
    };



    DeviceStatus  		stat;
    DeviceInfo	  		info;
    WebServer_T* 	    m_server;
    unsigned 			m_port;
    char* 				m_schemaS;

    unsigned size_devInfo;
    unsigned size_deviceStatus;
    unsigned size_planningRequest;
    unsigned size_SempRequest;

    char*  m_respBuffer;
    size_t m_sizeRespBuffer;
    void (*m_signalEmState)( EM_state_t i_state );
    void (*m_signalEndOfPlan)( );

    PlanningData 	m_plans[NR_OF_REQUESTS];

    // user-accessible "public" interface

    uSEMP( const char* i_udn_uuid,const char* i_deviceID, const char* i_deviceName, const char* i_deviceType
            , const char* i_deviceSerial, const char* i_vendor, unsigned i_maxConsumption
            , bool i_interruptible, bool i_acceptOptional
            ,WebServer_T* i_server, unsigned i_port  );

    const char* makeSsdpScheme( ssdp_cfg* i_ssdpcfg);
    void setCallbacks( unsigned long (*i_getTime)()
            , void (*i_sigEmS)( EM_state_t )
            , void (*i_sigEoP)( ) ) {
        get__time = i_getTime;
        m_signalEmState = i_sigEmS;
        m_signalEndOfPlan = i_sigEoP;
    }
    void handlePowerCtl();
    void startService( );
    void loop();

    const char* udn_uuid()      const { return info.udn_uuid(); };
    const char* deviceID()      const { return info.deviceID();}
    const char* deviceName()    const { return info.deviceName();}
    const char* deviceType()    const { return info.deviceType();}
    const char* deviceSerial()  const { return info.deviceSerial();}
    const char* vendor()        const { return info.vendor();};

    static const char* time2str( unsigned long theTime, unsigned i_fmt=0 );
    static const char* devTypeStr( unsigned i_type);

    /**
     *  request energy
     *  @param  i_now       timestamp in seconds ( e.g. unix time seconds since 1970... )
     *  @param  i_req       requested energy in [Wh]
     *  @param  i_opt       optional energy in [Wh]
     *  @param  i_est       earliest start time in seconds (absolute timestamp like i_now)
     *  @param  i_let       latest end time in seconds (absolute timestamp like i_now)
     *
     *  @return handle to created plan, -1 if creation failed
     */
    int requestEnergy(unsigned long i_now, unsigned i_req, unsigned i_opt, unsigned i_est, unsigned i_let );

    /**
     *  request time
     *  @param  i_now       timestamp in seconds ( e.g. unix time seconds since 1970... )
     *  @param  i_minOnTime       requested minumum on time in[s]
     *  @param  i_maxOnTime   requested maximum/optional on time in[s]
     *  @param  i_est       earliest start time in seconds (absolute timestamp like i_now)
     *  @param  i_let       latest end time in seconds (absolute timestamp like i_now)
     *
     *  @return handle to created plan, -1 if creation failed
     */
    int requestTime(unsigned long i_now, unsigned i_minOnTime, unsigned i_maxOnTime, unsigned i_est, unsigned i_let );

    /**
     *  modify an energy request / plan
     *  @param  i_plan		handle of plan to midify
     *  @param  i_now		timestamp in seconds ( e.g. unix time seconds since 1970... )
     *  @param  i_req 		requested energy in [Wh]
     *  @param  i_opt       optional energy in [Wh]
     *  @param  i_est		earliest start time in seconds (absolute timestamp like i_now)
     *  @param  i_let		latest end time in seconds (absolute timestamp like i_now)
     *
     *  @return handle to created plan, -1 if creation failed
     */
    int modifyPlan(unsigned i_plan, unsigned long i_now, unsigned i_req, unsigned i_opt, unsigned i_est, unsigned i_let );

    /**
     *  mondify an time request / plan
     *  @param  i_plan      handle of plan to midify
     *  @param  i_now       timestamp in seconds ( e.g. unix time seconds since 1970... )
     *  @param  i_min       minimal on time in [s]
     *  @param  i_max       maximum/optional on time in [s]
     *  @param  i_est       earliest start time in seconds (absolute timestamp like i_now)
     *  @param  i_let       latest end time in seconds (absolute timestamp like i_now)
     *
     *  @return handle to created plan, -1 if creation failed
     */
    int modifyPlanTime(unsigned i_plan, unsigned long i_now, unsigned i_min, unsigned i_max, unsigned i_est, unsigned i_let );

    /**
     *  update runtime/ energy (differentially )
     *  @param  i_now		timestamp in seconds ( e.g. unix time seconds since 1970... )
     *  @param  i_req 		change of required  energy in [Wh]
     *  @param  i_optional	change of optional  energy in [Wh]
     * @param  i_prolong    prolong latest end time by i_prolong[s]
     */
    void updateEnergy(unsigned long i_now, int i_req, int i_optional, int i_prolong=0);

    /**
     *  update runtime for timebased requests / timeframes
     *  @param  i_now       timestamp in seconds ( e.g. unix time seconds since 1970... )
     *  @param  i_pwrOn     true if relay is on
     */
    void updateTime( unsigned long i_now, bool i_pwrOn );

    /**
     * write a readable dump of all plans to o_wp
     * @param o_wp 	pointer to a sufficiently large buffer
     * 				One Plan is less than 40 chars  plus 2 lines a 40 chars
     *
     * @return		bytes written in o_wp
     */
    int dumpPlans(char* o_wp);

    /**
     * @brief 	get the most actual Plan/energy request
     * @return  the active plan
     */
    PlanningData*	getActivePlan();

    /**
     * @param	idx		the index/handle of a specific plan
     * @return 	pointer to the plan referenced by idx
     */
    PlanningData*	getPlan(unsigned idx) { return &m_plans[idx]; }


    /**
     * reset the plan refereced by
     * @param  i_plan	the index/handle of a specific plan
     * @return 0 if successful / -1 otherwise
     */
    int resetPlan(int i_plan=-1);

    /**
     * reset all plans
     */
    void deleteAllPlans();

    /**
     * @param  true -> EM suggestions are accepted
     */
    void acceptEMSignal( bool i_accept ) { stat.m_acceptEMSignal = i_accept; }

    /**
     * * set state of device  ON, OFF or OFFLINE
     * @param i_state of Device  EM_ON,EM_OFF, OFFLINE
     */
    void setEmState( EM_state_t i_state ){  stat.EM_stat = i_state; }

    /**
     * set state of device  ON or OFF
     * @param i_state of Device  true == EM_ON, false == EM_OFF
     */
    void setEmState( bool i_state ){  stat.EM_stat = i_state ? EM_ON: EM_OFF; }

    /**
     * @return state of Device  ON,OFF, OFFLINE
     * Offline = Device not communicating with gateway this is not really used (yet)
     */
    EM_state_t getEmState() { return stat.EM_stat; }


    void setPwr( unsigned i_pwr, unsigned i_min, unsigned i_max) {
        stat.m_averagePwr = i_pwr;
        stat.m_minPwr = i_min;
        stat.m_maxPwr = i_max;
    }
    // library-accessible "private" interface
private:


    int	makeDeviceStatusRequest(char* o_wp);
    int	makeRequestFromPlan( unsigned long i_now, PlanningData* i_plan, char *o_wp);//
    int makePlanningRequests( unsigned long i_now, char* o_wp);//
    void updateEMstat( EM_state_t i_state ) {
         stat.EM_stat = i_state;
         DBG_TRACE("setPwrState(%s)\n",(stat.EM_stat==EM_OFFLINE ? "OFFLINE" : (stat.EM_stat==EM_ON? "ON":"OFF") ));
         if(m_signalEmState) m_signalEmState( stat.EM_stat );
     }

    void endOfPlan();
};

#endif

