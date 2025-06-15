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
                             const std::string &relUpId, const std::string &relDownId, const std::string &displayId) {
  this->myId = id;  
  //this->set_name(name.c_str());
  this->groupId = groupId;
  this->timeUpDownId = timeUpDownId;
  this->myComponent = myComponent;
  this->btnUpId = btnUpId;
  this->btnDownId = btnDownId;
  this->relUpId = relUpId;
  this->relDownId = relDownId;
  this->displayId = displayId;
  this->hasSetup = false;
  myState = enRollerShutterState::isUnknown;
  this->timer = new Timer();
}

/// @brief Rollladen zurücksetzten == hochfahren
void RollerShutter::ResetRollerShutter() {
  myState = enRollerShutterState::isStarting;
  if (timer->StartTimer(this->timeUpDown->secondUp))
  {
    relDown->turn_off();
    relUp->turn_on();
    hasMakeGapCatched = false;
    hasMakeGapOpenCatched = false;
    hasGapUpTimeLog = false;
    hasGapUpGapLog = false;
    hasGapEndTimeLog = false;
    hasGapEndGapLog = false;
    sendState( -1.0 );
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
        ESP_LOGD(TAG, "Make StartUp->Stop");
        Stop();
        break;
      case enRollerShutterState::isStopDoDown:
      case enRollerShutterState::isStopDoTop:
      case enRollerShutterState::isDown:
      case enRollerShutterState::isGapEndGoUp:
        if(this->timer->StartTimer(this->timeUpDown->secondUp))
        {
          ESP_LOGD(TAG, "Make StartUp, Open Relais UP");
          relDown->turn_off();
          relUp->turn_on();
          this->myState = enRollerShutterState::isDoTop;
          sendState(closingPosition);
        }
        else
          ESP_LOGD(TAG,"StartUp - Timer has not started!");
        break;
      case enRollerShutterState::isStarted:
      case enRollerShutterState::isStarting:
      case enRollerShutterState::isTop:
        // nix tun, alles i.O.
        break;
      case enRollerShutterState::isUnknown:
        ESP_LOGD(TAG, "Make StartUp, Reset Rollershutter");
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
        ESP_LOGD(TAG, "Make StartDown-Stop");
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
          ESP_LOGD(TAG, "Make StartDown, Open Relais Down");
          relUp->turn_off();
          relDown->turn_on();
          this->myState = enRollerShutterState::isDoDown;
          sendState(closingPosition);
        }
        else
          ESP_LOGD(TAG,"StartDown - Timer has not started!");
        break;
      case enRollerShutterState::isDown:
      case enRollerShutterState::isStarting:
        // nix tun, alles i.O.
        break;
      case enRollerShutterState::isUnknown:
          ESP_LOGD(TAG, "Make StartDown, Reset Rollershutter");
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
    switch (myState) {
      case enRollerShutterState::isDoTop:
        {
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
          relUp->turn_off();
          relDown->turn_off();
          this->closingPosition = 0.0;
          this->myState = enRollerShutterState::isTop;
        }
        break;
      case enRollerShutterState::isGoToGapUp:
        {
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
        ESP_LOGD(TAG, "Stop not taked mystate==%d", (int)myState);
        break;
    }
    ESP_LOGD(TAG,"Stop with closingPosition=%f", closingPosition);
    sendState(closingPosition);
  }
}


/// @brief Den Rolladen aktivieren
void RollerShutter::MySetup() {
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
          if (std::strlen(this->displayId.c_str()) > 1) {
            this->display = getTextSensorById(this->displayId);
            this->hasSetup = true;                      
            ResetRollerShutter();
          }
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
      double checkvalue = 0.0;
      switch (myState) {
        case enRollerShutterState::isDoTop:
          checkvalue = this->closingPosition - (timeStartToCheck / this->timeUpDown->secondUp) * 100.0;
          if (checkvalue <= 0.0)
            Stop();
          else
            sendState(checkvalue);
          break;
        case enRollerShutterState::isDoDown:
          checkvalue = (this->closingPosition + (timeStartToCheck / this->timeUpDown->secondDown) * 100.0);
          if ( checkvalue >= 100.0)
            Stop();
          else
            sendState(checkvalue);
          break;
        case enRollerShutterState::isGoToGapUp:
          checkvalue = (this->closingPosition - (timeStartToCheck / this->timeUpDown->secondUp) * 100.0);
          if (checkvalue <= (this->timeUpDown->secondGap / this->timeUpDown->secondUp * 100.0))
            Stop();
          else
            sendState(checkvalue);
          break;
        case enRollerShutterState::isGoToGapDown:
          checkvalue = (this->closingPosition + (timeStartToCheck / this->timeUpDown->secondDown) * 100.0);
          if (checkvalue >= (this->timeUpDown->secondGap / this->timeUpDown->secondDown * 100.0))
            Stop();
          else
            sendState(checkvalue);
          break;
      }
    }
    else if (checkTimer == 0)
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

/// @brief Status bauen und senden
void RollerShutter::sendState(double checkValue)
{
  std::string newValue = "window-shutter-";
  if (myState == enRollerShutterState::isUnknown)
    newValue += "error";
  else
  {
    if (checkValue < 10.0)
      newValue += "0";
    else if ((checkValue >= 10.0) && (checkValue < 30.0))
      newValue += "1";
    else if ((checkValue >= 30.0) && (checkValue < 50.0))
      newValue += "2";
    else if ((checkValue >= 50.0) && (checkValue < 60.0))
      newValue += "3";
    else if ((checkValue >= 60.0) && (checkValue < 90.0))
      newValue += "4";
    else if (checkValue >= 90.0)
      newValue += "5";

    if ((myState == enRollerShutterState::isDoDown) ||
        (myState == enRollerShutterState::isGoToGapDown))
        newValue += "-down";
    else if ((myState == enRollerShutterState::isDoTop) ||
        (myState == enRollerShutterState::isGoToGapUp) ||
        (myState == enRollerShutterState::isStarting))
        newValue += "-up";
  }
  //newValue += ".svg";
  if (lastState.compare(newValue) != 0)
  {
    lastState = newValue;
    ESP_LOGD(TAG, "Send state %s", newValue.c_str());
    if (this->display != nullptr)
      this->display->publish_state(newValue);
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
  ESP_LOGD(TAG, "Button up send '%s'", value ? "true" : "false");
  this->btnUpIsPress = value; 
  this->MakeButtons();
}
/// @brief Setzt den Wert für Button Down ist gedrückt (für Zentraltaster)
/// @param value
void  RollerShutter::SetButtonDownIsPress(bool value) 
{
  ESP_LOGD(TAG, "Button down send '%s'", value ? "true" : "false");
  this->btnDownIsPress = value; 
  this->MakeButtons();
}


/// @brief führt die Befehle aus, entsprechend den gedrückten Buttons, Up hat Vorrang
void RollerShutter::MakeButtons() {
  if (hasSetup) {
    if (this->btnUpIsPress)
    {
      ESP_LOGD(TAG, "Make StartUp");
      StartUp();
    }
    else if (this->btnDownIsPress)
    {
      ESP_LOGD(TAG, "Make StartDown");
      StartDown();
    }
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
        this->timestampCheck = ESPTime::from_epoch_local(std::time(nullptr));
        if ((this->timestampCheck.month >= this->group->sundownner->monthFrom) &&
            (this->timestampCheck.month <= this->group->sundownner->monthTo)) {

          if ((this->timestampCheck.hour == this->group->sundownner->gapHour) &&
              (this->timestampCheck.minute == this->group->sundownner->gapMinute)) {
            if (!hasGapUpTimeLog)
            {
              hasGapUpTimeLog = true;
              ESP_LOGD(TAG, "Gap is in Time");
              hasGapResetTimeLog = false;
              hasGapResetLog = false;
            }
            if (!this->hasMakeGapCatched) {
              if (!hasGapUpGapLog)
              {
                hasGapUpGapLog = true;
                ESP_LOGD(TAG, "Gap is in Making");
              }
              this->hasMakeGapCatched = true;
              ESP_LOGD(TAG, "Start make gap!");
              StartGap();
            }
            else{
              if (!hasGapUpGapLog)
              {
                hasGapUpGapLog = true;
                ESP_LOGD(TAG, "Gap is hasMakeGapCatched==true");
              }
            }
          }
          if ((this->timestampCheck.hour == this->group->sundownner->upHoure) &&
              (this->timestampCheck.minute == this->group->sundownner->upMinute)) {
            if (!hasGapEndTimeLog)
            {
              hasGapEndTimeLog = true;
              ESP_LOGD(TAG, "Gap END is in Time");
            }
            if (!this->hasMakeGapOpenCatched) {
              if (!hasGapEndGapLog)
              {
                hasGapEndGapLog = true;
                ESP_LOGD(TAG, "Gap END is on Making");
              }
              this->hasMakeGapOpenCatched = true;
              this->myState == enRollerShutterState::isGapEndGoUp;
              ESP_LOGD(TAG, "Gap-Time is END, open shutter.");
              StartUp();
            }
            else{
              if (!hasGapEndGapLog)
              {
                hasGapEndGapLog = true;
                ESP_LOGD(TAG, "Gap END is hasMakeGapOpenCatched==true");
              }
            }
          }
          if ((this->timestampCheck.hour == (this->group->sundownner->upHoure + 1)) &&
              (this->timestampCheck.minute == this->group->sundownner->upMinute)) {
            if (!hasGapResetTimeLog)
            {
              hasGapResetTimeLog = true;
              ESP_LOGD(TAG, "Gap Reset is in Time");
            }
            if (this->hasMakeGapOpenCatched || this->hasMakeGapCatched) {
              if (!hasGapResetLog)
              {
                hasGapResetLog = true;
                ESP_LOGD(TAG, "Gap Reset is make");
                hasGapUpTimeLog = false;
                hasGapUpGapLog = false;
                hasGapEndTimeLog = false;
                hasGapEndGapLog = false;
              }
              ESP_LOGD(TAG, "Gap-reset catched.");
              this->hasMakeGapCatched = false;
              this->hasMakeGapOpenCatched = false;
            }
            else{
              if (!hasGapResetLog)
              {
                hasGapResetLog = true;
                ESP_LOGD(TAG, "Gap Reset is hasMakeGapOpenCatched==false && hasMakeGapCatched == false");
                hasGapUpTimeLog = false;
                hasGapUpGapLog = false;
                hasGapEndTimeLog = false;
                hasGapEndGapLog = false;
              }
            }
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
        if (this->timer->StartTimer(this->timeUpDown->secondGap))
        {
          ESP_LOGD(TAG,"Start GoToGap -Down");
          relUp->turn_off();
          relDown->turn_on();
          this->myState = enRollerShutterState::isGoToGapDown;
        }
      } else {
        if (this->timer->StartTimer(this->timeUpDown->secondGap))
        {
          ESP_LOGD(TAG,"Start GoToGap -Up");
          relDown->turn_off();
          relUp->turn_on();
          this->myState = enRollerShutterState::isGoToGapUp;
        }
      }
      sendState(closingPosition);
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
  if (result == nullptr)
    ESP_LOGW(TAG, "switch '%s' not found!", hisId.c_str());
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
  if (result == nullptr)
    ESP_LOGW(TAG, "switch '%s' not found!", hisId.c_str());
  return result;
}

  /// @brief Gibt den textsensor anhand seiner ID zurück
  /// @param hisId 
  /// @return 
  text_sensor::TextSensor *RollerShutter::getTextSensorById(const std::string &hisId) {
  text_sensor::TextSensor * result = nullptr;
  std::vector<text_sensor::TextSensor *> sensors = App.get_text_sensors();
  for (int idx = 0; idx < sensors.size(); idx++) {
    text_sensor::TextSensor *textSensorComponent = sensors.at(idx);
    if (textSensorComponent->get_object_id().compare(hisId) == 0)
    {      
      result = textSensorComponent;
      idx = sensors.size();
    }
  }
  sensors.clear();
  if (result == nullptr)
    ESP_LOGW(TAG, "text-sensor '%s' not found!", hisId.c_str());
  return result;
}

}  // namespace rollershutter
}  // namespace esphome
