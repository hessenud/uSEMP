//------------NTP---------------
class CTimer; // forward
class CTimer {
  
  unsigned long m_swTime;     //< switch time in minutes
  bool          m_switchmode; //< true = on false = off
  int*          m_channel;    //< switch number  
  bool          m_hot;        //< time != swTime; hot<-true | when swTime is reached timer is executed and hot <- false
  
  CTimer*       m_next;  
public:
  unsigned long getSwTime() { return m_swTime; }
  bool          hot() { return m_hot; }
  CTimer*       next(){ return m_next; }
  void          trigger();  
  void          append(CTimer* i_tmt);
  void          append_to(CTimer* i_list);
  bool          check(unsigned long _time);
  void rearm() {
    m_hot = true;
  }
  
};

CTimer* g_Timerlist;
void CTimer::append_to(CTimer* i_list)
{
  CTimer* wp= i_list;
  if (wp){
    while(  wp->next()) wp = wp->next();
    wp->m_next = this;
  }
}

void CTimer::append(CTimer* i_tmr)
{
  CTimer* wp= this;
  while(  wp->m_next)  wp = wp->next();
  wp->m_next = i_tmr;
}


void CTimer::trigger()
{
  *m_channel = m_switchmode;
  m_hot = false;
}


bool CTimer::check(unsigned long _time) 
{
  if(m_swTime == _time) 
  {
    if ( m_hot  ) // hot an time reached
    {
      trigger(); 
      return true;
    } 
  } else {
    rearm();  
  }
  return false;
}

/********************************************************************
 * NTP Client
 ********************************************************************/

extern String IpAddress2String(const IPAddress& ipAddress);
unsigned int localPort = 2390;      // local port to listen for UDP packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;
// send an NTP request to the time server at the given address

void setupTimeClk()
{
  Serial.println("Starting NTP UDP port");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
}


unsigned long g_localTime;

/*
 * © Francesco Potortì 2013 - GPLv3 - Revision: 1.13
 *
 * Send an NTP packet and wait for the response, return the Unix time
 *
 * To lower the memory footprint, no buffers are allocated for sending
 * and receiving the NTP packets.  Four bytes of memory are allocated
 * for transmision, the rest is random garbage collected from the data
 * memory segment, and the received packet is read one byte at a time.
 * The Unix time is returned, that is, seconds from 1970-01-01T00:00.
 */
unsigned long ntpUnixTime (UDP &udp)
{
  static int udpInited = udp.begin(123); // open socket on arbitrary port

  //const char timeServer[] = "pool.ntp.org";  // NTP server
  const char timeServer[] = "fritz.box";  // NTP server

  // Only the first four bytes of an outgoing NTP packet need to be set
  // appropriately, the rest can be whatever.
  const long ntpFirstFourBytes = 0xEC0600E3; // NTP request header

  // Fail if WiFiUdp.begin() could not init a socket
  if (! udpInited)
    return 0;

  // Clear received data from possible stray received packets
  udp.flush();

  // Send an NTP request
  if (! (udp.beginPacket(timeServer, 123) // 123 is the NTP port
   && udp.write((byte *)&ntpFirstFourBytes, 48) == 48
   && udp.endPacket()))
    return 0;       // sending request failed

  // Wait for response; check every pollIntv ms up to maxPoll times
  const int pollIntv = 150;   // poll every this many ms
  const byte maxPoll = 15;    // poll up to this many times
  int pktLen;       // received packet length
  for (byte i=0; i<maxPoll; i++) {
    if ((pktLen = udp.parsePacket()) == 48)
      break;
    delay(pollIntv);
  }
  if (pktLen != 48)
    return 0;       // no correct packet received

  // Read and discard the first useless bytes
  // Set useless to 32 for speed; set to 40 for accuracy.
  const byte useless = 40;
  for (byte i = 0; i < useless; ++i)
    udp.read();

  // Read the integer part of sending time
  unsigned long time = udp.read();  // NTP time
  for (byte i = 1; i < 4; i++)
    time = time << 8 | udp.read();

  // Round to the nearest second if we want accuracy
  // The fractionary part is the next byte divided by 256: if it is
  // greater than 500ms we round to the next second; we also account
  // for an assumed network delay of 50ms, and (0.5-0.05)*256=115;
  // additionally, we account for how much we delayed reading the packet
  // since its arrival, which we assume on average to be pollIntv/2.
  time += (udp.read() > 115 - pollIntv/8);

  // Discard the rest of the packet
  udp.flush();

  return time - 2208988800ul;   // convert NTP time to Unix time
}

unsigned long utc2local(unsigned long utc)
{
  unsigned long tz = +100; //100th hours
  
  return utc + (36 * tz);
}



unsigned long getTime()
{
  static unsigned ltime_ms;
  static unsigned long epoch;
  if (!epoch )
  {
    Serial.println(" get NTP time!");
    epoch = ntpUnixTime(udp);
    ltime_ms = millis();
    g_localTime = utc2local( epoch ) + 3600;
  }
  if ( epoch ) {
    if ( 0 == (g_localTime%600)) {
      epoch = 0;
    } else {
      // update time locally
      unsigned now = millis();
      int dt = now - ltime_ms;
      while ( dt >= 1000 ){
        ++g_localTime;
        ltime_ms += 1000;
        dt -= 1000;
        //Serial.printf("%s\n", getTimeString( g_localTime) );
      }
    }
  }
 // Serial.printf("get Time %ld\n",  g_localTime );
  return g_localTime;
}

const char* getTimeString( unsigned long theTime )
{
    static char timestr[9];
     // print the hour, minute and second:
    sprintf( timestr, "%2lu:%02lu:%02lu", ((theTime  % 86400L) / 3600), ((theTime  % 3600) / 60), theTime % 60);
    
    return timestr;
}

const char* getTimeStringS( unsigned long theTime )
{
    static char timestr[9];
     // print the hour and minute:
    sprintf( timestr, "%2lu:%02lu", ((theTime  % 86400L) / 3600), ((theTime  % 3600) / 60) );
    
    return timestr;
}





void loopTimeClk()
{
  static unsigned long _last;
  unsigned long _time = getTime(); 
 // Timer
  if ( _last != _time ) {
    _last = _time;
    //Serial.printf("Timer-Tick: %s checking timers\n",  getTimeString( _time ));
    for( CTimer* wp = g_Timerlist; wp; wp = wp->next() ) {
      wp->check(_time);
    }
  }
}
