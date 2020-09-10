

// Check values every 2 seconds
#define UPDATE_TIME                     3000

long _cumulatedEnergy;


void unblockingDelay(unsigned long mseconds) {
    unsigned long timeout = millis();
    while ((millis() - timeout) < mseconds) delay(1);
}

double   pwrFactor;
double   pwrMultiplier;
double   currentMultiplier;
double   voltageMultiplier;

#define REF_PWR 5.0
#define REF_VOLT 230.0

#define DIM_PWRS ((60000+500)/UPDATE_TIME)

unsigned long sumPwr;
unsigned minPwrIdx,maxPwrIdx;
unsigned minPwr,_minPwr;
unsigned maxPwr,_maxPwr;
unsigned powers[DIM_PWRS];
unsigned pwrIdx;  


void setupPOW() {
    // Open the relay to switch on the load
    relayState = LOW;
    
    g_semp->startService( &http_server, setPwr );
    g_semp->setPwrState( relayState );
    pwrIdx = 0;
    averagePwr = 0;
    _minPwr=minPwr=_maxPwr=maxPwr=minPwrIdx=maxPwrIdx = 0;

    for(unsigned  n=0; n<DIM(powers); ++n)
    { 
      powers[n] = 0;
    }

//

 
    http_server.on("/pwr", HTTP_GET, handlePwrReq );
    http_server.on("/pwr", HTTP_POST, handlePwrReq );
    http_server.on("/energy", HTTP_GET, handleEnergyReq );

    // calibrate();

    _cumulatedEnergy = 0;

}

unsigned timeStr2Value(  const char* i_ts)
{
  /* timestr    hh:mm  h:mm  hh:mm:ss
   * split
   */
  unsigned timeval = 0;
  unsigned n=0;
  char* p = ( char*)i_ts;
  char* str;
  while ((str = strtok_r(p, ":", &p)))
  {
    int val = atoi(str);
    switch(n)
    {
      case 0: timeval = val *3600;  break;
      case 1: timeval += val *60;   break;
      case 2: timeval += val;       break;
      default:
      Serial.printf(" Holy shit something went bonkers\n");
    }
    ++n;
  }
  
  return timeval;  
}

void handleEnergyReq()
{
  unsigned requestedEnergy = 0 KWh;
  unsigned optionalEnergy  = 0 KWh;
  unsigned long _now = getTime();
  unsigned dayoffset = _now - (_now%(1 DAY));
    
  for( int n = 0; n < http_server.args(); ++n)
  {
    String p1Name = http_server.argName(n);
    String p1Val = http_server.arg(n);
    double value= atof( p1Val.c_str() );
    Serial.printf("p%dName: %s  val: %s\n",n, p1Name.c_str(), p1Val.c_str() );
   
    if (p1Name == String("requested"))           { requestedEnergy = value;
    } else if (p1Name == String("optional"))     { optionalEnergy  = value;
    } else if (p1Name == String("start"))        { earliestStart   = value + _now;
    } else if (p1Name == String("end"))          { latestStop      = value + _now;
    } else if (p1Name == String("startTime"))    { earliestStart   = timeStr2Value(p1Val.c_str()) + dayoffset;
    } else if (p1Name == String("endTime"))      { latestStop      = timeStr2Value(p1Val.c_str()) + dayoffset;
    } else {
    }
  }
  if ( earliestStart < _now ) {
    earliestStart += (1 DAY);
    latestStop += (1 DAY);
  }
  Serial.printf("POW now: %s EST: %s  LET: %s\n", String(getTimeString(_now)).c_str(), String(getTimeString(earliestStart)).c_str(), String(getTimeString(latestStop)).c_str());
  PlanningData* plan = g_semp->requestEnergy(_now, requestedEnergy,  optionalEnergy,  earliestStart, latestStop );
  Serial.printf("POW requested Energy on plan %p\n", plan);
    
  String resp = String("[POW] Active Power (W)    : ") + String(activePwr) +
        String("\n Voltage (V)         : ") + String(voltage) +
        String("\n[POW] Current (A)         : ") + String(current) +
        String("\n[POW] Energy") +
        String("\n[POW] \ttotal     : ") + String(cumulatedEnergy) +
        String("\n[POW] \trequested : ") + String(requestedEnergy) + 
        String("\n[POW] \toptional  : ") + String(optionalEnergy)  +
        String("\n[POW] \tstart     : ") + String(getTimeString(earliestStart))   +
        String("\n[POW] \tend       : ") + String(getTimeString(latestStop))      +
        +"\n"; 
  Serial.printf("response:\n%s\n",  resp.c_str());
    
  Serial.printf("%s\n", resp.c_str());
  http_server.send(200, "text/plain", resp);
  Serial.printf(" requested Energy end\n" );
    
}



void handlePwrReq()
{
  bool def = true;
   String resp = String("<?xml version=\"1.0\"?>\r\n"
        "<Stat  xmlns=\"http://www.sma.de/communication/schema/SEMP/v1\">\r\n"
        " <led> ") + String(!ledState) + 
        String("</led>\r\n  <switch> ") + String(relayState) + 
        String("</switch>\r\n <activePower> ") + String(activePwr) +
        String("</activePower>\r\n  <averagePower> ") + String(averagePwr) +
        String("</averagePower>\r\n  <voltage> ") + String(voltage) +
        String("</voltage>\r\n  <current> ") + String(current) +
        String("</current>\r\n  <apparentPower> ") + String(apparentPwr) +
        String("</apparentPower>\r\n  <powerFactor> ") + String(pwrFactor) +
        String("</powerFactor>\r\n"
        "</Stat>\r\n"
        );
        
  for( int n = 0; n < http_server.args(); ++n)
  {
    def = false;
    String p1Name = http_server.argName(n);
    String p1Val = http_server.arg(n);
    //float value= atof( p1Val.c_str() );
    Serial.printf("p%dName: %s  val: %s\n",n, p1Name.c_str(), p1Val.c_str() );
    if (p1Name == String("power"))           {      resp = String("power:") + String(activePwr); //constantly 15W Power
    } else if (p1Name == String("voltage"))  {      resp = String("volt:") + String(voltage); //constantly 230V Power
    } else if (p1Name == String("current"))  {      resp = String("current:") + String(current); //constantly 15W Power
    } else if (p1Name == String("apparent")) {      resp = String("apparent:") + String(apparentPwr); //constantly 15W Power
    } else if (p1Name == String("factor"))   {      resp = String("factor:") + String(pwrFactor); //
    } else {
      http_server.send(404, "text/plain", resp);
      return;
    }
  }
  Serial.printf("PWR req: %s\n",resp.c_str());
  if (def)  http_server.send(200, "text/xml", resp);
  else      http_server.send(200, "text/plain", resp);
}





void loopPOW() {
  
   
    static unsigned long last;
    unsigned long now=millis();
    // This UPDATE_TIME should be at least twice the minimum time for the current or voltage
    // signals to stabilize. Experimentally that's about 1 second.
    long dt = ( now - last);
    if ((now - last) > UPDATE_TIME) {
      last = now;
   
      unsigned requestedEnergy = 0;
      unsigned optionalEnergy  = 0;
      PlanningData* plan = g_semp->getActivePlan();
      if (plan){
        requestedEnergy = plan->m_requestedEnergy;
        optionalEnergy  = plan->m_optionalEnergy;
      }
        
      // Simulation of Power read
        static unsigned sim_rd;
        static const unsigned hlw_sim[] = { 900, 600, 1250, 1750, 1500, 150, 850, 10, 500, 1000, 1250, 120, 750 ,80, 500, 1000, 250
           ,750 , 1000, 150, 850,  500, 1000, 250, 500, 1000, 250, 750, 300, 800, 1400, 1600, 900, 700   };
        static unsigned hlw_vsim[] = {229, 230,231,235,232,228,227 };
           

        apparentPwr = activePwr =  g_semp->pwrState() ? hlw_sim[(sim_rd)%DIM(hlw_sim)] : 0;
        voltage =  hlw_vsim[(sim_rd)%DIM(hlw_vsim)];
        current = (double)activePwr / (double)voltage;
        ++sim_rd;
        Serial.printf("Simulated POW read(%u): %u  avr:%u  u:%u i:%f\n",sim_rd, activePwr, averagePwr, voltage, current);
//Simulation end
      
        // get min and max of last values
        _minPwr = _maxPwr = activePwr;
#if 1
        // manipulate sum directly
        sumPwr -= powers[pwrIdx];
        sumPwr += activePwr;
        // maintain mix/maxIdx to avoid searching the array
        if ( powers[pwrIdx] <= powers[minPwrIdx] ) minPwrIdx = pwrIdx;
        if ( powers[pwrIdx] >= powers[maxPwrIdx] ) maxPwrIdx = pwrIdx;
        powers[pwrIdx] = activePwr;
        averagePwr = sumPwr/DIM(powers);
        // commit the min/max values
        minPwr = powers[minPwrIdx];
        maxPwr = powers[maxPwrIdx] ; 
#else

        unsigned long sum = 0;
        powers[pwrIdx] = activePwr;
        for(unsigned  n=0; n<DIM(powers); ++n)
        {
          if ( powers[n] < _minPwr ) _minPwr = (unsigned) powers[n];
          if ( powers[n] > _maxPwr ) _maxPwr = (unsigned) powers[n];
          sum += powers[n];
          averagePwr = sum/DIM(powers);
        }
        // commit the min/max values
        minPwr = (unsigned) _minPwr;
        maxPwr = (unsigned) _maxPwr;  
#endif
        if ( ++pwrIdx >= DIM(powers) ) {
          pwrIdx = 0;;  
          //Serial.printf("idx: %u/%u --average POWER: %u\n", pwrIdx, DIM_PWRS, averagePwr );
        } else {
          //Serial.printf("summing actualPower: %u\n", activePwr );
        }
        
#ifdef USE_POW_DBG
        Serial.printf("[POW] Active Power (W)    : %u\n", activePwr     );
        Serial.printf("[POW] Voltage (V)         : %u\n", voltage       );
        Serial.printf("[POW] Current (A)         : %f\n", current );
        Serial.printf("[POW] Apparent Power (VA) : %u\n", apparentPwr );
        Serial.printf("[POW] Power Factor (%)    : %f\n", pwrFactor     );  
#endif // USE_POW_DBG
        _cumulatedEnergy += activePwr*dt;  /// Wms  -> 1/(3600*1000) Wh
        while ( _cumulatedEnergy > Wh2Wms(1) ) {
         
          ++cumulatedEnergy;
          _cumulatedEnergy -= Wh2Wms(1);
          if ( requestedEnergy > 0) {
            --requestedEnergy;
              g_semp->updateEnergy( now,  -1,  0 );  
          } 

          if ( optionalEnergy > 0)  {
            --optionalEnergy;
            g_semp->updateEnergy( now, 0,  -1 );
          }
      
        }
        Serial.printf("[POW] _cumulatedEnergy    : %ld\n", _cumulatedEnergy     );  
        Serial.printf("[POW] cumulatedEnergy    : %u\n", cumulatedEnergy     );  
        Serial.printf("[POW] requestedEnergy    : %u\n", requestedEnergy     );  
        Serial.printf("[POW] optionalEnergy     : %u\n", optionalEnergy     );  

        g_semp->setPwr( averagePwr, minPwr, maxPwr);
        
    }
}

void setPwr(bool i_state )
{
  relayState = i_state;
}
