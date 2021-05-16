/**************************************************************************
 **************************************************************************/
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <uSEMP.h>

#define DEVICE_SERIAL_NR    2
#define DEVICE_NAME "AdditionalEnergyRequester"
#define HOSTNAME "AddER" 
#define VENDOR   "HesTec"
 
// Device 00 Params
#define MAX_CONSUMPTION  3700

#define SERIAL_BAUDRATE       115200

/*--------helper macros -----------*/


#define DIM(a) (sizeof(a)/sizeof(a[0]))

// GPIOs
//extern void newTasklet(void (*i_tsklt)(void), unsigned long i_inhibit=0);
const int ledPin   =  LED_BUILTIN; // 15;
const int buttonPin = 0;

int ledState   = HIGH;
char ChipID[8+1];
char udn_uuid[36+1]; 
char DeviceID[26+1];
char DeviceSerial[4+1];


#define SEMP_PORT 9980
ESP8266WebServer http_server(80); 
ESP8266WebServer semp_server(SEMP_PORT);



uSEMP* g_semp;
void setup() {
  Serial.begin(SERIAL_BAUDRATE);

   // SEMP init
  snprintf( ChipID, sizeof(ChipID), "%08x", ESP.getChipId() );
  snprintf( udn_uuid, sizeof(udn_uuid), "f1d67bee-2a4e-d608-ffff-aeee%08x", ESP.getChipId() );
  snprintf( DeviceID , sizeof(DeviceID), "F-30021968-0001%08x-00", ESP.getChipId() );
  snprintf( DeviceSerial , sizeof(DeviceSerial), "%04d", DEVICE_SERIAL_NR );
  Serial.printf("ChipID: %s\n", ChipID);
  
  g_semp = new uSEMP( udn_uuid, DeviceID, DEVICE_NAME, DeviceSerial, "Other", VENDOR, MAX_CONSUMPTION, false, false , false, 
                      &semp_server, SEMP_PORT );
  Serial.printf("uuid  : %s\n", g_semp->udn_uuid());
  Serial.printf("DevID : %s\n", g_semp->deviceID());


  
  setupWIFI();
  setupPOW(); 
  g_semp->setCallbacks( getTime ,([]( EM_state_t ) {  /*  EM state update from uSEMP*/  }),([]( ) { /* end of Plan*/  }));
  g_semp->acceptEMSignal( true );
           g_semp->setEmState( EM_ON );
  
  setupTimeClk();
  setupIO();
  setupWebSrv();

  setupSSDP();


  // request the extra energy 
  requestDailyPlan(0);
}


static unsigned long ltime=0;

const char* state2txt( int state)
{
  return state== HIGH ? "HIGH" : "LOW";
}

void loop() 
{
  loopIO();
  loopWebSrv();
  unsigned long now = getTime();
  if ( now/10 != ltime/10  ) {
    ledState = !ledState;
    char buffer[10*40];
    char* wp = &buffer[0];
    
    PlanningData* activePlan = g_semp->getActivePlan();
    for ( unsigned idx = 0; idx< NR_OF_REQUESTS; ++idx  )
    {     
      PlanningData* plan = g_semp->getPlan(idx);
      if (plan){
        unsigned est = plan->m_earliestStart;
        unsigned let = plan->m_latestEnd;
        char act = plan->used() ? ((plan == activePlan) ? '*' : '-') : ' ';
       // Serial.printf("%d-- plan %p vs active %p  -> %s\n", idx, plan, activePlan, (plan == activePlan) ? "match!!" : "---");
        wp += sprintf(wp,"%u:%5s-%5s%c%u\n", idx
        , String(getTimeStringS(est)).c_str(), String(getTimeStringS(let)).c_str(), act, plan->m_maxOnTime ); // , plan->m_requestedEnergy, plan->m_optionalEnergy);
      }
    }
    wp += sprintf(wp, "\n--------------------\n");
    wp += sprintf(wp, "* %s *",  getTimeString( getTime() ) );
   
 
    Serial.printf("%s\n", buffer );
 
    ltime = now;
  }
  
  loopPOW();
  g_semp->loop();
}



/////////////////////
// . Example Text
