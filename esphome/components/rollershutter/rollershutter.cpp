/* Rolladensteuerung
   Komponente zum Steuern eines einzelnen Rollladen
   mit Ein- und Ausgängen, Urlaubsteuerung und übergeordneter Steuerung
   (Hausseite, Sonneneinstrahlung etc.)
   Mario Rosenbohm
   Freeware
*/

#include "rollershutter.h"
#include "rollershuttercomponent.h"

#include <string>

namespace esphome {
namespace rollershutter {

static const char *TAG = "RollerShutter";

/// @brief Konstruktor
/// @param id String der id
/// @param name String, Name
/// @param groupId ID der mit der Gruppe / Sonnenseite
/// @param timeUpDownId ID mit den Laufzeiten
/// @param myComponent RollerShutterComponent* die umschließende Componente
/// @param btnUpId String, id des Schalter up
/// @param btnDownId String, id des Schalter down
/// @param relUpId String, id des Relay up
/// @param relDownId String, id des Relay down
RollerShutter::RollerShutter(const std::string &id, const std::string &name, const std::string &groupId, const std::string &timeUpDownId,
                             RollerShutterComponent *myComponent, const std::string &btnUpId, const std::string &btnDownId,
                             const std::string &relUpId, const std::string &relDownId) {
  this->myId = id;
  this->name = name;
  this->groupId = groupId;
  this->timeUpDownId = timeUpDownId;
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
        {
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
        }
        break;
      case enRollerShutterState::isDoDown:
        {
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
        }
        break;
      case enRollerShutterState::isStarted:
        {
          relUp->turn_off();
          relDown->turn_off();
          this->closingPosition = 0.0;
          this->myState = enRollerShutterState::isTop;
        }
        break;
      case enRollerShutterState::isGoToGapUp:
        {
          time_t timestampStop = std::time(nullptr);
          relUp->turn_off();
          relDown->turn_off();
          double timeStartToStop = difftime(timestampStop, timestampStart) * 1000.0;
          this->closingPosition -= timeStartToStop / ((double) this->timeUpDown->millisecondUp) * 100.0;
          if (this->closingPosition <= (this->timeUpDown->millisecondGap / this->timeUpDown->millisecondUp * 100.0))
            this->myState = enRollerShutterState::isOnGap;
          else
            this->myState = enRollerShutterState::isStopGapUp;
        }
        break;
      case enRollerShutterState::isGoToGapDown:
        {
          time_t timestampStop = std::time(nullptr);
          relUp->turn_off();
          relDown->turn_off();
          double timeStartToStop = difftime(timestampStop, timestampStart) * 1000.0;
          this->closingPosition += timeStartToStop / ((double) this->timeUpDown->millisecondDown) * 100.0;
          if (this->closingPosition >= (this->timeUpDown->millisecondGap / this->timeUpDown->millisecondDown * 100.0))
            this->myState = enRollerShutterState::isOnGap;
          else
            this->myState = enRollerShutterState::isStopGapDown;
        }
        break;
    }
  }
}


/// @brief Den Rolladen aktivieren
void RollerShutter::Setup() {
  ESP_LOGD(TAG, "setup id = %s", this->myId.c_str());
  
  if (std::strlen(this->btnUpId.c_str()) > 1) {
    binary_sensor::BinarySensor* btnUp = getBinarySensorById(this->btnUpId);
    btnUp->add_on_state_callback([this](bool state){ this->OnButtonUpStateChange(state); });
    if (std::strlen(this->btnDownId.c_str()) > 1) {
      binary_sensor::BinarySensor *btnDown = getBinarySensorById(this->btnDownId);
      btnDown->add_on_state_callback([this](bool state){ this->OnButtonDownStateChange(state); });
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

/// @brief EventManager für ButtonUp
/// @param state 
void RollerShutter::OnButtonUpStateChange(bool state)
{
  if (state) 
  {
    this->btnUpIsPress = true;
    ESP_LOGI(TAG, "Button Up id= %s is Press", this->btnUpId.c_str());
    MakeButtons();
  }
}
/// @brief EventManager für ButtonDown
/// @param state 
void RollerShutter::OnButtonDownStateChange(bool state)
{
  if (state) 
  {
    this->btnDownIsPress = true;
    ESP_LOGI(TAG, "Button Down id= %s is Press", this->btnDownId.c_str());
    MakeButtons();
  }
}


/*
/// @brief Testet, ob die Buttons für Hoch oder Runter gedrückt sind
void RollerShutter::CheckButtons() {
  if (hasSetup) {
    if (this->btnUp->has_state())
    {      
      this->btnUpIsPress = this->btnUp->state;
      if (this->btnUpIsPress)
        ESP_LOGD(TAG, "BTN UP is Press for %s", this->myId.c_str());
    }
    else
      this->btnUpIsPress = false;
    if (this->btnDown->has_state())
    {      
      this->btnDownIsPress = this->btnDown->state;
      if (this->btnDownIsPress)
        ESP_LOGD(TAG, "BTN DOWN is Press for %s", this->myId.c_str());
    }
    else
      this->btnDownIsPress = false;
  }
}
*/
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
switch_::Switch *RollerShutter::getSwitchById(const std::string &hisId) {  
  switch_::Switch *result = nullptr;
  std::vector<switch_::Switch *> switches = App.get_switches();
  for (int idx = 0; idx < switches.size(); idx++ ) {
    switch_::Switch *switchComponent = switches.at(idx);
    if (switchComponent->get_object_id().compare(hisId) == 0)
    {      
      result = switchComponent;
      idx = switches.size();
    }
  }  
  switches.clear();
  return result;
}

/// @brief Gibt den Sensor anhand seiner Id Zurück
/// @param hisId
/// @return
binary_sensor::BinarySensor *RollerShutter::getBinarySensorById(const std::string &hisId) {
  binary_sensor::BinarySensor * result = nullptr;
  std::vector<binary_sensor::BinarySensor *> sensors = App.get_binary_sensors();
  for (int idx = 0; idx < sensors.size(); idx++) {
    binary_sensor::BinarySensor *binSesorComponent = sensors.at(idx);
    if (binSesorComponent->get_object_id().compare(hisId) == 0)
    {      
      result = binSesorComponent;
      idx = sensors.size();
    }
  }
  sensors.clear();
  return result;
}

}  // namespace rollershutter
}  // namespace esphome
