/* Rolladensteuerung
   Komponente zum Steuern eines einzelnen Rollladen
   mit Ein- und Ausgängen, Urlaubsteuerung und übergeordneter Steuerung
   (Hausseite, Sonneneinstrahlung etc.)
   Mario Rosenbohm
   Freeware
*/
#pragma once

#define USE_BINARY_SENSOR
#define USE_SWITCH
#define USE_DATETIME_DATE
#define USE_DATETIME_TIME
#define USE_EVENT
#define USE_UPDATE

#include "esphome/core/application.h"
#include "esphome/core/component_iterator.h"
#include "esphome/core/helpers.h"
#include "esphome/core/component.h"
#include "esphome/core/controller.h"
#include "esphome/core/util.h"
#include "esphome/core/log.h"
#include "esphome/core/scheduler.h"
#include "esphome/core/time.h"

#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/pcf8574/pcf8574.h"
#include "esphome/components/ethernet/ethernet_component.h"
#include "esphome/components/uart/uart_component.h"

#include "rollershutter.h"

namespace esphome {
namespace rollershutter {

/** Die Compenentenklasse des Rolladen */
class RollerShutterComponent : public Component {
 private:
  /// @brief die Id
  std::string myId;
  /// @brief Gruppenname
  std::string name;

  /// @brief Laufzeitvoreinstellungen
  std::vector<RL_Time *> *movingTimes;
  /// @brief Rollladengruppen / Hausseiten
  std::vector<RL_Group *> *groups;
  /// @brief die Rolläden
  std::vector<RollerShutter *> *shutters;
  /// @brief Ich bin für Alle Hoch/Runter/Holliday Buttons der Master/Sender
  bool allBtnIsMaster;
  /// @brief Ich bin für Alle Hoch/Runter/Holliday Buttons der Slave/Empfänger
  bool allBtnIsSlave;
  /// @brief Button-Input-GPIO ALLE Hochfahren
  std::string btnUpId;
  /// @brief Pointer auf den GPIO-Pin Btn-Alle-Up
  binary_sensor::BinarySensor *btnUp;
  /// @brief der Button Up wurde gedrückt
  bool btnUpIsPress;
  /// @brief Button-Input-GPIO ALLE Runterfahren
  std::string btnDownId;
  /// @brief der Button Up wird per Remote ausgelesen
  bool btnUpIsRemote;
  /// @brief Pointer auf den GPIO-Pin Btn-Alle-Down
  binary_sensor::BinarySensor *btnDown;
  /// @brief der Button Down wurde gedrückt
  bool btnDownIsPress;
  /// @brief der Button Down wird per Remote ausgelesen
  bool btnDownIsRemote;
  /// @brief Button-Input-GPIO Urlaubsmodus
  std::string btnHollidayId;
  /// @brief Pointer auf den GPIO-Pin Urlaubsmodus
  binary_sensor::BinarySensor *btnHolliday;
  /// @brief der Button Urlaubsmodus wurde gedrückt/ eingeschaltet
  bool btnHollidayIsOn;
  /// @brief der Button Urlaubsmodus wird per Remote ausgelesen
  bool btnHollidayIsRemote;
  /// @brief Das Ereigniss wurde erzeugt
  bool bthHollidayHasCatched;

  /// @brief true, solang die erste Runde nicht abgeschlossen ist
  bool isStarted;
  /// @brief true, wenn das Setup abgeschlossen ist
  bool hasSetup;

  /// @brief Gibt die Time entsprechend der Id zurück
  /// @param id
  /// @return default nullptr
  RL_Time *getTimeById(std::string id);

  /// @brief Gibt die Group entsprechend der Id zurück
  /// @param id
  /// @return default nullptr
  RL_Group *getGroupById(std::string id);

 public:
  /// @brief Konstruktor
  /// @param id
  /// @param name
  /// @param btnUpId
  /// @param btnDownId
  /// @param btnHollidayId
  RollerShutterComponent(std::string id, std::string name);

  /// @brief Destruktor
  ~RollerShutterComponent();

  /// @brief Buttons für alle setzten
  /// @param btnUpId
  /// @param btnDownId
  /// @param btnHollidayId
  /// @param allInputIsMaster
  /// @param allInputIsSlave
  void SetButtons(std::string btnUpId, std::string btnDownId, std::string btnHollidayId, bool allInputIsMaster,
                  bool allInputIsSlave);

  /// @brief onSetup
  void setup() override;
  /// @brief log Config
  void dump_config() override;

  /// @brief Fügt eine RL_Time der Liste hinzu
  /// @param item
  void AddTime(RL_Time *item);
  /// @brief Fügt eine Gruppe der Liste hinzu
  /// @param item
  void AddGroup(RL_Group *item);

  /// @brief Einfügen eines Rolladen in die Liste
  /// @param id
  /// @param name
  /// @param idGroup
  /// @param idTime
  /// @param pinInUp
  /// @param pinInDown
  /// @param pinOutUp
  /// @param pinOutDown
  void AddShutter(std::string id, std::string name, std::string idGroup, std::string idTime, std::string btnUpId,
                  std::string btDownId, std::string relUpId, std::string relDownId);

  /// @brief Erzeugt die Listen für das Abarbeiten der Buttons
  void PrepareShutters();

  /// @brief Erstes Ausführen
  void InitialRun();

  /// @brief onSetup
  void setup() override;

  /// @brief onLoop
  void loop() override;

  /// @brief Dump-Config
  void dump_config() override;

  /// @brief Gibt das Switch anhand seiner Id zurück
  /// @param hisId
  /// @return
  switch_::Switch *getSwitchById(const std::string hisId);

  /// @brief Gibt den Sensor anhand seiner Id Zurück
  /// @param hisId
  /// @return
  binary_sensor::BinarySensor *getBinarySensorById(const std::string hisId);
};

}  // namespace rollershutter
}  // namespace esphome
