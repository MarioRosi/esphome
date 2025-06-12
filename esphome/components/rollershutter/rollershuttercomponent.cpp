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
}

/// @brief name und Id setzten  
void RollerShutterComponent::SetIdAndName(const std::string &myId, const std::string &myName)
{
  this->myId = myId;
  this->myName = myName;
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

/// @brief ButtonAllUp ist gedrückt
void RollerShutterComponent::PressButtonAllUp()
{
  if (!this->btnUpIsPress)
  {
    this->btnUpIsPress = true;
    ESP_LOGI(TAG, "Press button all up raise");
  }
}

/// @brief ButtonAllDown ist gedrückt
void RollerShutterComponent::PressButtonAllDown()
{
  if (!this->btnDownIsPress)
  {
    this->btnDownIsPress = true;
    ESP_LOGI(TAG, "Press button all down raise");
  }
}

/// @brief Ferien einschalten
void RollerShutterComponent::SetIsOnHollidayOn()
{
  if (!this->btnHollidayIsOn)
  {
    this->btnHollidayIsOn = true;
    ESP_LOGI(TAG, "Button holliday on");
  }
}

/// @brief Ferien ausschalten
void RollerShutterComponent::SetIsOnHollidayOff()
{
  if (this->btnHollidayIsOn)
  {
    this->btnHollidayIsOn = false;
    ESP_LOGI(TAG, "Button holliday off");
  }
}


/// @brief onLoop
void RollerShutterComponent::loop() {  
  if (hasSetup) {
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
}

}  // namespace rollershutter
}  // namespace esphome
