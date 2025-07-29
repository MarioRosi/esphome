/* Rolladensteuerung
   Komponente zum Steuern eines einzelnen Rollladen
   mit Ein- und Ausgängen, Urlaubsteuerung und übergeordneter Steuerung
   (Hausseite, Sonneneinstrahlung etc.)
   Mario Rosenbohm
   Freeware
*/
#pragma once
#ifndef ROLLERSHUTTER
#define ROLLERSHUTTER

#define USE_BINARY_SENSOR
#define USE_SWITCH
// #define USE_DATETIME_DATE
// #define USE_DATETIME_TIME
// #define USE_EVENT
// #define USE_UPDATE


#include "esphome/core/defines.h"
#include "esphome/core/application.h"
#include "esphome/core/component_iterator.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/core/controller.h"
#include "esphome/core/util.h"
#include "esphome/core/log.h"
#include "esphome/core/scheduler.h"
#include "esphome/core/time.h"
#include "esphome/core/string_ref.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
//#include "esphome/components/pcf8574/pcf8574.h"

#include "chrono"
#include <string>

namespace esphome {
namespace rollershutter {

// extern definition
class RollerShutterComponent;

/// @brief Status des Rolladen
enum enRollerShutterState {
  /// @brief darf es nicht geben
  isUnknown = 0,
  /// @brief ich wurde neu gestartet, fahre automatisch runter
  isStartingDown = 1,
  /// @brief ich wurde neu gestartet, fahre automatisch hoch
  isStartingUp = 2,
  /// @brief ich wurde gestartet und bin oben!
  isStarted = 3,
  /// @brief ich bin oben
  isTop = 4,
  /// @brief ich fahre nach unten
  isDoDown = 5,
  /// @brief Runterfahren wurde gestoppt
  isStopDoDown = 6,
  /// @brief ich bin unten
  isDown = 7,
  /// @brief ich fahre hoch
  isDoTop = 8,
  /// @brief Hochfahren wurde gestoppt
  isStopDoTop = 9,
  /// @brief Fahre auf Lücke hoch
  isGoToGapUp = 10,
  /// @brief Hochfahren auf Lück gestoppt
  isStopGapUp = 11,
  /// @brief Fahre auf Lücke runter
  isGoToGapDown = 12,
  /// @brief Runterfahren auf Lücke gestoppt
  isStopGapDown = 13,
  /// @brief ich stehe auf Lücke
  isOnGap = 14,
  /// @brief Ende der auf Lücke, hochfahren
  isGapEndGoUp = 15,
};


/// @brief Struktur für die Speicherung von Fahrzeiten
class RL_Time {
 public:
  /// @brief die Id
  std::string id;
  /// @brief Anzahl Sekunden für Hochfahren
  double secondUp;
  /// @brief Anzahl Sekunden für Runterfahren
  double secondDown;
  /// @brief Anzahl Sekunden für Runterfahren auf Lücke
  double secondGap;
  /// @brief Konstruktor
  /// @param id
  /// @param secondUp
  /// @param secondDown
  /// @param secondGap
  RL_Time(const std::string &id, double secondUp, double secondDown, double secondGap) {
    this->id = id;
    this->secondUp =  secondUp;
    this->secondDown = secondDown;
    this->secondGap =  secondGap;
  }
};

/// @brief Struktur für die Speicherung von Intervallen zum Runter- und Hochfahren bei Sonnenbestrahlung
class RL_SunDowner {
 public:
  /// @brief Sundowner ist offline
  bool offline;
  /// @brief Ab welchem Monat
  int monthFrom;
  /// @brief Bis welchen Monat
  int monthTo;
  /// @brief Runterfahren Stunde
  int gapHour;
  /// @brief Runterfahren Minute
  int gapMinute;
  /// @brief Hochfahren Stunde
  int upHoure;
  /// @brief Hochfahren Minute
  int upMinute;

  /// @brief Konstruktor
  /// @param monthFrom
  /// @param monthTo
  /// @param gapHour
  /// @param gapMinute
  /// @param upHoure
  /// @param upMinute
  RL_SunDowner(int monthFrom, int monthTo, int gapHour, int gapMinute, int upHoure, int upMinute) {
    this->monthFrom = monthFrom;
    this->monthTo = monthTo;
    this->gapHour = gapHour;
    this->gapMinute = gapMinute;
    this->upHoure = upHoure;
    this->upMinute = upMinute;
    this->offline = false;
  }

  /// @brief leer-Konstruktor == offline
  RL_SunDowner() {
    this->monthFrom = -1;
    this->monthTo = -1;
    this->gapHour = -1;
    this->gapMinute = -1;
    this->upHoure = -1;
    this->upMinute = -1;
    this->offline = true;
  }
};

/// @brief  Struktur für die Gruppe
class RL_Group {
 public:
  /// @brief die Id
  std::string id;
  /// @brief Gruppenname
  std::string name;
  /// @brief  Einstellungen für Sommer
  RL_SunDowner *sundownner;
  /// @brief Konstruktor
  /// @param id
  /// @param name
  /// @param sundownner
  RL_Group(const std::string &id, const std::string &name, RL_SunDowner *sundownner) {    
    this->id = id;
    this->name = name;
    this->sundownner = sundownner;    
  }
};

using namespace std::chrono_literals;
/// @brief Meine eigene Timerklasse
class Timer {
  private:
    /// @brief Bis dahin läuft der Timer // unix epoch time (seconds since UTC Midnight January 1, 1970)
    std::int64_t timeStampEnd;
    /// @brief Bei dieser zeit wurde der Timer gestartet
    std::int64_t timeStampStart;
    /// @brief zum zwischenspeichern von Werten
    std::int64_t temp;
    /// @brief Wieviele Sekunden ist der Timer gelaufen?
    double secondsIsRunning;
    /// @brief läuft der Timer?
    bool timerIsRunning;
  public:
    /// @brief Konstructor
    Timer() {secondsIsRunning = -1.0; timerIsRunning = false;}
    /// @brief Startet den Timer
    /// @param runningTimeMs 
    /// @return true, Timer konnte gestartet werden, false = es läuft bereits dieser Timer!
    bool StartTimer(double runningTimeSeconds)
    {      
      if (!timerIsRunning)
      {
        ESP_LOGD("Timer", "StartTimer for %f seconds", runningTimeSeconds);
        timeStampStart = GetCurrentTime();
        timeStampEnd = timeStampStart + GetMilliseconds(runningTimeSeconds);
        timerIsRunning = true;
        ESP_LOGD("Timer", "EndTimer is %lld miliseconds", timeStampEnd);
        return true;
      }
      else
      {
        ESP_LOGD("Timer", "Timer is running!");
      }
      return false;
    }

    /// @brief Gibt die aktuelle Zeit als milliSekunden zurück
    /// @return 
    std::int64_t GetCurrentTime()
    {     
      timeval tv;
      gettimeofday(&tv, NULL);
      return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
      //return  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    }

    /// @brief Gibt die Sekunden als millisekunden zurück
    /// @param seconds 
    /// @return 
    std::int64_t GetMilliseconds(double seconds)
    {   
      return ((std::int64_t) (seconds  * 1000.0));
      //return std::chrono::duration_cast<std::chrono::milliseconds>((seconds * 1000.0) * 1ms).count();
    }

    /// @brief wandelt die chrono::milliseconds in double seconds um
    /// @param milliseconds 
    /// @return 
    double GetSeconds(std::int64_t milliseconds)
    {
      return ((double)milliseconds) / 1000.0;
    }

    /// @brief Läuft überhaupt ein Timer?
    /// @return 
    bool IsTimerRunning() {return timerIsRunning;}
    /// @brief Testet, ob der Timer abgelaufen ist
    /// @return >=1 = Timer ist nich nicht zu ende, er läuft noch. 0 == Timer zu ende. -1 == Timer schläft
    int CheckTimer()
    {
      if (timerIsRunning)
      {
         temp = GetCurrentTime();        
        if (temp >= timeStampEnd)
        {          
          StopTimer();
          return 0;
        }
        else 
          secondsIsRunning = GetSeconds(temp - timeStampStart);
        return 1;
      }
      return -1;
    }

    double GetSecondsIsRunning() {return secondsIsRunning;}

    /// @brief Stopt den Timer und gibt die LaufSekunden zurück
    /// @return 0 == Timer war bereits gestoppt, >0 == Laufsekunden
    double StopTimer()
    {
      if (timerIsRunning)
      {        
        temp = GetCurrentTime();
        secondsIsRunning = GetSeconds(temp - timeStampStart);
        timerIsRunning = false;     
        ESP_LOGD("Timer","Timer is stoping");  
        return secondsIsRunning;
      }
      return 0;
    }

    /// @brief Timer Schlafen legen
    void SleepTimer() { this->secondsIsRunning = -1.0;}
};

/** Basisklasse für die Rolladensteuerung, ist ein Rolladen */
class RollerShutter{
 private:
  /// @brief die Id
  std::string myId;
  // /// @brief Gruppenname
  // std::string name;
  /// @brief der letzte State um das Senden zu verringern
  std::string lastState;
  /** Button-Input-GPIO Hochfahren */
  std::string btnUpId;
  /// @brief Der Sensor für Hochfahren
  binary_sensor::BinarySensor* btnUp;
  /// @brief der Button Up wurde gedrückt
  bool btnUpIsPress;
  /// @brief Relais-Output-GPIO Hochfahren
  std::string relUpId;
  /// @brief Pointer auf den GPIO-Pin RelUp
  switch_::Switch *relUp;
  /// @brief Button-Input-GPIO Runterfahren
  std::string btnDownId;
  /// @brief Der Sensor für Runterfahren
  binary_sensor::BinarySensor* btnDown;
  /// @brief der Button Down wurde gedrückt
  bool btnDownIsPress;
  /// @brief Relais-Output-GPIO Runterfahren
  std::string relDownId;
  /// @brief Pointer auf den GPIO-Pin RelDown
  switch_::Switch *relDown;
  /// @brief ist das Setup durchgelaufen?
  bool hasSetup;

  /// @brief Sonnenschutz wird nicht hochgefahren
  bool gapUpIsBlocked;
  /// @brief habe ich heut das Lückenfahren abgefangen?
  bool hasMakeGapCatched;
  /// @brief habe ich heut das Lückenauffahren abgefangen?
  bool hasMakeGapOpenCatched;
  /// @brief Mein derzeitiger Status
  enRollerShutterState myState;
  /// @brief Mein eigener Timer
  Timer *timer;
  /// @brief Die Position des Rolladen 0.0 == oben, 100.0 = vollständig geschlossen
  double closingPosition;
  /// @brief die ID des Textsensor für die Anzeige
  std::string displayId;
  /// @brief Die Icon Anzeige
  text_sensor::TextSensor *display;

  /// @brief ID für die zugehörige Gruppe 
  std::string groupId;
  /// @brief  Die zugehörige Gruppe 
  RL_Group *group;

  /// @brief ID für die Zeiten für Hoch und runter fahren
  std::string timeUpDownId;
  /// @brief die Zeiten für Hoch und runter fahren
  RL_Time *timeUpDown;
  /// @brief TestVariable, dann gibt es sie nur einmal!
  ESPTime timestampCheck ;

  /// @brief die umschließende Komponente
  RollerShutterComponent *myComponent;

  /// @brief Status bauen und senden
  /// @param checkValue im Regelfall closingPosition oder die Vorberechnung
  void sendState(double checkValue);
 public:
  /// @brief Konstruktor
  /// @param id String der id
  /// @param name String, Name
  /// @param groupId ID mit der Gruppe / Sonnenseite
  /// @param timeUpDownId ID mit den Laufzeiten
  /// @param myComponent RollerShutterComponent* die umschließende Componente
  /// @param btnUpId String, id des Schalter up
  /// @param btDownId String, id des Schalter down
  /// @param relUpId String, id des Relay up
  /// @param relDownId String, id des Relay down
  RollerShutter(const std::string &id, const std::string &name, const std::string &groupId, const std::string &timeUpDownId,
                RollerShutterComponent *myComponent, const std::string &btnUpId, 
                const std::string &btDownId, const std::string &relUpId, const std::string &relDownId,
                const std::string &displayId);

  /// @brief Rollladen zurücksetzten == hochfahren
  void ResetRollerShutter();
  
  /// @brief Starte das Hochfahren
  /// @return true erfolgreich gestartet, false == gestoppt oder nix zu tun
  bool StartUp();
  /// @brief starte das Runterfahren
  /// @return true erfolgreich gestartet, false == gestoppt oder nix zu tun
  bool StartDown();
  /// @brief Hoch- oder Runterfahren stoppen und die abgelaufenen Zeit merken
  /// @return
  void Stop();
  /// @brief Testet, ob die Zeit für STOP- Hoch, Runter, Lücke erreicht ist
  void CheckTimerStop();
  /// @brief EventManager für ButtonUp
  /// @param state 
  void OnButtonUpStateChange(bool state);
  /// @brief EventManager für ButtonDown
  /// @param state 
  void OnButtonDownStateChange(bool state);
  /// @brief führt die Befehle entsprechend der Buttons aus
  void MakeButtons();
  /// @brief Setzt den Wert für Button Up ist gedrückt (für Zentraltaster)
  /// @param value
  void SetButtonUpIsPress(bool value);
  /// @brief Setzt den Wert für Button Down ist gedrückt (für Zentraltaster)
  /// @param value
  void SetButtonDownIsPress(bool value);
  /// @brief Der Sonnenschutz wird nicht hochgefahren
  void SetGapUpIsBlocked();
  /// @brief Testet, ob die Zeit für Fahre-Auf-Lücke erreicht ist
  void CheckTimerStartGap();
  /// @brief Fährt auf Lücke, wenn das Rollo unten ist, wird "hochgefahren"
  void StartGap();
  /// @brief Gibt die GruppenId zurück
  /// @return 
  std::string &GetGroupId() {return this->groupId;}
  /// @brief Setter der Group
  /// @param group 
  void SetGroup(RL_Group* group)  { this->group = group;}
  /// @brief Gibt die ZeitenId zurück
  /// @return 
  std::string &GetTimeUpDownId() {return this->timeUpDownId;}
  /// @brief Setter der TimeUpDown
  /// @param group 
  void SetTimeUpDown(RL_Time* timeUpDown)  { this->timeUpDown = timeUpDown;}
  /// @brief Gibt den Status des Rolladen zurück
  /// @return
  enRollerShutterState GetShutterState() { return this->myState; }
  /// @brief Setzt den aktuellen Rolladenstatus
  /// @param newState
  void SetShutterState(enRollerShutterState newState) { this->myState = newState; }
  /// @brief Den Rolladen einrichten
  void MySetup();
  /// @brief Gibt das Switch anhand seiner Id zurück
  /// @param hisId
  /// @return
  switch_::Switch *getSwitchById(const std::string &hisId);
  /// @brief Gibt den Sensor anhand seiner Id Zurück
  /// @param hisId
  /// @return
  binary_sensor::BinarySensor *getBinarySensorById(const std::string &hisId);

  /// @brief Gibt den textsensor anhand seiner ID zurück
  /// @param hisId 
  /// @return 
  text_sensor::TextSensor *getTextSensorById(const std::string &hisId);
};

}  // namespace rollershutter
}  // namespace esphome

#endif // ROLLERSHUTTER