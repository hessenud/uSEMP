

// Check values every 2 seconds
#define UPDATE_TIME                     3000


void unblockingDelay(unsigned long mseconds) {
    unsigned long timeout = millis();
    while ((millis() - timeout) < mseconds) delay(1);
}


void setupPOW() {
    g_semp->startService( );
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



void loopPOW() {
  
   
    static unsigned long last;
    unsigned long now=millis();
    // This UPDATE_TIME should be at least twice the minimum time for the current or voltage
    // signals to stabilize. Experimentally that's about 1 second.
    long dt = ( now - last);
    if ((now - last) > UPDATE_TIME) {
      last = now;
     
      g_semp->updateEnergy( now, 0,  0 );
      g_semp->setPwr( (getTime() % 1 DAY) % 360, 1, 20);
    }
}

void setPwr( bool )
{
}
