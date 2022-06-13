#include "time.h"
#include "sntp.h"
#include "Timezone.h"

#ifndef _TIMECALC_H_
#define _TIMECALC_H_

const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;
int timeFinishSetup = 0;

const char* time_zone = "IST-2IDT,M3.4.4/26,M10.5.0";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)
char time_now[21];
const char epoch[21] = "1970-01-01T00:00:00Z";

/*
 * Get in seconds how much time has past at time_rn since epoch
 * time_rn - Time as char* that you wish to calculate its seconds since epoch
 */
double secondsSinceEpoch(String time_rn) {
  struct tm timeinfo, time_epoch;
  strptime(time_rn.c_str(), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  strptime(epoch, "%Y-%m-%dT%H:%M:%SZ", &time_epoch);
  return difftime(mktime(&timeinfo), mktime(&time_epoch));
}

/*
 * Calculates when time would be finished
 * time_rn - Time of start as char*
 * secondsAdd - how long is usage in seconds
 * return as string what time will be at time_rn + secondsAdd
 */
String finishTime(String time_rn, int secondsAdd)
{
  struct tm timeinfo;
  char time_now[21];
  time_t tt = time_t(secondsSinceEpoch(time_rn) + secondsAdd);
  timeinfo = *gmtime(&tt);
  strftime(time_now, 21, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(time_now);
}

/*
 * return as string what the time is right now
 */
String printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    return ("No time available (yet)");
  }
  strftime(time_now, 21, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
  return String(time_now);
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
  Serial.println("Got time adjustment from NTP!");
  timeFinishSetup = 1;
}

/*
 * Get in seconds how much time is left until time_end
 */
int calcSecsLeft(String time_end)
{
  if(time_end.compareTo("") == 0)
    return -1;
  return (secondsSinceEpoch(time_end) - secondsSinceEpoch(printLocalTime()));
}

/*
 * Set up, should be called in our station's setup
 */
bool setupTime()
{
  Serial.println("Setting up Time");
  sntp_set_time_sync_notification_cb( timeavailable );
  sntp_servermode_dhcp(1);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  return true;
}

#endif
