/* Rolladensteuerung
   Komponente zum Steuern eines einzelnen Rollladen
   mit Ein- und Ausgängen, Urlaubsteuerung und übergeordneter Steuerung
   (Hausseite, Sonneneinstrahlung etc.)
   Mario Rosenbohm
   Freeware
*/

#include "rollershutter.h"
#include <string>

namespace esphome {
namespace rollershutter {

/// @brief Konstruktor
/// @param id String der id
/// @param name String, Name
/// @param group RL_Group* mit der Gruppe / Sonnenseite
/// @param timeUpDown RL_Time* mit den Laufzeiten
/// @param myComponent RollerShutterComponent* die umschließende Componente
/// @param btnUpId String, id des Schalter up
/// @param btDownId String, id des Schalter down
/// @param relUpId String, id des Relay up
/// @param relDownId String, id des Relay down
RollerShutter::RollerShutter(std::string id, std::string name, RL_Group *group, RL_Time *timeUpDown,
                             RollerShutterComponent *myComponent, std::string btnUpId, std::string btDownId,
                             std::string relUpId, std::string relDownId) {
  this->myId = id;
  this->name = name;
  this->group = group;
  this->timeUpDown = timeUpDown;
  this->myComponent = myComponent;
  this->btnUpId = btnUpId;
  this->btnDownId = btnDownId;
  this->relUpId = relUpId;
  this->relDownId = relDownId;
  this->hasSetup = false;
  myState = enRollerShutterState::isUnknown;
}

/// @brief Rollladen zurücksetzten == hochfahren
void RollerShutter::ResetRollerShutter() {
  myState == isStarting;
  this->timestampStart = std::time(nullptr);
  relDown->turn_off();
  relUp->turn_on();
  hasMakeGapCatched = false;
  hasMakeGapOpenCatched = false;
}

/// @brief Starte das Hochfahren
/// @return true erfolgreich gestartet, false == gestoppt oder nix zu tun
bool RollerShutter::StartUp() {
  bool result = false;
  if (hasSetup) {
    switch (myState) {
      case enRollerShutterState::isDoTop:
      case enRollerShutterState::isDoDown:
      case enRollerShutterState::isGoToGapDown:
      case enRollerShutterState::isGoToGapUp:
        Stop();
        break;
      case enRollerShutterState::isStopDoDown:
      case enRollerShutterState::isStopDoTop:
      case enRollerShutterState::isDown:
        this->timestampStart = std::time(nullptr);
        relDown->turn_off();
        relUp->turn_on();
        this->myState = enRollerShutterState::isDoTop;
        break;
      case enRollerShutterState::isStarted:
      case enRollerShutterState::isStarting:
      case enRollerShutterState::isTop:
        // nix tun, alles i.O.
        break;
      case enRollerShutterState::isUnknown:
        ResetRollerShutter();
        break;
    }
  }
  return result;
}

/// @brief starte das Runterfahren
/// @return true erfolgreich gestartet, false == gestoppt oder nix zu tun
bool RollerShutter::StartDown() {
  bool result = false;
  if (hasSetup) {
    switch (myState) {
      case enRollerShutterState::isDoTop:
      case enRollerShutterState::isDoDown:
        Stop();
        break;
      case enRollerShutterState::isStarted:
      case enRollerShutterState::isStopDoDown:
      case enRollerShutterState::isStopDoTop:
      case enRollerShutterState::isTop:
        // Relais hoch aus
        // Relais runter an
        // Timer für ausschalten erzeugen
        this->timestampStart = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        relUp->turn_off();
        relDown->turn_on();
        this->myState = enRollerShutterState::isDoDown;
        break;

      case enRollerShutterState::isDown:
      case enRollerShutterState::isStarting:
        // nix tun, alles i.O.
        break;
      case enRollerShutterState::isUnknown:
        ResetRollerShutter();
        break;
    }
  }
  return result;
}

/// @brief Hoch- oder Runterfahren stoppen und die abgelaufenen Zeit merken
/// @return
void RollerShutter::Stop() {
  if (hasSetup) {
    switch (myState) {
      case enRollerShutterState::isDoTop:
        time_t timestampStop = std::time(nullptr);
        relUp->turn_off();
        relDown->turn_off();
        double timeStartToStop = difftime(timestampStop, timestampStart) * 1000.0;
        this->closingPosition -= timeStartToStop / ((double) this->timeUpDown->millisecondUp) * 100.0;
        this->myState = enRollerShutterState::isStopDoTop;
        if (this->closingPosition <= 0.0) {
          this->closingPosition = 0.0;
          this->myState = enRollerShutterState::isTop;
        }
        break;
      case enRollerShutterState::isDoDown:
        time_t timestampStop = std::time(nullptr);
        relUp->turn_off();
        relDown->turn_off();
        double timeStartToStop = difftime(timestampStop, timestampStart) * 1000.0;
        this->closingPosition += timeStartToStop / ((double) this->timeUpDown->millisecondDown) * 100.0;
        this->myState = enRollerShutterState::isStopDoDown;
        if (this->closingPosition >= 100.0) {
          this->closingPosition = 100.0;
          this->myState = enRollerShutterState::isDown;
        }
        break;
      case enRollerShutterState::isStarted:
        relUp->turn_off();
        relDown->turn_off();
        this->closingPosition = 0.0;
        this->myState = enRollerShutterState::isTop;
        break;
      case enRollerShutterState::isGoToGapUp:
        time_t timestampStop = std::time(nullptr);
        relUp->turn_off();
        relDown->turn_off();
        double timeStartToStop = difftime(timestampStop, timestampStart) * 1000.0;
        this->closingPosition -= timeStartToStop / ((double) this->timeUpDown->millisecondUp) * 100.0;
        if (this->closingPosition <= (this->timeUpDown->millisecondGap / this->timeUpDown->millisecondUp * 100.0))
          this->myState = enRollerShutterState::isOnGap;
        else
          this->myState = enRollerShutterState::isStopGapUp;
        break;
      case enRollerShutterState::isGoToGapDown:
        time_t timestampStop = std::time(nullptr);
        relUp->turn_off();
        relDown->turn_off();
        double timeStartToStop = difftime(timestampStop, timestampStart) * 1000.0;
        this->closingPosition += timeStartToStop / ((double) this->timeUpDown->millisecondDown) * 100.0;
        if (this->closingPosition >= (this->timeUpDown->millisecondGap / this->timeUpDown->millisecondDown * 100.0))
          this->myState = enRollerShutterState::isOnGap;
        else
          this->myState = enRollerShutterState::isStopGapDown;
        break;
    }
  }
}

/// @brief Setzt die zugehörige Gruppe
/// @param mygroup
void RollerShutter::SetGroup(RL_Group *mygroup) { this->group = mygroup; }
/// @brief Setzt die zugehörige Zeiteinstellung
/// @param myTime
void RollerShutter::SetTime(RL_Time *myTime) { this->timeUpDown = myTime; }

/// @brief Den Rolladen aktivieren
void RollerShutter::Setup() {
  if (std::strlen(this->btnUpId.c_str()) > 1) {
    this->btnUp = getBinarySensorById(this->btnUpId);
    if (std::strlen(this->btnDownId.c_str()) > 1) {
      this->btnDown = getBinarySensorById(this->btnDownId);
      if (std::strlen(this->relUpId.c_str()) > 1) {
        this->relUp = getSwitchById(this->relUpId);
        if (std::strlen(this->relDownId.c_str()) > 1) {
          this->relDown = getSwitchById(this->relDownId);
          this->hasSetup = true;
          ResetRollerShutter();
        }
      }
    }
  }
}

/// @brief Testet, ob die Zeit für Hoch, Runter, Lücke erreicht ist
void RollerShutter::CheckTimerStop() {
  if (hasSetup) {
    time_t timestampCheck = std::time(nullptr);
    double timeStartToCheck = difftime(timestampCheck, timestampStart) * 1000.0;
    switch (myState) {
      case enRollerShutterState::isDoTop:
        if (timeStartToCheck >= this->timeUpDown->millisecondUp)
          Stop();
        else {
          if ((this->closingPosition - (timeStartToCheck / this->timeUpDown->millisecondUp) * 100.0) <= 0.0)
            Stop();
        }
        break;
      case enRollerShutterState::isDoDown:
        if (timeStartToCheck >= this->timeUpDown->millisecondDown)
          Stop();
        else {
          if ((this->closingPosition + (timeStartToCheck / this->timeUpDown->millisecondDown) * 100.0) >= 100.0)
            Stop();
        }
        break;
      case enRollerShutterState::isStarting:
        if (timeStartToCheck >= this->timeUpDown->millisecondUp) {
          myState = enRollerShutterState::isStarted;
          Stop();
        }
        break;
      case enRollerShutterState::isGoToGapUp:
        if (timeStartToCheck >= this->timeUpDown->millisecondUp - this->timeUpDown->millisecondGap)
          Stop();
        else {
          if ((this->closingPosition - (timeStartToCheck / this->timeUpDown->millisecondUp) * 100.0) <=
              (this->timeUpDown->millisecondGap / this->timeUpDown->millisecondUp * 100.0))
            Stop();
        }
        break;
      case enRollerShutterState::isGoToGapDown:
        if (timeStartToCheck >= this->timeUpDown->millisecondGap)
          Stop();
        else {
          if ((this->closingPosition + (timeStartToCheck / this->timeUpDown->millisecondDown) * 100.0) >=
              (this->timeUpDown->millisecondGap / this->timeUpDown->millisecondDown * 100.0))
            Stop();
        }
        break;
    }
  }
}

/// @brief Testet, ob die Buttons für Hoch oder Runter gedrückt sind
void RollerShutter::CheckButtons() {
  if (hasSetup) {
    if (this->btnUp->has_state())
      this->btnUpIsPress = this->btnUp->state;
    else
      this->btnUpIsPress = false;
    if (this->btnDown->has_state())
      this->btnDownIsPress = this->btnDown->state;
    else
      this->btnDownIsPress = false;
  }
}

/// @brief führt die Befehle aus, entsprechend den gedrückten Buttons, Up hat Vorrang
void RollerShutter::MakeButtons() {
  if (hasSetup) {
    if (btnUpIsPress)
      StartUp();
    else if (btnDownIsPress)
      StartDown();
    this->btnUpIsPress = false;
    this->btnDownIsPress = false;
  }
}

/// @brief Testet, ob die Zeit für Fahre-Auf-Lücke erreicht ist
void RollerShutter::CheckTimerStartGap() {
  if (hasSetup) {
    if ((this->group != nullptr) && (!this->group->sundownner->offline)) {
      if (myState != enRollerShutterState::isGoToGapDown && myState != enRollerShutterState::isGoToGapUp &&
          myState != enRollerShutterState::isStopGapDown && myState != enRollerShutterState::isStopGapUp &&
          myState != enRollerShutterState::isGapEndGoUp) {
        ESPTime timestampCheck = ESPTime::from_epoch_local(std::time(nullptr));
        if ((timestampCheck.month >= this->group->sundownner->monthFrom) &&
            (timestampCheck.month <= this->group->sundownner->monthTo)) {
          if ((timestampCheck.hour == this->group->sundownner->gapHour) &&
              (timestampCheck.minute == this->group->sundownner->gapMinute) && !hasMakeGapCatched) {
            hasMakeGapCatched = true;
            StartGap();
          } else if ((timestampCheck.hour == this->group->sundownner->upHoure) &&
                     (timestampCheck.minute == this->group->sundownner->upMinute) && !hasMakeGapOpenCatched) {
            hasMakeGapOpenCatched = true;
            this->myState == enRollerShutterState::isGapEndGoUp;
            StartUp();
          } else if ((timestampCheck.hour == this->group->sundownner->upHoure + 1) &&
                     (timestampCheck.minute == this->group->sundownner->upMinute) && hasMakeGapOpenCatched &&
                     hasMakeGapCatched) {
            hasMakeGapCatched = false;
            hasMakeGapOpenCatched = false;
          }
        }
      }
    }
  }
}

/// @brief Fährt auf Lücke, wenn das Rollo unten ist, wird "hochgefahren"
void RollerShutter::StartGap() {
  if (hasSetup) {
    switch (myState) {
      case enRollerShutterState::isDoDown:
      case enRollerShutterState::isDoTop:
        Stop();
        break;
    }
    if (myState != enRollerShutterState::isGoToGapDown && myState != enRollerShutterState::isGoToGapUp &&
        myState != enRollerShutterState::isStopGapDown && myState != enRollerShutterState::isStopGapUp &&
        myState != enRollerShutterState::isOnGap) {
      if (this->closingPosition < (this->timeUpDown->millisecondGap / this->timeUpDown->millisecondDown * 100.0)) {
        this->timestampStart = std::time(nullptr);
        relUp->turn_off();
        relDown->turn_on();
        this->myState = enRollerShutterState::isGoToGapDown;
      } else {
        this->timestampStart = std::time(nullptr);
        relDown->turn_off();
        relUp->turn_on();
        this->myState = enRollerShutterState::isGoToGapUp;
      }
    }
  }
}
/// @brief Gibt das Switch anhand seiner Id zurück
/// @param hisId
/// @return
switch_::Switch *RollerShutter::getSwitchById(const std::string hisId) {
  for (auto *switchComponent : App.get_switches()) {
    if (strcmp(switchComponent->get_object_id().c_str(), hisId.c_str()) == 0)
      return switchComponent;
  }
  return nullptr;
}

/// @brief Gibt den Sensor anhand seiner Id Zurück
/// @param hisId
/// @return
binary_sensor::BinarySensor *RollerShutter::getBinarySensorById(const std::string hisId) {
  for (auto *binSesorComponent : App.get_binary_sensors()) {
    if (strcmp(binSesorComponent->get_object_id().c_str(), hisId.c_str()) == 0)
      return binSesorComponent;
  }
  return nullptr;
}

}  // namespace rollershutter
}  // namespace esphome
