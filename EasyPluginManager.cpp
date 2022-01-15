/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>
#include <thread>
#include <cstring> // strlen()
#include <vdr/plugin.h>
#include <vdr/device.h>
#include <vdr/skins.h>
#include "EasyPluginManager.h"
#include "FileList.h"
#include "IniFile.h"

/*******************************************************************************
 * a pointer to VDR's internal Plugin Manager, defined in <vdr/plugin.h>
 ******************************************************************************/
cPluginManager* VdrPluginManager;




/*******************************************************************************
 * cEasyPluginManager
 ******************************************************************************/

cEasyPluginManager::cEasyPluginManager(void) :
  initialized(false), debug(false)
{
  VdrPluginManager = cPluginManager::pluginManager;
}

void cEasyPluginManager::LoadSetup(std::string ConfigDir) {
  std::string SetupConf = std::move(ConfigDir);
  SetupConf.resize(SetupConf.size() - strlen("plugins"));
  SetupConf += "setup.conf";

  std::ifstream is(SetupConf.c_str());
  if (is) {
     std::stringstream ss;
     ss << is.rdbuf();
     is.close();
     std::string s;
     while(std::getline(ss, s)) {
        size_t delim = s.find(" = ");
        if (delim == std::string::npos) continue;

        std::string Plugin, Name, Value;
        Name  = s.substr(0, delim);
        Value = s.substr(delim + 3);

        delim = Name.find(".");
        if (delim == std::string::npos) continue;

        Plugin = Name.substr(0, delim);
        Name   = Name.substr(delim + 1);
        PluginSetup.push_back(std::make_tuple(Plugin, Name, Value));
        }
     }
}

#define ToStdErr(msg) StdErr(__PRETTY_FUNCTION__,__LINE__,msg)

void cEasyPluginManager::StdErr(const char* f, int l, std::string msg) {
  if (debug)
     std::cerr << f << ':' << l << " : " << msg << std::endl;
}

cDll* cEasyPluginManager::GetDll(std::string PluginName) {
  for(cDll* d = VdrPluginManager->dlls.First(); d; d = VdrPluginManager->dlls.Next(d)) {
     if (PluginName == d->Plugin()->Name()) return d;
     }
  return nullptr;
}

bool cEasyPluginManager::UnloadPlugin(std::string Name) {
  if (not Loaded(Name)) {
     ToStdErr("plugin " + Name + " not loaded.");
     return false;
     }
  if (BlackListed(Name))
     return false;

  cDll* d = GetDll(Name);

  if (d == nullptr) {
     ToStdErr("plugin " + Name + " not found.");
     return false;
     }
  {
  const std::lock_guard<std::mutex> LockGuard(mutex);
  VdrPluginManager->dlls.Del(d, false);
  }

  cPlugin* plugin = d->Plugin();
  if (plugin && plugin->started) {
     plugin->started = false;
     plugin->Stop();
     std::this_thread::sleep_for(std::chrono::seconds(3)); 
     }
  delete d;
  Skins.Message(mtInfo, tr("Plugin stopped."));
  return true;
}

bool cEasyPluginManager::AutoLoad(void) {
  for(auto name:AvailablePlugins) {
     if (Loaded(name)) {
        if (name != parent->Name())
           ToStdErr(name + " plugin already loaded - skip auto load");
        continue;
        }
     std::string ConfigFile = ConfigName(name);
     if (not FileExists(ConfigFile)) {
        ToStdErr("plugin " + name + ": skipped");
        continue;
        }

     TIniFile ini(ConfigFile);
     if (not ini.Valid()) {
        ToStdErr(name + ": '" + ConfigFile + "' syntax error - skip auto load");
        continue;
        }
     if (not ini.ReadBool("EasyPluginManager", "AutoRun", false)) {
        ToStdErr("plugin " + name + ": skipped");
        continue;
        }

     LoadPlugin(name, ini.ReadString("EasyPluginManager", "Args", ""), true);
     }
  return true;
}

bool cEasyPluginManager::LoadPlugin(std::string Name, std::string Arguments, bool AutoLoad) {
  if (Loaded(Name)) {
     ToStdErr("plugin " + Name + " already loaded.");
     return false;
     }

  int numdevices = cDevice::NumDevices();
  std::string args(Name);
  if (not Arguments.empty())
     args += ' ' + Arguments;

  VdrPluginManager->AddPlugin(args.c_str());
  cDll* ThisDll = VdrPluginManager->dlls.Last();

  if (not ThisDll->Load(true)) {
     delete ThisDll;
     ToStdErr("could not load " + args);
     return false;
     }

  cPlugin* plugin = ThisDll->Plugin();

  /* forward setup values to plugin */
  for(auto t:PluginSetup) {
     if (std::get<0>(t) != Name) continue;
     plugin->SetupParse(std::get<1>(t).c_str(), std::get<2>(t).c_str());
     }

  /* if early on AutoLoad, VDR itself will call later Initialize() and Start() */
  if (not AutoLoad) {
     if (not plugin->Initialize()) {
        delete ThisDll;
        ToStdErr("could not initialize plugin " + Name);
        return false;
        }
     ToStdErr("initialized " + Name);
     
     if (not plugin->Start()) {
        delete ThisDll;
        ToStdErr("could not start plugin " + Name);
        return false;
        }
     }
  
  if (numdevices != cDevice::NumDevices()) {
     ToStdErr("Add '" + Name + "' to BlackList");
     Blacklist.push_back(Name);
     }

  if (not AutoLoad)
     Skins.Message(mtInfo, tr("Plugin started."));
  ToStdErr("plugin " + Name + " started.");
  return true;
}


void cEasyPluginManager::Initialize() {
  if (initialized) return;

  cFileList list(PluginDirectory(), FileExt());

  for(auto s:list.List()) {
     size_t p = s.find(FileExt());
     if (p == std::string::npos)
        continue;
     else
        s.erase(p, std::string::npos);

     p = s.find("libvdr-");
     if (p != 0)
        continue;
     else
        s.erase(0, strlen("libvdr-"));

     if (s == parent->Name())
        continue;

     AvailablePlugins.push_back(s);
     }
  std::sort(AvailablePlugins.begin(), AvailablePlugins.end());
  initialized = true;
}

bool cEasyPluginManager::Loaded(std::string PluginName) {
  const std::lock_guard<std::mutex> LockGuard(mutex);
  return (VdrPluginManager->GetPlugin(PluginName.c_str()) != nullptr);
}

bool cEasyPluginManager::BlackListed(std::string PluginName) {
  return (std::find(Blacklist.begin(), Blacklist.end(), PluginName) != Blacklist.end());
}

bool cEasyPluginManager::FileExists(std::string FileName) {
  return (access(FileName.c_str(), R_OK) == 0); 
}

std::string cEasyPluginManager::PluginDirectory() {
  return VdrPluginManager->directory;
}

std::string cEasyPluginManager::FileExt() {
  return std::string(".so.") + APIVERSION;
}

void cEasyPluginManager::SetParent(cPlugin* Parent) {
  parent = Parent;
}

void cEasyPluginManager::SetDebug(bool On) {
  debug = On;
}

void cEasyPluginManager::SetPluginConfigFile(std::string s) {
  plugin_config_file = s;
}

std::string cEasyPluginManager::ConfigName(std::string PluginName) {
  size_t p = plugin_config_file.find('*');
  return plugin_config_file.substr(0, p) + PluginName + plugin_config_file.substr(p+1, std::string::npos);
}

void cEasyPluginManager::GetStartupInfo(std::string PluginName, cStartupInfo& Dest) {
  Dest.PluginName = PluginName;
  Dest.Args = "";
  Dest.Loaded = Loaded(PluginName)?1:0;
  Dest.AutoRun = 0;
  Dest.Stop = 1;

  std::string ConfigFile = ConfigName(PluginName);
  if (FileExists(ConfigFile)) {
     TIniFile ini(ConfigFile);
     if (ini.Valid()) {
        Dest.Args    = ini.ReadString("EasyPluginManager", "Args"   , "");
        Dest.AutoRun = ini.ReadBool  ("EasyPluginManager", "AutoRun", false)?1:0;
        Dest.Stop    = ini.ReadBool  ("EasyPluginManager", "Stop"   , true)?1:0;
        }
     }
  Dest.prevArgs    = Dest.Args;
  Dest.prevLoaded  = Dest.Loaded;
  Dest.prevAutoRun = Dest.AutoRun;
  Dest.prevStop    = Dest.Stop;
}

bool cEasyPluginManager::PutStartupInfo(cStartupInfo& Src) {
  std::string ConfigFile = ConfigName(Src.PluginName);
  TIniFile ini(ConfigFile);
  ini.WriteString("EasyPluginManager", "Args"   , Src.Args);
  ini.WriteBool  ("EasyPluginManager", "AutoRun", Src.AutoRun);
  ini.WriteBool  ("EasyPluginManager", "Stop"   , Src.Stop);
  return ini.UpdateFile();
}
