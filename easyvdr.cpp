/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <string>
#include <vector>
#include <iostream>
#include <vdr/plugin.h>
#include "IniFile.h"
#include "FileList.h"
#include "EasyPluginManager.h"
#include "DeviceManager.h"
#include "MainMenu.h"


/*******************************************************************************
 * a instance of this plugins "PluginManager", defined in "EasyPluginManager.h"
 ******************************************************************************/
cEasyPluginManager PluginManager;

/*******************************************************************************
 * a instance of this plugins "DeviceManager", defined in "DeviceManager.h"
 ******************************************************************************/
cDeviceManager DeviceManager;


/*******************************************************************************
 * the plugin itself.
 ******************************************************************************/
class cPluginEasyvdr : public cPlugin {
private:
  static constexpr const char* s_version            = "2021.01.24.1++";
  static constexpr const char* s_description        = "setup helper plugin";
  static constexpr const char* s_mainmenuentry      = "setup helper";
  static constexpr const char* s_plugin_config_file = "/etc/vdr/conf.d/*_settings.ini";
  static constexpr const char* s_configfile         = "/etc/vdr/conf.d/easyvdr.ini";

  bool debug;
  std::string description, mainmenuentry, option;
  std::string configfile;
  TIniFile Config;

  void StdErr(const char* f, int l, std::string msg); 
public:
  cPluginEasyvdr(void);
  virtual ~cPluginEasyvdr();
  virtual const char* Version(void) { return s_version; }
  virtual const char* Description(void) { return description.c_str(); }
  virtual const char* MainMenuEntry(void) { return mainmenuentry.c_str(); }
  virtual const char* CommandLineHelp(void);
  virtual bool ProcessArgs(int argc, char* argv[]);
  virtual bool Initialize(void);
  virtual bool Start(void);
  virtual void Stop(void);
  virtual void Housekeeping(void);
  virtual void MainThreadHook(void);
  virtual cString Active(void);
  virtual time_t WakeupTime(void);

  virtual cOsdObject* MainMenuAction(void);
  virtual cMenuSetupPage* SetupMenu(void);
  virtual bool SetupParse(const char* Name, const char* Value);
  virtual bool Service(const char* Id, void* Data = nullptr);
  virtual const char** SVDRPHelpPages(void);
  virtual cString SVDRPCommand(const char* Command, const char* Option, int& ReplyCode);
};

VDRPLUGINCREATOR(cPluginEasyvdr);

/* debug messages - if enabled in config. */
#define ToStdErr(msg) StdErr(__PRETTY_FUNCTION__,__LINE__,msg)





/*******************************************************************************
 * implementation of the main plugin class.
 ******************************************************************************/

cPluginEasyvdr::cPluginEasyvdr(void) :
  debug(false),
  description(s_description), mainmenuentry(s_mainmenuentry),
  configfile(s_configfile)
{
  PluginManager.SetParent(this);
}

cPluginEasyvdr::~cPluginEasyvdr() {
  ToStdErr("plugin destroyed.");
}

void cPluginEasyvdr::StdErr(const char* f, int l, std::string msg) {
  if (debug)
     std::cerr << f << ':' << l << " : " << msg << std::endl;
}

const char* cPluginEasyvdr::CommandLineHelp(void) {
  ToStdErr(" ");
  option  = "  -c <fullpath>         override default " + std::string(mainmenuentry) + " plugin config file path,\n";
  option += "                        [default: "        + std::string(s_configfile)  + "]\n";

  return option.c_str();
}

bool cPluginEasyvdr::ProcessArgs(int argc, char* argv[]) {
  option.clear();
  for(int i=1; i<argc; i++) {
     if (option.empty())
        option = argv[i];
     else {
        if (option == "-c")
           configfile = argv[i];
        else {
           esyslog("[%s:%d]: unknown option %s",
                   __PRETTY_FUNCTION__, __LINE__, option.c_str());
           return false;
           }
        option.clear();
        }
     }
  return option.empty();
}

bool cPluginEasyvdr::Initialize(void) {
  Config.ReadFile(configfile);
  if (not Config.Valid()) {
     debug = true;
     ToStdErr("ERROR - cannot read config " + configfile);
     esyslog("plugin %s: ERROR - cannot read config %s", Name(), configfile.c_str());
     return false;
     }

  description   = Config.ReadString("Common", "Description"     , s_description);
  mainmenuentry = Config.ReadString("Common", "MainMenuEntry"   , s_mainmenuentry);
  debug         = Config.ReadBool  ("Common", "PluginDebug"     , false);

  PluginManager.LoadSetup(cPlugin::ConfigDirectory());
  PluginManager.SetPluginConfigFile(Config.ReadString("Common", "PluginConfigFile", s_plugin_config_file));
  PluginManager.SetDebug(Config.ReadBool("Common", "PluginManagerDebug", false));
  PluginManager.Initialize();
  PluginManager.AutoLoad();

  DeviceManager.Enabled(Config.ReadBool("DeviceManager", "Enabled", false));
  if (DeviceManager.Enabled()) {  
     DeviceManager.MaxDevices(Config.ReadInteger("DeviceManager", "MaxDevices", 100));
     DeviceManager.SettlingTime(Config.ReadInteger("DeviceManager", "SettlingTime", 500));
     DeviceManager.SetDebug(Config.ReadBool("DeviceManager", "Debug", false));
     for(int i=0; i<MAXDEVICES; i++) {
        std::string Device("Device" + std::to_string(i));

        if (not Config.ReadBool("DeviceManager", Device + ".ProvidesEpg", true))
           DeviceManager.DisableEpg(i);
        if (not Config.ReadBool("DeviceManager", Device + ".ProvidesSourceA", true))
           DeviceManager.DisableSource(i, "A");
        if (not Config.ReadBool("DeviceManager", Device + ".ProvidesSourceC", true))
           DeviceManager.DisableSource(i, "C");
        if (not Config.ReadBool("DeviceManager", Device + ".ProvidesSourceT", true))
           DeviceManager.DisableSource(i, "T");
        if (not Config.ReadBool("DeviceManager", Device + ".ProvidesSourceT2", true))
           DeviceManager.DisableSource(i, "T2");
        if (not Config.ReadBool("DeviceManager", Device + ".ProvidesSourceS", true))
           DeviceManager.DisableSource(i, "S");
        if (not Config.ReadBool("DeviceManager", Device + ".ProvidesSourceS2", true))
           DeviceManager.DisableSource(i, "S2");
        }
     DeviceManager.Initialize();
     }
  return true;
}

bool cPluginEasyvdr::Start(void)
{
  if (DeviceManager.Enabled())
     DeviceManager.Start();

  return true;
}

void cPluginEasyvdr::Stop(void)
{
  if (DeviceManager.Enabled())
     DeviceManager.Stop();
}

void cPluginEasyvdr::Housekeeping(void)
{
  // Perform any cleanup or other regular tasks.
}

void cPluginEasyvdr::MainThreadHook(void)
{
  // Perform actions in the context of the main program thread.
  // WARNING: Use with great care - see PLUGINS.html!
}

cString cPluginEasyvdr::Active(void)
{
  // Return a message string if shutdown should be postponed
  return nullptr;
}

time_t cPluginEasyvdr::WakeupTime(void)
{
  // Return custom wakeup time for shutdown script
  return 0;
}

cOsdObject* cPluginEasyvdr::MainMenuAction(void) {
  // Perform the action when selected from the main VDR menu.
  return ShowMainMenu(mainmenuentry);
}

cMenuSetupPage* cPluginEasyvdr::SetupMenu(void)
{
  // Return a setup menu in case the plugin supports one.
  return nullptr;
}

bool cPluginEasyvdr::SetupParse(const char* Name, const char* Value)
{
  // Parse your own setup parameters and store their values.
  return false;
}

bool cPluginEasyvdr::Service(const char* Id, void* Data)
{
  // Handle custom service requests from other plugins
  return false;
}

const char** cPluginEasyvdr::SVDRPHelpPages(void)
{
  // Return help text for SVDRP commands this plugin implements
  return nullptr;
}

cString cPluginEasyvdr::SVDRPCommand(const char* Command, const char* Option, int& ReplyCode)
{
  // Process SVDRP commands this plugin implements
  return nullptr;
}
