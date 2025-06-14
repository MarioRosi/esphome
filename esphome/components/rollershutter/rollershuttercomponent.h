/* Rolladensteuerung
   Komponente zum Steuern eines einzelnen Rollladen
   mit Ein- und Ausgängen, Urlaubsteuerung und übergeordneter Steuerung
   (Hausseite, Sonneneinstrahlung etc.)
   Mario Rosenbohm
   Freeware
*/
#pragma once
#ifndef ROLLERSHUTTERCOMPONENT
#define ROLLERSHUTTERCOMPONENT

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

#include "esphome/components/pcf8574/pcf8574.h"
#include "esphome/components/ethernet/ethernet_component.h"
#include "esphome/components/uart/uart_component.h"

#include <string>
#include <vector>

namespace esphome {
namespace rollershutter {

// extern definition
class RL_Time;
class RL_Group;
class RollerShutter;


/** Die Compenentenklasse des Rolladen */
class RollerShutterComponent : public Component {
 private:
  /// @brief die Id
  std::string myId;
  /// @brief Gruppenname
  std::string myName;

  /// @brief Laufzeitvoreinstellungen
  std::vector<RL_Time *> *movingTimes;
  /// @brief Rollladengruppen / Hausseiten
  std::vector<RL_Group *> *groups;
  /// @brief die Rolläden
  std::vector<RollerShutter *> *shutters;
  /// @brief der Button Up wurde gedrückt
  bool btnUpIsPress;
  /// @brief der Button Down wurde gedrückt
  bool btnDownIsPress;
  /// @brief der Button Urlaubsmodus wurde gedrückt/ eingeschaltet
  bool btnHollidayIsOn;
  /// @brief Das Ereigniss wurde erzeugt
  bool bthHollidayHasCatched;
  /// @brief Loop Variable
  ESPTime timestampCheck;
 public:
  /// @brief Konstruktor
  RollerShutterComponent();

  /// @brief Destruktor
  ~RollerShutterComponent();

  /// @brief name und Id setzten  
  void SetIdAndName(const std::string &myId, const std::string &myName);

  /// @brief Fügt eine RL_Time der Liste hinzu
  /// @param id 
  /// @param secondUp 
  /// @param secondDown 
  /// @param secondGap 
  void AddTime(const std::string &id, int secondUp, int secondDown, int secondGap);

  // @brief Fügt eine Gruppe der Liste hinzu

  /// @brief Fügt eine Gruppe mit Sundowner der Liste hinzu
  /// @param id 
  /// @param name 
  /// @param monthFrom 
  /// @param monthTo 
  /// @param gapHour 
  /// @param gapMinute 
  /// @param upHoure 
  /// @param upMinute 
  void AddGroup(const std::string &id, const std::string &name, 
    int monthFrom, int monthTo, int gapHour, int gapMinute, int upHoure, int upMinute);

  /// @brief Fügt eine Gruppe ohne / mit deaktiviertem Sundowner der Liste hinzu
  /// @param id 
  /// @param name 
  void AddGroup(const std::string &id, const std::string &name);

  /// @brief Gibt die Time entsprechend der Id zurück
  /// @param id
  /// @return default nullptr
  RL_Time *GetTimeById(const std::string &id);

  /// @brief Gibt die Group entsprechend der Id zurück
  /// @param id
  /// @return default nullptr
  RL_Group *GetGroupById(const std::string &id);

  /// @brief Einfügen eines Rolladen in die Liste
  /// @param id
  /// @param name
  /// @param idGroup
  /// @param idTime
  /// @param pinInUp
  /// @param pinInDown
  /// @param pinOutUp
  /// @param pinOutDown
  void AddShutter(const std::string &id, const std::string &name, const std::string &idGroup, 
                  const std::string &idTime, const std::string &btnUpId, const std::string &btDownId, 
                  const std::string &relUpId, const std::string &relDownId, const std::string &displayId);

  /// @brief EventManager für ButtonUp
  /// @param state 
  void OnButtonUpStateChange(bool state);
  /// @brief EventManager für ButtonDown
  /// @param state 
  void OnButtonDownStateChange(bool state);

  /// @brief ButtonAllUp ist gedrückt
  void PressButtonAllUp();

  /// @brief ButtonAllDown ist gedrückt
  void PressButtonAllDown();

  /// @brief Ferien anschalten
  void SetIsOnHollidayOn();
  /// @brief Ferien ausschalten
  void SetIsOnHollidayOff();

  /// @brief Erstes Ausführen
  void InitialRun();

  /// @brief onLoop
  void loop() override;

  /// @brief Dump-Config
  void dump_config() override;

};

}  // namespace rollershutter
}  // namespace esphome

#endif // ROLLERSHUTTERCOMPONENT
