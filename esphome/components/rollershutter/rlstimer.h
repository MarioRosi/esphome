/* Rolladensteuerung-Timer
   Timerklasse für die Rolladensteuerung
   Mario Rosenbohm
   Freeware
*/
#pragma once
#ifndef RLSTIMER
#define RLSTIMER

#include <inttypes.h>
#include <chrono>
#include "esphome/core/defines.h"
#include "esphome/core/application.h"
#include "esphome/core/component_iterator.h"
#include "esphome/core/helpers.h"
#include "esphome/core/util.h"
#include "esphome/core/log.h"
#include "esphome/core/scheduler.h"
#include "esphome/core/time.h"

using namespace std::chrono_literals;

namespace esphome {
namespace rollershutter {


//external definitions
class RollerShutter;

/// @brief Meine eigene Timerklasse
class RLSTimer {
  private:
    /// @brief Die Stopfunktion des Rolladen, wird bei erreichen des Timers aufgerufen
    RollerShutter* owner;
    /// @brief Bis dahin läuft der Timer // unix epoch time (seconds since UTC Midnight January 1, 1970)
    std::uint64_t timeStampEnd;
    /// @brief Bei dieser zeit wurde der Timer gestartet
    std::uint64_t timeStampStart;
    /// @brief Wieviele Sekunden ist der Timer gelaufen?
    double secondsIsRunning;
    /// @brief läuft der Timer?
    bool timerIsRunning;
    /// @brief Gibt die aktuelle Zeit als milliSekunden zurück
    /// @return 
    std::uint64_t GetCurrentTime()
    {
      return  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    /// @brief Gibt die Sekunden als millisekunden zurück
    /// @param seconds 
    /// @return 
    std::uint64_t GetMilliseconds(double seconds)
    {   
      return ((std::uint64_t) (seconds  * 1000.0));
    }

    /// @brief wandelt die chrono::milliseconds in double seconds um
    /// @param milliseconds 
    /// @return 
    double GetSeconds(std::uint64_t milliseconds)
    {
      return ((double)milliseconds) / 1000.0;
    }
  public:
    /// @brief Konstructor
    RLSTimer(RollerShutter* owner);
    /// @brief Startet den Timer
    /// @param runningTimeMs 
    /// @return true, Timer konnte gestartet werden, false = es läuft bereits dieser Timer!
    bool StartTimer(double runningTimeSeconds);

    /// @brief Läuft überhaupt ein Timer?
    /// @return 
    bool IsTimerRunning() {return timerIsRunning;}
    /// @brief Testet, ob der Timer abgelaufen ist
    /// @return >=1 = Timer ist nich nicht zu ende, er läuft noch. 0 == Timer zu ende. -1 == Timer schläft
    int CheckTimer();

    double GetSecondsIsRunning() {return secondsIsRunning;}

    /// @brief Stopt den Timer und gibt die LaufSekunden zurück
    /// @return 0 == Timer war bereits gestoppt, >0 == Laufsekunden
    double StopTimer();

    /// @brief Timer Schlafen legen
    void SleepTimer() { this->secondsIsRunning = -1.0;}
};

}  // namespace rollershutter
}  // namespace esphome

#endif // RLSTIMER