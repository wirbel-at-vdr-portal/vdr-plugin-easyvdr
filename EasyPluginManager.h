/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <tuple>
#include <thread>
#include <mutex>
#include <vdr/plugin.h>

class cEasyPluginManager {
friend class cPluginSubMenu;
public:
  static std::string PluginDirectory();
  static std::string FileExt();
private:
  bool initialized;
  bool debug;
  std::string SetupConf;
  std::string plugin_config_file;
  std::vector<std::string> AvailablePlugins;
  std::vector<std::string> Blacklist;
  std::vector<std::tuple<std::string,std::string,std::string>> PluginSetup;
  std::mutex mutex;
  cPlugin* parent;

  void  StdErr(const char* f, int l, std::string msg);
  bool  Loaded(std::string PluginName);
  bool  FileExists(std::string FileName);
  cDll* GetDll(std::string PluginName);
  std::string ConfigName(std::string PluginName);
public:
  cEasyPluginManager(void);
  ~cEasyPluginManager() {}
  void Initialize();

  void LoadSetup(std::string ConfigDir);
  bool LoadPlugin(std::string Name, std::string Arguments, bool AutoLoad = false);
  bool UnloadPlugin(std::string Name);
  bool AutoLoad(void);

  void SetParent(cPlugin* Parent);
  void SetDebug(bool On);
  void SetPluginConfigFile(std::string s);
  bool BlackListed(std::string PluginName);

public:
  typedef struct {
     std::string PluginName;
     std::string Args, prevArgs;
     int Loaded, prevLoaded;
     int AutoRun, prevAutoRun;
     int Stop, prevStop;
     } cStartupInfo;

  void GetStartupInfo(std::string PluginName, cStartupInfo& Dest);
  bool PutStartupInfo(cStartupInfo& Src);
};
