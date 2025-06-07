/* Rolladensteuerung
   Komponente zum Steuern eines einzelnen Rollladen
   mit Ein- und Ausgängen, Urlaubsteuerung und übergeordneter Steuerung
   (Hausseite, Sonneneinstrahlung etc.)
   Mario Rosenbohm
   Freeware
*/

#include "rollershuttercomponent.h"
#include "rollershutter.h"
#include <string>
#include <vector>

/* Wichtig, bei Verweung von Lambdafunktionen, sonst tu er nix !!
::update()
{
  if (this->writer_.has_value())
    (this->writer_)(*this);
}
*/

namespace esphome {
namespace rollershutter {

static const char *TAG = "RollerShutterComponent.cpp";

/// @brief Konstruktor
RollerShutterComponent::RollerShutterComponent() {
  this->movingTimes = new std::vector<RL_Time *>();
  this->groups = new std::vector<RL_Group *>();
  this->shutters = new std::vector<RollerShutter *>();
  this->isStarted = true;
  this->hasSetup = false;
  this->btnUpIsRemote = false;
  this->btnDownIsRemote = false;
  this->btnHollidayIsRemote = false;  
}

/// @brief name und Id setzten  
void RollerShutterComponent::SetIdAndName(const std::string &myId, const std::string &myName)
{
  this->myId = myId;
  this->myName = myName;
}


/// @brief Buttons für alle setzten
/// @param btnUpId
/// @param btnDownId
/// @param btnHollidayId
/// @param allInputIsMaster
/// @param allInputIsSlave
void RollerShutterComponent::SetButtons(const std::string &btnUpId, const std::string &btnDownId, const std::string &btnHollidayId,
                                        bool allInputIsMaster, bool allInputIsSlave) {
  this->btnUpId = btnUpId;
  this->btnDownId = btnDownId;
  this->btnHollidayId = btnHollidayId;
  this->allBtnIsMaster = allInputIsMaster;
  this->allBtnIsSlave = allInputIsSlave;
}

RollerShutterComponent::~RollerShutterComponent() {
  this->movingTimes->clear();
  this->groups->clear();
  this->shutters->clear();
}

/// @brief Fügt eine RL_Time der Liste hinzu
/// @param id 
/// @param secondUp 
/// @param secondDown 
/// @param secondGap 
void RollerShutterComponent::AddTime(const std::string &id, int secondUp, int secondDown, int secondGap)
{
  RL_Time *item = new RL_Time(id, secondUp, secondDown, secondGap);
  this->movingTimes->push_back(item); 
}

/// @brief Fügt eine Gruppe mit Sundowner der Liste hinzu1
/// @param id 
/// @param name 
/// @param monthFrom 
/// @param monthTo 
/// @param gapHour 
/// @param gapMinute 
/// @param upHoure 
/// @param upMinute 
void RollerShutterComponent::AddGroup(const std::string &id, const std::string &name, 
  int monthFrom, int monthTo, int gapHour, int gapMinute, int upHoure, int upMinute)
{
  RL_SunDowner *sd = new RL_SunDowner(monthFrom, monthTo, gapHour, gapMinute, upHoure, upMinute);
  RL_Group *item = new RL_Group(id, name, sd);
  this->groups->push_back(item); 
}

/// @brief Fügt eine Gruppe ohne / mit deaktiviertem Sundowner der Liste hinzu
/// @param id 
/// @param name 
void RollerShutterComponent::AddGroup(const std::string &id, const std::string &name)
{
  RL_SunDowner *sd = new RL_SunDowner();
  RL_Group *item = new RL_Group(id, name, sd);
  this->groups->push_back(item); 
}

/// @brief Einfügen eines Rolladen in die Liste
/// @param id
/// @param name
/// @param idGroup
/// @param idTime
/// @param pinInUp
/// @param pinInDown
/// @param pinOutUp
/// @param pinOutDown
void RollerShutterComponent::AddShutter(const std::string &id, const std::string &name, const std::string &groupId, const std::string &timeId,
                                        const std::string &btnUpId, const std::string &btnDownId, const std::string &relUpId,
                                        const std::string &relDownId, const std::string &displayId) {
  RollerShutter *shutter = new RollerShutter(id, name, groupId, timeId, this, btnUpId, 
                                            btnDownId, relUpId, relDownId, displayId);
  shutters->push_back(shutter);
}

/// @brief Gibt die Time entsprechend der Id zurück
/// @param id
/// @return default nullptr
RL_Time *RollerShutterComponent::GetTimeById(const std::string &id) {
  RL_Time *result = nullptr;
  if (!this->movingTimes->empty()) {
    for (int idx=0; idx < this->movingTimes->size(); idx++) {
      RL_Time *time = this->movingTimes->at(idx);
      if (time->id.compare(id) == 0) {
        result = time;
        idx = this->movingTimes->size();
      }
    }
  }
  return result;
}

/// @brief Gibt die Group entsprechend der Id zurück
/// @param id
/// @return default nullptr
RL_Group *RollerShutterComponent::GetGroupById(const std::string &id) {
  RL_Group *result = nullptr;
  if (!this->groups->empty()) {
    for (int idx=0; idx < this->groups->size(); idx++) {
      RL_Group *group = this->groups->at(idx);
      if (group->id.compare(id) == 0) {
        result = group;
        idx = this->groups->size();
      }
    }
  }
  return result;
}


/// @brief erstes Ausführen
void RollerShutterComponent::InitialRun() {
  ESP_LOGI(TAG, "Initial Run begin"); 
  for (int idx = 0; idx < this->shutters->size(); idx++)
  {
    RollerShutter *shutter = this->shutters->at(idx);    
    RL_Time *timeUD = GetTimeById(shutter->GetTimeUpDownId());
    shutter->SetTimeUpDown(timeUD);
    RL_Group *group = GetGroupById(shutter->GetGroupId());
    shutter->SetGroup(group);
    shutter->MySetup();    
  }
  ESP_LOGI(TAG, "Initial Run End");
}

/// @brief onSetup
void RollerShutterComponent::MySetup() {
  if (this->allBtnIsMaster) {
    ESP_LOGI(TAG, "MySetup 1"); 
    if (std::strlen(this->btnUpId.c_str()) > 1) {
      if (this->btnDownId.compare("remote") != 0) {
    ESP_LOGI(TAG, "MySetup 2a"); 
        this->btnUp = getBinarySensorById(this->btnUpId);
        this->btnUp->add_on_state_callback([this](bool state){ this->OnButtonUpStateChange(state); });
        this->btnUpIsRemote = false;
      } else {
    ESP_LOGI(TAG, "MySetup 2b"); 
        this->btnUp = nullptr;
        this->btnUpIsRemote = false;
      }
      if (std::strlen(this->btnDownId.c_str()) > 1) {
        if (this->btnDownId.compare("remote") != 0) {
    ESP_LOGI(TAG, "MySetup 3a"); 
          this->btnDown = getBinarySensorById(this->btnDownId);
          this->btnDown->add_on_state_callback([this](bool state){ this->OnButtonDownStateChange(state); });
          this->btnDownIsRemote = false;
        } else {
    ESP_LOGI(TAG, "MySetup 3b"); 
          this->btnDown = nullptr;
          this->btnDownIsRemote = false;
        }
        if (std::strlen(this->btnHollidayId.c_str()) > 1) {
          if (this->btnDownId.compare("remote") != 0) {
    ESP_LOGI(TAG, "MySetup 4a"); 
            this->btnHolliday = getBinarySensorById(this->btnHollidayId);
            this->btnHollidayIsRemote = false;
          } else {
    ESP_LOGI(TAG, "MySetup 4b"); 
            this->btnHolliday = nullptr;
            this->btnHollidayIsRemote = false;
          }
        }
        this->hasSetup = true;
      }
    }
  } else if (this->allBtnIsSlave) {
    ESP_LOGI(TAG, "MySetup 5"); 
    if (this->btnDownId.compare("remote") == 0) {
    ESP_LOGI(TAG, "MySetup 5-1"); 
      this->btnUp = nullptr;
      this->btnUpIsRemote = true;
    }
    if (std::strlen(this->btnDownId.c_str()) > 1) {
    ESP_LOGI(TAG, "MySetup 5-2"); 
      if (this->btnDownId.compare("remote") == 0) {
    ESP_LOGI(TAG, "MySetup 5-3"); 
        this->btnDown = nullptr;
        this->btnDownIsRemote = true;
      }
      if (std::strlen(this->btnHollidayId.c_str()) > 1) {
    ESP_LOGI(TAG, "MySetup 5-4"); 
        if (this->btnDownId.compare("remote") == 0) {
    ESP_LOGI(TAG, "MySetup 5-5"); 
          this->btnHolliday = nullptr;
          this->btnHollidayIsRemote = true;
    ESP_LOGI(TAG, "MySetup 5-6"); 
        }
      }
      this->hasSetup = true;
    ESP_LOGI(TAG, "MySetup 5-7"); 

    }
  } else {
    this->hasSetup = true;
  } 
 ESP_LOGI(TAG, "MySetup 6"); 

  if (this->hasSetup)  
  {   
    ESP_LOGW(TAG, "Setup war erfolgreich");   
  }
  else
  {
    ESP_LOGW(TAG, "Setup NICHT erfolgreich");          
  }
}

/// @brief EventManager für ButtonUp
/// @param state 
void RollerShutterComponent::OnButtonUpStateChange(bool state)
{
  if (state) 
  {
    this->btnUpIsPress = true;
    ESP_LOGI(TAG, "Button All-Up id= %s is Press", this->btnUpId.c_str());    
  }
}
/// @brief EventManager für ButtonDown
/// @param state 
void RollerShutterComponent::OnButtonDownStateChange(bool state)
{
  if (state) 
  {
    this->btnDownIsPress = true;
    ESP_LOGI(TAG, "Button All-Down id= %s is Press", this->btnDownId.c_str());    
  }
}


/// @brief onLoop
void RollerShutterComponent::loop() {  
  if (hasSetup) {
    // zu erst die Haupt-Buttons abfragen
    /*
    if (!this->btnUpIsRemote) {
      if (this->btnUp->has_state())
        this->btnUpIsPress = this->btnUp->state;
      else
        this->btnUpIsPress = false;
    }
    if (!this->btnDownIsRemote) {
      if (this->btnDown->has_state())
        this->btnDownIsPress = this->btnDown->state;
      else
        this->btnDownIsPress = false;
    }
    if (!this->btnHollidayIsRemote) {
      if (this->btnHolliday->has_state())
        this->btnHollidayIsOn = this->btnHolliday->state;
      else
        this->btnHollidayIsOn = false;
    }
    */
    // Wenn Urlaubssteuerung, hoch und runter zufällig zwischen 5-7 und 17-19
    if (this->btnHollidayIsOn) {
      int checkHourUp = 5;
      int checkHourDown = 17;
      int checkMinUpDown = (int) (118.0 * random_float());
      if (checkMinUpDown > 59) {
        checkHourDown++;
        checkHourUp++;
        checkMinUpDown -= 59;
      }
      ESPTime timestampCheck = ESPTime::from_epoch_local(std::time(nullptr));
      if (!this->bthHollidayHasCatched) {
        if ((timestampCheck.hour == checkHourUp) && (timestampCheck.minute == checkMinUpDown)) {
          this->bthHollidayHasCatched = true;
          this->btnUpIsPress = true;
        } else if ((timestampCheck.hour == checkHourDown) && (timestampCheck.minute == checkMinUpDown)) {
          this->bthHollidayHasCatched = true;
          this->btnDownIsPress = true;
        }
      } else {
        if (timestampCheck.hour == checkHourUp + 4) {
          this->bthHollidayHasCatched = false;
        } else if (timestampCheck.hour == checkHourDown + 4) {
          this->bthHollidayHasCatched = false;
        }
      }
    }
    // zu erst die Buttons abfragen
    if (btnUpIsPress || btnDownIsPress)
    {
      // wenn inzwischen beide gedrückt wurden, dann nix machen
      if (!(btnUpIsPress && btnDownIsPress)) 
      {
        for (auto itter = this->shutters->cbegin(), last = this->shutters->cend(); itter != last; itter++) {
          RollerShutter *shutter = *itter;
          if (this->btnUpIsPress)
            shutter->SetButtonUpIsPress(true);
          else if (this->btnDownIsPress)
            shutter->SetButtonDownIsPress(true);
        }
      }
      this->btnUpIsPress = false;
      this->btnDownIsPress = false;
    }
    for (auto itter = this->shutters->cbegin(), last = this->shutters->cend(); itter != last; itter++) {
      RollerShutter *shutter = *itter;
      shutter->CheckTimerStartGap();
      shutter->CheckTimerStop();
    }
  }
}

/// @brief Dump-Config
void RollerShutterComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Rollershutter '%s'", this->myName.c_str());
  ESP_LOGCONFIG(TAG, "  Anzahl der Rolläden = %zu", this->shutters->size());
  if (this->allBtnIsMaster) {
    ESP_LOGCONFIG(TAG, "  Alles Runter, ich bin Master = %s", this->btnDownIsRemote ? "nein" : "ja");
    ESP_LOGCONFIG(TAG, "  Alles Hoch, ich bin Master = %s", this->btnUpIsRemote ? "nein" : "ja");
    ESP_LOGCONFIG(TAG, "  Urlaubsschalter, ich bin Master = %s", this->btnHollidayIsRemote ? "nein" : "ja");
  } else if (this->allBtnIsMaster) {
    ESP_LOGCONFIG(TAG, "  Alles Runter, ich bin Remote = %s", this->btnDownIsRemote ? "ja" : "nein");
    ESP_LOGCONFIG(TAG, "  Alles Hoch, ich bin Remote = %s", this->btnUpIsRemote ? "ja" : "nein");
    ESP_LOGCONFIG(TAG, "  Urlaubsschalter, ich bin Remote = %s", this->btnHollidayIsRemote ? "ja" : "nein");
  } else {
    ESP_LOGCONFIG(TAG, "  Alles Runter, wird nicht genutzt");
    ESP_LOGCONFIG(TAG, "  Alles Hoch, wird nicht genutzt");
    ESP_LOGCONFIG(TAG, "  Urlaubsschalter, wird nicht genutzt");
  }
}

/// @brief Gibt das Switch anhand seiner Id zurück
/// @param hisId
/// @return
switch_::Switch *RollerShutterComponent::getSwitchById(const std::string &hisId) {
  for (auto *switchComponent : App.get_switches()) {
    if (switchComponent->get_object_id().compare(hisId) == 0)
    {
      return switchComponent;
    }
  }
  return nullptr;
}

/// @brief Gibt den Sensor anhand seiner Id Zurück
/// @param hisId
/// @return
binary_sensor::BinarySensor *RollerShutterComponent::getBinarySensorById(const std::string &hisId) {
  for (auto *binSesorComponent : App.get_binary_sensors()) {
    if (binSesorComponent->get_object_id().compare(hisId) == 0)
    {
      return binSesorComponent;
    }
  }
  return nullptr;
}

}  // namespace rollershutter
}  // namespace esphome
