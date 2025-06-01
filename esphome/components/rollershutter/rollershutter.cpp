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
  this->timer = new Timer();
}

/// @brief Rollladen zurücksetzten == hochfahren
void RollerShutter::ResetRollerShutter() {
  myState == isStarting;
  if (timer->StartTimer(this->timeUpDown->secondUp))
  {
    relDown->turn_off();
    relUp->turn_on();
    hasMakeGapCatched = false;
    hasMakeGapOpenCatched = false;
  }
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
        if(this->timer->StartTimer(this->timeUpDown->secondUp))
        {
          relDown->turn_off();
          relUp->turn_on();
          this->myState = enRollerShutterState::isDoTop;
        }
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
        if(this->timer->StartTimer(this->timeUpDown->secondDown))
        {
          relUp->turn_off();
          relDown->turn_on();
          this->myState = enRollerShutterState::isDoDown;
        }
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
    this->timer->StopTimer();
    double timeStartToStop = this->timer->GetSecondsIsRunning();
    this->timer->SleepTimer();
    ESP_LOGD(TAG, "Stop 1");
    switch (myState) {
      case enRollerShutterState::isDoTop:
        {
          ESP_LOGD(TAG, "Stop isDoTop");
          relUp->turn_off();
          relDown->turn_off();
          this->closingPosition -= timeStartToStop / ((double) this->timeUpDown->secondUp) * 100.0;
          this->myState = enRollerShutterState::isStopDoTop;
          if (this->closingPosition <= 0.0) {
            this->closingPosition = 0.0;
            this->myState = enRollerShutterState::isTop;
          }
        }
        break;
      case enRollerShutterState::isDoDown:
        {
          ESP_LOGD(TAG, "Stop isDoDown");
          relUp->turn_off();
          relDown->turn_off();          
          this->closingPosition += timeStartToStop / ((double) this->timeUpDown->secondDown) * 100.0;
          this->myState = enRollerShutterState::isStopDoDown;
          if (this->closingPosition >= 100.0) {
            this->closingPosition = 100.0;
            this->myState = enRollerShutterState::isDown;
          }
        }
        break;
      case enRollerShutterState::isStarted:
        {
          ESP_LOGD(TAG, "Stop isStarted");
          relUp->turn_off();
          relDown->turn_off();
          this->closingPosition = 0.0;
          this->myState = enRollerShutterState::isTop;
        }
        break;
      case enRollerShutterState::isGoToGapUp:
        {
          ESP_LOGD(TAG, "Stop isGoToGapUp");
          relUp->turn_off();
          relDown->turn_off();          
          this->closingPosition -= timeStartToStop / ((double) this->timeUpDown->secondUp) * 100.0;
          if (this->closingPosition <= (this->timeUpDown->secondGap / this->timeUpDown->secondUp * 100.0))
            this->myState = enRollerShutterState::isOnGap;
          else
            this->myState = enRollerShutterState::isStopGapUp;
        }
        break;
      case enRollerShutterState::isGoToGapDown:
        {
          ESP_LOGD(TAG, "Stop isGoToGapDown");
          relUp->turn_off();
          relDown->turn_off();          
          this->closingPosition += timeStartToStop / ((double) this->timeUpDown->secondDown) * 100.0;
          if (this->closingPosition >= (this->timeUpDown->secondGap / this->timeUpDown->secondDown * 100.0))
            this->myState = enRollerShutterState::isOnGap;
          else
            this->myState = enRollerShutterState::isStopGapDown;
        }
        break;
      default:
        int state = (int)myState;
        ESP_LOGD(TAG, "Stop not taked mystate==%d", state);
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
    int checkTimer = this->timer->CheckTimer();
    double timeStartToCheck = this->timer->GetSecondsIsRunning();
    if (checkTimer > 0)
    {                
      switch (myState) {
        case enRollerShutterState::isDoTop:
          if ((this->closingPosition - (timeStartToCheck / this->timeUpDown->secondUp) * 100.0) <= 0.0)
            Stop();
          break;
        case enRollerShutterState::isDoDown:
          if ((this->closingPosition + (timeStartToCheck / this->timeUpDown->secondDown) * 100.0) >= 100.0)
            Stop();
          break;
        case enRollerShutterState::isGoToGapUp:
          if ((this->closingPosition - (timeStartToCheck / this->timeUpDown->secondUp) * 100.0) <=
              (this->timeUpDown->secondGap / this->timeUpDown->secondUp * 100.0))
            Stop();
          break;
        case enRollerShutterState::isGoToGapDown:
          if ((this->closingPosition + (timeStartToCheck / this->timeUpDown->secondDown) * 100.0) >=
              (this->timeUpDown->secondGap / this->timeUpDown->secondDown * 100.0))
            Stop();
          break;
      }
    }
    else if (checkTimer== 0)
    {
      if (myState == enRollerShutterState::isStarting) myState = enRollerShutterState::isStarted;
      ESP_LOGD("RollerShutter", "Timer is stopped after %f seconds", this->timer->GetSecondsIsRunning());
      Stop();
    }
    else if (checkTimer < 0)
    {
      // nix tun ich schlafe
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

/// @brief Setzt den Wert für Button Up ist gedrückt (für Zentraltaster)
/// @param value
void RollerShutter::SetButtonUpIsPress(bool value) 
{
   this->btnUpIsPress = value; 
   this->MakeButtons();
}
/// @brief Setzt den Wert für Button Down ist gedrückt (für Zentraltaster)
/// @param value
void  RollerShutter::SetButtonDownIsPress(bool value) 
{
   this->btnDownIsPress = value; 
   this->MakeButtons();
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
    if (myState == enRollerShutterState::isDoDown || myState == enRollerShutterState::isDoTop)
        Stop();
    if (myState != enRollerShutterState::isGoToGapDown && myState != enRollerShutterState::isGoToGapUp &&
        myState != enRollerShutterState::isStopGapDown && myState != enRollerShutterState::isStopGapUp &&
        myState != enRollerShutterState::isOnGap) {
      if (this->closingPosition < (this->timeUpDown->secondGap / this->timeUpDown->secondDown * 100.0)) {
        this->timer->StartTimer(this->timeUpDown->secondGap);
        relUp->turn_off();
        relDown->turn_on();
        this->myState = enRollerShutterState::isGoToGapDown;
      } else {
        this->timer->StartTimer(this->timeUpDown->secondGap);
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
