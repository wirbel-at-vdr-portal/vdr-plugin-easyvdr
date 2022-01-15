/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <string>
#include "NotifyBase.h"
#include "IniFile.h"

class cDeviceManager;

class cDvbWatcher : public iNotifyBase {
private:
  cDeviceManager* parent;
public:
  cDvbWatcher(cDeviceManager* Parent);
  ~cDvbWatcher();
  void StartupDvb(void);
  virtual void OnNewDir(std::string Name);
  virtual void OnDelDir(std::string Name);    
};

class cDevWatcher : public iNotifyBase {
private:
  cDeviceManager* parent;
  cDvbWatcher* dvb;
  bool started;
  void CreateDvbWatch(void);
  void DeleteDvbWatch(void);
public:
  cDevWatcher(cDeviceManager* Parent);
  ~cDevWatcher();
  void StartupDvb(void);
  virtual void OnNewDir(std::string Name);
  virtual void OnDelDir(std::string Name);
  virtual void OnDelWatchedFolder(std::string Name);
};

class cDvbDevice;
class cChannel;

class cDeviceManager : public cDeviceHook {
public:
  typedef struct {
     std::string Device;
     std::string Name;
     std::string Adapter;
     std::string FirstFrontend;
     std::string Demux;
     std::string Frontends;
     std::string MultiFrontend;
     std::string Loaded;
     bool Available;
     bool Dvb;
     } cDeviceInfo;

  typedef struct {
     cDevice* device;
     bool epg;
     unsigned source;
     } cHook;

private:
  bool enabled;
  bool debug;
  TIniFile Devices;
  cDevWatcher* watch;
  size_t settle;
  size_t numDevices;
  size_t maxDevices;
  std::vector<cHook> hooks;

  void StdErr(const char* f, int l, std::string msg);
  void AddToList(cDvbDevice* device, int index, bool late);
  void AddHook(int index);

public:
  cDeviceManager(void);
  ~cDeviceManager();
  void Enabled(bool On);
  bool Enabled(void);
  void Initialize(void);
  bool Start(void);
  bool Stop(void);
  void NewAdapter(std::string Adapter);
  void DelAdapter(std::string Adapter);
  void SettlingTime(size_t ms) { settle = ms; }
  void SetDebug(bool On) { debug = On; }
  bool KnownDevice(int adapter, int demux = -1);
  int  DeviceNumber(int adapter, int demux);
  int  NumDevices(void) { return numDevices; }
  void MaxDevices(size_t num);
  int  MaxDevices(void) { return maxDevices; }
  void GetDeviceInfo(int Index, cDeviceInfo& Dest);
  void DisableEpg(int index);
  void DisableSource(int index, std::string source);
  bool DeviceProvidesTransponder(const cDevice* Device, const cChannel* Channel) const;
  bool DeviceProvidesEIT(const cDevice* Device) const;
};
