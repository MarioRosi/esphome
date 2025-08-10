/* Rolladensteuerung-Timer
   Timerklasse für die Rolladensteuerung
   Mario Rosenbohm
   Freeware
*/

#include "rlstimer.h"
#include "rollershutter.h"

namespace esphome {
namespace rollershutter {

/// @brief Konstructor
RLSTimer::RLSTimer(RollerShutter* owner)
{
  this->owner = owner;
  secondsIsRunning = -1.0;
  timerIsRunning = false;
}
/// @brief Startet den Timer
/// @param runningTimeMs 
/// @return true, Timer konnte gestartet werden, false = es läuft bereits dieser Timer!
bool RLSTimer::StartTimer(double runningTimeSeconds)
{      
  if (!timerIsRunning)
  {
    ESP_LOGD("Timer", "StartTimer for %f seconds", runningTimeSeconds);
    timeStampStart = GetCurrentTime();
    timeStampEnd = timeStampStart + GetMilliseconds(runningTimeSeconds);
    timerIsRunning = true;
    ESP_LOGD("Timer", "EndTimer is %" PRIu64 " miliseconds", timeStampEnd);
    return true;
  }
  else
  {
    ESP_LOGD("Timer", "Timer is running!");
  }
  return false;
}

/// @brief Testet, ob der Timer abgelaufen ist
/// @return >=1 = Timer ist nich nicht zu ende, er läuft noch. 0 == Timer zu ende. -1 == Timer schläft
int RLSTimer::CheckTimer()
{
  if (timerIsRunning)
  {
    std::uint64_t temp = GetCurrentTime();        
    if (temp >= timeStampEnd)
    {          
      owner->Stop();
      StopTimer();
      return 0;
    }
    else 
      secondsIsRunning = GetSeconds(temp - timeStampStart);
    return 1;
  }
  return -1;
}

/// @brief Stopt den Timer und gibt die LaufSekunden zurück
/// @return 0 == Timer war bereits gestoppt, >0 == Laufsekunden
double RLSTimer::StopTimer()
{
  if (timerIsRunning)
  {        
    std::uint64_t temp = GetCurrentTime();
    secondsIsRunning = GetSeconds(temp - timeStampStart);
    timerIsRunning = false;     
    ESP_LOGD("Timer","Timer is stoping");  
    return secondsIsRunning;
  }
  return 0;
}



}  // namespace rollershutter
}  // namespace esphome