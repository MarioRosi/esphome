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
  ESP_LOGD(TAG, "Ende aus dem Konstruktor"); // mem=%d", heap_caps_get_free_size((1<<12)));
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
  ESP_LOGD(TAG, "SetButtons");
}

RollerShutterComponent::~RollerShutterComponent() {
  this->movingTimes->clear();
  this->groups->clear();
  this->shutters->clear();
}

/// @brief Fügt eine RL_Time der Liste hinzu
/// @param id 
/// @param millisecondUp 
/// @param millisecondDown 
/// @param millisecondGap 
void RollerShutterComponent::AddTime(const std::string &id, int millisecondUp, int millisecondDown, int millisecondGap)
{
  RL_Time *item = new RL_Time(id, millisecondUp, millisecondDown, millisecondGap);
  this->movingTimes->push_back(item); 
  ESP_LOGD(TAG, "AddTime mit id = %s", id.c_str());
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
  //ESP_LOGD(TAG, "AddGroup (id = %s, name=%s, monthFrom=%d, monthTo=%d, gapH=%d, gapM=%d, upH=%d, upM=%d)", id.c_str(), name.c_str(), monthFrom, monthTo, gapHour, gapMinute, upHoure, upMinute);
  ESP_LOGD(TAG, "AddGroup new RL_SunDowner");
  RL_SunDowner *sd = new RL_SunDowner(monthFrom, monthTo, gapHour, gapMinute, upHoure, upMinute);
  ESP_LOGD(TAG, "AddGroup after new RL_SunDowner");
  ESP_LOGD(TAG, "AddGroup new SD");
  RL_Group *item = new RL_Group(id, name, sd);
  ESP_LOGD(TAG, "AddGroup new GRP "); //%s", item->name.c_str());
  this->groups->push_back(item); 
  ESP_LOGD(TAG, "AddGroup mit SD "); //id = %s", id);
}

/// @brief Fügt eine Gruppe ohne / mit deaktiviertem Sundowner der Liste hinzu
/// @param id 
/// @param name 
void RollerShutterComponent::AddGroup(const std::string &id, const std::string &name)
{
  RL_SunDowner *sd = new RL_SunDowner();
  RL_Group *item = new RL_Group(id, name, sd);
  this->groups->push_back(item); 
  ESP_LOGD(TAG, "AddGroup ohne SD "); //mit id = %s", id.c_str());
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
                                        const std::string &relDownId) {
  RollerShutter *shutter = new RollerShutter(id, name, groupId, timeId, this, btnUpId, btnDownId, relUpId, relDownId);
  shutters->push_back(shutter);
  ESP_LOGD(TAG, "AddShutter mit id"); // = %s", id);
}

/// @brief Gibt die Time entsprechend der Id zurück
/// @param id
/// @return default nullptr
RL_Time *RollerShutterComponent::GetTimeById(const std::string &id) {
  RL_Time *result = nullptr;
  if (!this->movingTimes->empty()) {
    for (auto itter = this->movingTimes->cbegin(), last = this->movingTimes->cend(); itter != last; itter++) {
      RL_Time *time = *itter;
      if (time->id.compare(id) == 0) {
        result = time;
        itter = last;
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
  if (!this->movingTimes->empty()) {
    for (auto itter = this->groups->cbegin(), last = this->groups->cend(); itter != last; itter++) {
      RL_Group *group = *itter;
      if (group->id.compare(id) == 0) {
        result = group;
        itter = last;
      }
    }
  }
  return result;
}

/// @brief Erzeugt die Listen für das Abarbeiten der Buttons
void RollerShutterComponent::PrepareShutters() {
  /*
  if (shutters->size() > 0)
  {
    for (auto itter = this->shutters->cbegin(), last = this->shutters->cend();
              itter != last; itter++)
    {
      RollerShutter* shutter = *itter;
      auto itterUpIdToShutters = btnUpToSutters->find(shutter->GetBtnUpId());
      if (itterUpIdToShutters == btnUpToSutters->end())
      {
        std::vector<RollerShutter*>* newList = new std::vector<RollerShutter*>();
        newList->push_back(shutter);
        this->btnUpToSutters->insert(std::pair<std::string, std::vector<RollerShutter*>*>(shutter->GetBtnUpId(),
  newList));
      }
      else
      {
        itterUpIdToShutters->second->push_back(shutter);
      }
      auto itterDownIdToShutters = btnUpToSutters->find(shutter->GetBtnDownId());
      if (itterDownIdToShutters == btnUpToSutters->end())
      {
        std::vector<RollerShutter*>* newList = new std::vector<RollerShutter*>();
        newList->push_back(shutter);
        this->btnDownToSutters->insert(std::pair<std::string, std::vector<RollerShutter*>*>(shutter->GetBtnDownId(),
  newList));
      }
      else
      {
        itterDownIdToShutters->second->push_back(shutter);
      }
    }
  }
  */
}

/// @brief erstes Ausführen
void RollerShutterComponent::InitialRun() {
  ESP_LOGD(TAG, "Initial Run begin"); 
  for (int idx = 0; idx < this->shutters->size(); idx++)
  {
    ESP_LOGD(TAG, "Initial Run 1");
    RollerShutter *shutter = this->shutters->at(idx);    
    ESP_LOGD(TAG, "Initial Run 2");
    RL_Time *timeUD = GetTimeById(shutter->GetTimeUpDownId());
    ESP_LOGD(TAG, "Initial Run 3");
    /*
    shutter->SetTimeUpDown(timeUD);
    ESP_LOGD(TAG, "Initial Run 4");
    RL_Group *group = GetGroupById(shutter->GetGroupId());
    ESP_LOGD(TAG, "Initial Run 5");
    shutter->SetGroup(group);
    ESP_LOGD(TAG, "Initial Run 6");
    shutter->Setup();    
    ESP_LOGD(TAG, "Initial Run 7");
    /*
    if ((shutter->GetShutterState() == enRollerShutterState::isUnknown)) {
      ESP_LOGD(TAG, "Initial Run 4");
      shutter->ResetRollerShutter();
      ESP_LOGD(TAG, "Initial Run 5");
    }
    ESP_LOGD(TAG, "Initial Run 6");
    /**/
  }
  /*
  for (auto itter = this->shutters->cbegin(), last = this->shutters->cend(); itter != last; itter++) {
    ESP_LOGD(TAG, "Initial Run 1");
    RollerShutter *shutter = *itter;
    ESP_LOGD(TAG, "Initial Run 2");
    shutter->Setup();
    ESP_LOGD(TAG, "Initial Run 3");
    if ((shutter->GetShutterState() == enRollerShutterState::isUnknown)) {
      ESP_LOGD(TAG, "Initial Run 4");
      shutter->ResetRollerShutter();
      ESP_LOGD(TAG, "Initial Run 5");
    }
    ESP_LOGD(TAG, "Initial Run 6");
  } 
  */ 
  ESP_LOGD(TAG, "Initial Run End");
}

/// @brief onSetup
void RollerShutterComponent::setup() {
  ESP_LOGD(TAG, "Setup RSC '%s'", this->myName.c_str());  
  if (this->allBtnIsMaster) {
    if (std::strlen(this->btnUpId.c_str()) > 1) {
      if (this->btnDownId.compare("remote") != 0) {
        ESP_LOGD(TAG, "Get all btn up id = %s", this->btnUpId.c_str());
        this->btnUp = getBinarySensorById(this->btnUpId);
        this->btnUpIsRemote = false;
        ESP_LOGD(TAG, "Has all btn up = %s", this->btnUp->get_name().c_str());
      } else {
        this->btnUp = nullptr;
        this->btnUpIsRemote = false;
      }
      if (std::strlen(this->btnDownId.c_str()) > 1) {
        if (this->btnDownId.compare("remote") != 0) {
          ESP_LOGD(TAG, "Get all btn down id = %s", this->btnDownId.c_str());
          this->btnDown = getBinarySensorById(this->btnDownId);
          this->btnDownIsRemote = false;
          ESP_LOGD(TAG, "Has all btn down = %s", this->btnDown->get_name().c_str());
        } else {
          this->btnDown = nullptr;
          this->btnDownIsRemote = false;
        }
        if (std::strlen(this->btnHollidayId.c_str()) > 1) {
          if (this->btnDownId.compare("remote") != 0) {
            ESP_LOGD(TAG, "Get all btn holliday id = %s", this->btnHollidayId.c_str());
            this->btnHolliday = getBinarySensorById(this->btnHollidayId);
            this->btnHollidayIsRemote = false;
            ESP_LOGD(TAG, "has all btn holliday = %s", this->btnHolliday->get_name().c_str());
          } else {
            ESP_LOGD(TAG, "Has No Holliday");
            this->btnHolliday = nullptr;
            this->btnHollidayIsRemote = false;
          }
        }
        this->hasSetup = true;
      }
    }
  } else if (this->allBtnIsSlave) {
    if (this->btnDownId.compare("remote") == 0) {
      this->btnUp = nullptr;
      this->btnUpIsRemote = true;
    }
    if (std::strlen(this->btnDownId.c_str()) > 1) {
      if (this->btnDownId.compare("remote") == 0) {
        this->btnDown = nullptr;
        this->btnDownIsRemote = true;
      }
      if (std::strlen(this->btnHollidayId.c_str()) > 1) {
        if (this->btnDownId.compare("remote") == 0) {
          this->btnHolliday = nullptr;
          this->btnHollidayIsRemote = true;
        }
      }
      this->hasSetup = true;
    }
  } else {
    this->hasSetup = true;
  } 
  if (this->hasSetup)  
  {   
    // this->InitialRun();
    ESP_LOGW(TAG, "Setup war erfolgreich");   
  }
  else
  {
    ESP_LOGW(TAG, "Setup NICHT erfolgreich");          
  }
  /**/
  ESP_LOGD(TAG, "Ende aus dem Setup");        
}

/// @brief onLoop
void RollerShutterComponent::loop() {
  /*
  if (hasSetup) {
    // zu erst die Haupt-Buttons abfragen
    if (!this->btnUpIsRemote) {
      if (this->btnUp->has_state())
        this->btnUpIsPress = this->btxnUp->state;
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
    for (auto itter = this->shutters->cbegin(), last = this->shutters->cend(); itter != last; itter++) {
      RollerShutter *shutter = *itter;
      if (this->btnUpIsPress)
        shutter->SetButtonUpIsPress(true);
      else if (this->btnDownIsPress)
        shutter->SetButtonDownIsPress(true);
      else
        shutter->CheckButtons();
    }
    this->btnUpIsPress = false;
    this->btnDownIsPress = false;
    // dann abarbeiten
    for (auto itter = this->shutters->cbegin(), last = this->shutters->cend(); itter != last; itter++) {
      RollerShutter *shutter = *itter;
      shutter->MakeButtons();
      shutter->CheckTimerStartGap();
      shutter->CheckTimerStop();
    }
  }
  */
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
  ESP_LOGD(TAG, "getSwitchById in");
  for (auto *switchComponent : App.get_switches()) {
    if (strcmp(switchComponent->get_object_id().c_str(), hisId.c_str()) == 0)
    {
      ESP_LOGD(TAG, "getSwitchById found");
      return switchComponent;
    }
  }
  ESP_LOGD(TAG, "getSwitchById not found");
  return nullptr;
}

/// @brief Gibt den Sensor anhand seiner Id Zurück
/// @param hisId
/// @return
binary_sensor::BinarySensor *RollerShutterComponent::getBinarySensorById(const std::string &hisId) {
  ESP_LOGD(TAG, "getBinarySensorById in");
  for (auto *binSesorComponent : App.get_binary_sensors()) {
    if (strcmp(binSesorComponent->get_object_id().c_str(), hisId.c_str()) == 0)
    {
      ESP_LOGD(TAG, "getBinarySensorById found");
      return binSesorComponent;
    }
  }
  ESP_LOGD(TAG, "getBinarySensorById not found");
  return nullptr;
}

}  // namespace rollershutter
}  // namespace esphome
