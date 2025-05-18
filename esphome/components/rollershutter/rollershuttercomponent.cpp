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

static const char *TAG = "RollerShutterComponent.component";

/// @brief Konstruktor
RollerShutterComponent::RollerShutterComponent() {
  ESP_LOGI(TAG, "Start aus dem Konstruktor");
  this->movingTimes = new std::vector<RL_Time *>();
  this->groups = new std::vector<RL_Group *>();
  this->shutters = new std::vector<RollerShutter *>();
  this->isStarted = true;
  this->hasSetup = false;
  this->btnUpIsRemote = false;
  this->btnDownIsRemote = false;
  this->btnHollidayIsRemote = false;
  ESP_LOGI(TAG, "Ende aus dem Konstruktor");
}

/// @brief name und Id setzten  
void RollerShutterComponent::SetIdAndName(std::string myId, std::string myName)
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
void RollerShutterComponent::SetButtons(std::string btnUpId, std::string btnDownId, std::string btnHollidayId,
                                        bool allInputIsMaster, bool allInputIsSlave) {
  ESP_LOGI(TAG, "SetButtons");
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
/// @param millisecondUp 
/// @param millisecondDown 
/// @param millisecondGap 
void RollerShutterComponent::AddTime(std::string id, int millisecondUp, int millisecondDown, int millisecondGap)
{
  ESP_LOGI(TAG, "AddTime");
  RL_Time *item = new RL_Time(id, millisecondUp, millisecondDown, millisecondGap);
  this->movingTimes->push_back(item); 
}

/// @brief Fügt eine Gruppe mit Sundowner der Liste hinzu
/// @param id 
/// @param name 
/// @param monthFrom 
/// @param monthTo 
/// @param gapHour 
/// @param gapMinute 
/// @param upHoure 
/// @param upMinute 
void RollerShutterComponent::AddGroup(std::string id, std::string name, 
  int monthFrom, int monthTo, int gapHour, int gapMinute, int upHoure, int upMinute)
{
  ESP_LOGI(TAG, "AddGroup mit SD");
  RL_SunDowner *sd = new RL_SunDowner(monthFrom, monthTo, gapHour, gapMinute, upHoure, upMinute);
  RL_Group *item = new RL_Group(id, name, sd);
  this->groups->push_back(item); 
}

/// @brief Fügt eine Gruppe ohne / mit deaktiviertem Sundowner der Liste hinzu
/// @param id 
/// @param name 
void RollerShutterComponent::AddGroup(std::string id, std::string name)
{
  ESP_LOGI(TAG, "AddGroup ohne SD");
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
void RollerShutterComponent::AddShutter(std::string id, std::string name, std::string idGroup, std::string idTime,
                                        std::string btnUpId, std::string btDownId, std::string relUpId,
                                        std::string relDownId) {
  ESP_LOGI(TAG, "AddShutter");
  RL_Time *time = getTimeById(idTime);
  RL_Group *group = getGroupById(idGroup);
  RollerShutter *shutter = new RollerShutter(id, name, group, time, this, btnUpId, btnDownId, relUpId, relDownId);
  shutters->push_back(shutter);
}

/// @brief Gibt die Time entsprechend der Id zurück
/// @param id
/// @return default nullptr
RL_Time *RollerShutterComponent::getTimeById(std::string id) {
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
RL_Group *RollerShutterComponent::getGroupById(std::string id) {
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
  ESP_LOGI(TAG, "Initial Run");
  for (auto itter = this->shutters->cbegin(), last = this->shutters->cend(); itter != last; itter++) {
    RollerShutter *shutter = *itter;
    shutter->Setup();
    if ((shutter->GetShutterState() == enRollerShutterState::isUnknown)) {
      shutter->ResetRollerShutter();
    }
  }
}

/// @brief onSetup
void RollerShutterComponent::setup() {
  ESP_LOGD(TAG, "Hallo aus dem Setup");
  InitialRun();
  if (this->allBtnIsMaster) {
    if (std::strlen(this->btnUpId.c_str()) > 1) {
      if (this->btnDownId.compare("remote") != 0) {
        this->btnUp = getBinarySensorById(this->btnUpId);
        this->btnUpIsRemote = false;
      } else {
        this->btnUp = nullptr;
        this->btnUpIsRemote = false;
      }
      if (std::strlen(this->btnDownId.c_str()) > 1) {
        if (this->btnDownId.compare("remote") != 0) {
          this->btnDown = getBinarySensorById(this->btnDownId);
          this->btnDownIsRemote = false;
        } else {
          this->btnDown = nullptr;
          this->btnDownIsRemote = false;
        }
        if (std::strlen(this->btnHollidayId.c_str()) > 1) {
          if (this->btnDownId.compare("remote") != 0) {
            this->btnHolliday = getBinarySensorById(this->btnHollidayId);
            this->btnHollidayIsRemote = false;
          } else {
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
    ESP_LOGD(TAG, "Ende aus dem Setup");
    ESP_LOGI(TAG, "Ende aus dem Setup");
    ESP_LOGVV(TAG, "Ende aus dem Setup");
}

/// @brief onLoop
void RollerShutterComponent::loop() {
  if (hasSetup) {
    // zu erst die Haupt-Buttons abfragen
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
}

/// @brief Dump-Config
void RollerShutterComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "Rollershutter");
  ESP_LOGCONFIG(TAG, "Anzahl der Rolläden = ", this->shutters->size());
  if (this->allBtnIsMaster) {
    ESP_LOGCONFIG(TAG, "Alles Runter, ich bin Master = ", !this->btnDownIsRemote);
    ESP_LOGCONFIG(TAG, "Alles Hoch, ich bin Master = ", !this->btnUpIsRemote);
    ESP_LOGCONFIG(TAG, "Urlaubsschalter, ich bin Master = ", !this->btnHollidayIsRemote);
  } else if (this->allBtnIsMaster) {
    ESP_LOGCONFIG(TAG, "Alles Runter, ich bin Remote = ", this->btnDownIsRemote);
    ESP_LOGCONFIG(TAG, "Alles Hoch, ich bin Remote = ", this->btnUpIsRemote);
    ESP_LOGCONFIG(TAG, "Urlaubsschalter, ich bin Remote = ", this->btnHollidayIsRemote);
  } else {
    ESP_LOGCONFIG(TAG, "Alles Runter, wird nicht genutzt");
    ESP_LOGCONFIG(TAG, "Alles Hoch, wird nicht genutzt");
    ESP_LOGCONFIG(TAG, "Urlaubsschalter, wird nicht genutzt");
  }
}

/// @brief Gibt das Switch anhand seiner Id zurück
/// @param hisId
/// @return
switch_::Switch *RollerShutterComponent::getSwitchById(const std::string hisId) {
  for (auto *switchComponent : App.get_switches()) {
    if (strcmp(switchComponent->get_object_id().c_str(), hisId.c_str()) == 0)
      return switchComponent;
  }
  return nullptr;
}

/// @brief Gibt den Sensor anhand seiner Id Zurück
/// @param hisId
/// @return
binary_sensor::BinarySensor *RollerShutterComponent::getBinarySensorById(const std::string hisId) {
  for (auto *binSesorComponent : App.get_binary_sensors()) {
    if (strcmp(binSesorComponent->get_object_id().c_str(), hisId.c_str()) == 0)
      return binSesorComponent;
  }
  return nullptr;
}

}  // namespace rollershutter
}  // namespace esphome
