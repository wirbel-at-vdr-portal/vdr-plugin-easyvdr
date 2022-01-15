/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include "MainMenu.h"
#include <string>
#include <vector>
#include <iostream>
#include <cstdio>        // snprintf()
#include <vdr/osdbase.h> // cOsdItem
#include <vdr/skins.h>   // mcSetupMisc
#include <vdr/device.h>  // MAXDEVICES
#include "EasyPluginManager.h"
#include "DeviceManager.h"

/*******************************************************************************
 * forward declarations
 *******************************************************************************/
//#define DEBUG
extern cEasyPluginManager PluginManager;
extern cDeviceManager DeviceManager;
#ifdef DEBUG
   static std::string PrintState(eOSState State);
   static std::string PrintKey(eKeys Key);
   #define ToStdErr(msg) \
   std::cerr << __PRETTY_FUNCTION__ << ':' << __LINE__ << " : " << msg << std::endl;
#else
   #define PrintState(State)
   #define PrintKey(Key)
   #define ToStdErr(msg)
#endif


/*******************************************************************************
 * cDeviceSubMenu, show all known devices and the settings for them.
 *******************************************************************************/

class cDeviceSubItem : public cOsdItem {
private:
  std::string name;
  int index;
public:
  cDeviceSubItem(const char* Text, std::string Name, int Index);
  int Index(void) { return index; }
  std::string Name(void) { return name; }
};

cDeviceSubItem::cDeviceSubItem(const char* Text, std::string Name, int Index)
 : cOsdItem(Text), name(Name), index(Index) {}

class cDeviceSubMenu : public cMenuSetupPage {
private:
  TIniFile Devices;
  //virtual void Set(void);
public:
  cDeviceSubMenu(void);
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Store(void) {}
};

cDeviceSubMenu::cDeviceSubMenu(void) {
  SetMenuCategory(mcSetupPlugins);
//SetSection(tr("Devices"));
//SetHasHotkeys();


  static cDeviceManager::cDeviceInfo info[MAXDEVICES];
  for(int i = 0; i<cDevice::NumDevices(); i++) {
     DeviceManager.GetDeviceInfo(i,info[i]);

     if (info[i].Available) {
        // text formatting
        info[i].Name.insert         (0, "   " + info[i].Device + ".Name          = ");
        info[i].Adapter.insert      (0, "   " + info[i].Device + ".Adapter       = ");
        info[i].FirstFrontend.insert(0, "   " + info[i].Device + ".FirstFrontend = ");
        info[i].Demux.insert        (0, "   " + info[i].Device + ".Demux         = ");
        info[i].Frontends.insert    (0, "   " + info[i].Device + ".NumFrontends  = ");
        info[i].MultiFrontend.insert(0, "   " + info[i].Device + ".MultiFrontend = ");
        info[i].Loaded.insert       (0, "   " + info[i].Device + ".Loaded        = ");

        info[i].Device.insert(0, "--- ");
        info[i].Device.append(" ---");

        Add(new cOsdItem(info[i].Device.c_str()));
        Add(new cOsdItem(info[i].Name.c_str()));
        Add(new cOsdItem(info[i].Adapter.c_str()));
        Add(new cOsdItem(info[i].FirstFrontend.c_str()));
        Add(new cOsdItem(info[i].Demux.c_str()));
        Add(new cOsdItem(info[i].Frontends.c_str()));
        Add(new cOsdItem(info[i].MultiFrontend.c_str()));
        Add(new cOsdItem(info[i].Loaded.c_str()));
        }
     else if (not info[i].Dvb) {
        info[i].Name.insert(0, "   " + info[i].Device + ".Name          = ");

        info[i].Device.insert(0, "--- ");
        info[i].Device.append(" ---");

        Add(new cOsdItem(info[i].Device.c_str()));
        Add(new cOsdItem(info[i].Name.c_str()));
        }
     }

}

eOSState cDeviceSubMenu::ProcessKey(eKeys Key) {
  eOSState state = HasSubMenu() ? /*cMenuSetupBase*/ cMenuSetupPage::ProcessKey(Key) : cOsdMenu::ProcessKey(Key);

  if (Key != kNone) ToStdErr(PrintKey(Key));
  if (state != osUnknown) { PrintState(state); }

  if (Key == kOk) {
     if (state == osUnknown) {
        cDeviceSubItem* item = (cDeviceSubItem*) Get(Current());
        return osContinue;
        if (item) {
           //return AddSubMenu(new cDeviceDetails(item->Name()));
           }
        }
     else if (state == osContinue) {
        cOsdProvider::UpdateOsdSize(true);
        Display();
        }
     }
  return state;
}



/*******************************************************************************
 * cPluginStartMenu, here all the work for plugins is done.
 *******************************************************************************/

class cPluginStartMenu : public cMenuSetupPage {
private:
  cEasyPluginManager::cStartupInfo si;
  char ArgBuf[256];

  void Set(void);
public:
  cPluginStartMenu(std::string PluginName);
  virtual void Store(void);
  eOSState ProcessKey(eKeys Key);
};

cPluginStartMenu::cPluginStartMenu(std::string PluginName) {
  PluginManager.GetStartupInfo(PluginName, si);
  if (PluginManager.BlackListed(PluginName))
     si.Stop = false;

  snprintf(ArgBuf, sizeof(ArgBuf), "%s", si.Args.c_str()); 

  SetMenuCategory(mcSetupPlugins);
  SetTitle((tr("Plugin ") + si.PluginName).c_str());
  Set();
}

void cPluginStartMenu::Set(void) {
  ToStdErr("entering function.");

  if (si.Stop)
     SetHelp(tr("Stop"), tr("Start"));
  else
     SetHelp(nullptr   , tr("Start"));
  Add(new cMenuEditBoolItem(tr("AutoRun"), &si.AutoRun));
  Add(new cMenuEditStrItem( tr("Arguments"), ArgBuf, sizeof(ArgBuf)));
}

void cPluginStartMenu::Store(void) {
  ToStdErr("ok button..");

  si.Args = (char*) ArgBuf;

  bool modified = false;
  modified |= si.Args    != si.prevArgs;
  modified |= si.Loaded  != si.prevLoaded;
  modified |= si.AutoRun != si.prevAutoRun;
  modified |= si.Stop    != si.prevStop;

  if (not modified) return;

  ToStdErr("modified!");
  PluginManager.PutStartupInfo(si);
}

eOSState cPluginStartMenu::ProcessKey(eKeys Key) {
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (Key != kNone) ToStdErr(PrintKey(Key));
  if (state != osUnknown) { PrintState(state); }

  if (state == osUnknown) {
     switch (Key) {
        case kOk:
           Store();
           state = osBack;
           break;
       case kGreen:
           si.Args = (char*) ArgBuf;
           PluginManager.LoadPlugin(si.PluginName, si.Args);
           if (PluginManager.BlackListed(si.PluginName))
              SetHelp(nullptr   , tr("Start"));
           break;
       case kRed:
           PluginManager.UnloadPlugin(si.PluginName);
           break;
       default: break;
       }
     }
  return state;
}



/*******************************************************************************
 * cPluginSubMenuItem, a specific Item in cPluginSubMenu
 *******************************************************************************/
class cPluginSubMenuItem : public cOsdItem {
private:
  std::string pluginname;
  int index;
public:
  cPluginSubMenuItem(const char* Text, std::string Name, int Index);
  int Index(void) { return index; }
  std::string PluginName(void) { return pluginname; }
};

cPluginSubMenuItem::cPluginSubMenuItem(const char* Text, std::string Name, int Index)
 : cOsdItem(Text), pluginname(Name), index(Index) {} 


/*******************************************************************************
 * cPluginSubMenu, Lists all available Plugins.
 *******************************************************************************/
class cPluginSubMenu : public cMenuSetupPage {
public:
  cPluginSubMenu(void);
  virtual eOSState ProcessKey(eKeys Key);
  virtual void Store(void) {}
};

cPluginSubMenu::cPluginSubMenu(void) {
  SetMenuCategory(mcSetupPlugins);
  SetSection(tr("Plugins"));
  SetHasHotkeys();
  int i=0;
  for(auto s:PluginManager.AvailablePlugins) {
     Add(new cPluginSubMenuItem(hk(s.c_str()), s, i++));
     }
}

eOSState cPluginSubMenu::ProcessKey(eKeys Key) {
  eOSState state = HasSubMenu() ? /*cMenuSetupBase*/ cMenuSetupPage::ProcessKey(Key) : cOsdMenu::ProcessKey(Key);

  if (Key != kNone) ToStdErr(PrintKey(Key));
  if (state != osUnknown) { PrintState(state); }

  if (Key == kOk) {
     if (state == osUnknown) {
        cPluginSubMenuItem* item = (cPluginSubMenuItem*) Get(Current());
        if (item) {
           return AddSubMenu(new cPluginStartMenu(item->PluginName()));
           }
        }
     else if (state == osContinue) {
        cOsdProvider::UpdateOsdSize(true);
        Display();
        }
     }
  return state;
}


/*******************************************************************************
 * cMainMenu
 *******************************************************************************/

class cMainMenu : public cOsdMenu {
private:
  std::string name;
  virtual void Set();
public:
  cMainMenu(std::string Name);
  virtual eOSState ProcessKey(eKeys Key);
};

cMainMenu::cMainMenu(std::string Name) : cOsdMenu(""), name(Name) {
  SetMenuCategory(mcSetup);
  Set();
}

static const char* MainMenuEntries[] = {
  "Plugins",    //case osUser1
  "Devices",    //case osUser2
  nullptr,      //case osUser3
  nullptr,      //case osUser4
  nullptr,      //case osUser5
  nullptr,      //case osUser6
  nullptr,      //case osUser7
  nullptr,      //case osUser8
  nullptr,      //case osUser9
  nullptr,      //case osUser10
  nullptr       //end marker.
};

void cMainMenu::Set() {
  std::string title = tr("Setup") + std::string(" - ") + name;

  Clear();
  SetTitle(title.c_str());
  SetHasHotkeys();
 
  for(int i=0; MainMenuEntries[i]; i++) {
     Add(new cOsdItem(hk(tr(MainMenuEntries[i])), (eOSState)((int) osUser1 + i)));
     }
}

eOSState cMainMenu::ProcessKey(eKeys Key) {
  if (Key != kNone) ToStdErr(PrintKey(Key));

  int osdLanguage = I18nCurrentLanguage();
  eOSState state = cOsdMenu::ProcessKey(Key);
  if (state == osUnknown) return state;

  #ifdef DEBUG
  ToStdErr(PrintState(state));
  #endif

  switch(state) {
     case osUser1 : return AddSubMenu(new cPluginSubMenu);
     case osUser2 : return AddSubMenu(new cDeviceSubMenu);
   //case osUser3 : return AddSubMenu(new );
   //case osUser4 : return AddSubMenu(new );
   //case osUser5 : return AddSubMenu(new );
   //case osUser6 : return AddSubMenu(new );
   //case osUser7 : return AddSubMenu(new );
   //case osUser8 : return AddSubMenu(new );
   //case osUser9 : return AddSubMenu(new );
   //case osUser10: return AddSubMenu(new );
     default:;
     }
  if (I18nCurrentLanguage() != osdLanguage) {
     Set();
     if (!HasSubMenu())
        Display();
     }
  return state;
}



/*******************************************************************************
 * VDR's main menu function can do one of three things:
 * 1. Return a pointer to a cOsdMenu object which will be displayed as a submenu
 *    of the main menu (just like the Recordings menu, for instance). That menu
 *    can then implement further functionality and, for instance, could
 *    eventually start a custom player to replay a file other than a VDR recording.
 * 2. Return a pointer to a cOsdObject object which will be displayed instead of
 *    the normal menu. The derived cOsdObject can open a raw OSD from within its
 *    Show() function (it should not attempt to do so from within its constructor,
 *    since at that time the OSD is still in use by the main menu).
 *    See the 'osddemo' example that comes with VDR for a demonstration of how
 *    this is done.
 * 3. Perform a specific action and return NULL. In that case the main menu will
 *    be closed after calling MainMenuAction(). 
 ******************************************************************************/

cOsdObject* ShowMainMenu(std::string Name) {
  return new cMainMenu(Name);
}






#ifdef DEBUG
/*******************************************************************************
 * debugging functions.
 *******************************************************************************/

static std::string PrintState(eOSState State) {
 switch(State) {
    case osUnknown      : return "osUnknown";
    case osContinue     : return "osContinue";
    case osSchedule     : return "osSchedule";
    case osChannels     : return "osChannels";
    case osTimers       : return "osTimers";
    case osRecordings   : return "osRecordings";
    case osPlugin       : return "osPlugin";
    case osSetup        : return "osSetup";
    case osCommands     : return "osCommands";
    case osPause        : return "osPause";
    case osRecord       : return "osRecord";
    case osReplay       : return "osReplay";
    case osStopRecord   : return "osStopRecord";
    case osStopReplay   : return "osStopReplay";
    case osCancelEdit   : return "osCancelEdit";
    case osBack         : return "osBack";
    case osEnd          : return "osEnd";
    case os_User        : return "os_User";     
    case osUser1        : return "osUser1";     
    case osUser2        : return "osUser2";     
    case osUser3        : return "osUser3";     
    case osUser4        : return "osUser4";     
    case osUser5        : return "osUser5";     
    case osUser6        : return "osUser6";     
    case osUser7        : return "osUser7";     
    case osUser8        : return "osUser8";     
    case osUser9        : return "osUser9";     
    case osUser10       : return "osUser10";    
    default             : return "Unknown State";
    }
}


static std::string PrintKey(eKeys Key) {
 switch(Key) {
    case kUp            : return "kUp";
    case kDown          : return "kDown";
    case kMenu          : return "kMenu";
    case kOk            : return "kOk";
    case kBack          : return "kBack";
    case kLeft          : return "kLeft";
    case kRight         : return "kRight";
    case kRed           : return "kRed";
    case kGreen         : return "kGreen";
    case kYellow        : return "kYellow";
    case kBlue          : return "kBlue";
    case k0...k9        : return std::string("k") + (char) ('0' + (Key - '0'));
    case kInfo          : return "kInfo";
    case kPlayPause     : return "kPlayPause";
    case kPlay          : return "kPlay";
    case kPause         : return "kPause";
    case kStop          : return "kStop";
    case kRecord        : return "kRecord";
    case kFastFwd       : return "kFastFwd";
    case kFastRew       : return "kFastRew";
    case kNext          : return "kNext";
    case kPrev          : return "kPrev";
    case kPower         : return "kPower";
    case kChanUp        : return "kChanUp";
    case kChanDn        : return "kChanDn";
    case kChanPrev      : return "kChanPrev";
    case kVolUp         : return "kVolUp";
    case kVolDn         : return "kVolDn";
    case kMute          : return "kMute";
    case kAudio         : return "kAudio";
    case kSubtitles     : return "kSubtitles";
    case kSchedule      : return "kSchedule";
    case kChannels      : return "kChannels";
    case kTimers        : return "kTimers";
    case kRecordings    : return "kRecordings";
    case kSetup         : return "kSetup";
    case kCommands      : return "kCommands";
    case kUser0...kUser9: return std::string("kUser") + (char) ('0' + (Key - '0'));
    case kNone          : return "kNone";
    case kKbd           : return "kKbd";
    case k_Plugin       : return "k_Plugin";
    case k_Setup        : return "k_Setup";
    default             : return "Unknown Key";
    }
}
#endif
