/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <iostream>
#include <algorithm>
#include <vdr/device.h>
#include <vdr/dvbdevice.h>
#include <vdr/channels.h>
#include "DeviceManager.h"
#include "NotifyBase.h"
#define ToStdErr(msg) if (debug) StdErr(__PRETTY_FUNCTION__,__LINE__,msg)

/*******************************************************************************
 * cDeviceManager
 ******************************************************************************/
cDeviceManager::cDeviceManager(void)
  : enabled(false), debug(false), numDevices(0), maxDevices(0)
{
  watch = new cDevWatcher(this);
  for(size_t i = 0; i < MAXDEVICES; i++)
     if (cDevice::UseDevice(i)) maxDevices++;
}

cDeviceManager::~cDeviceManager() {
  delete watch;
}

void cDeviceManager::StdErr(const char* f, int l, std::string msg) {
  if (debug)
     std::cerr << f << ':' << l << " : " << msg << std::endl;
}

void cDeviceManager::DisableEpg(int index) {
  Devices.WriteBool("device" + std::to_string(index), "ProvidesEpg", false);  
}

void cDeviceManager::DisableSource(int index, std::string source) {
  Devices.WriteBool("device" + std::to_string(index), "ProvidesSource" + source, false);
}

void cDeviceManager::AddToList(cDvbDevice* device, int index, bool late) {
  std::string Device  ("device"   + std::to_string(index));
  std::string Adapter ("adapter"  + std::to_string(device->Adapter()));
  std::string Frontend("frontend" + std::to_string(device->Frontend()));
  std::string Demux   ("demux"    + std::to_string(device->Frontend())); /* really - see dvbdevice.c:1941 */
  size_t frontends = cFileList("/dev/dvb/" + Adapter, "frontend").List().size();
  size_t demuxes   = cFileList("/dev/dvb/" + Adapter, "demux").List().size();

  numDevices++;
  Devices.WriteString (Device, "Adapter", Adapter);
  Devices.WriteString (Device, "FirstFrontend", Frontend);
  Devices.WriteString (Device, "Demux", Demux);
  Devices.WriteInteger(Device, "Frontends", frontends);
  Devices.WriteBool   (Device, "MultiFrontend", frontends > demuxes);
  Devices.WriteBool   (Device, "Late", late);
  Devices.WriteBool   (Device, "IsDvb", true);

}

void cDeviceManager::Initialize(void) {
  ToStdErr("begin");
  for(int i = 0; i < cDevice::NumDevices(); i++) {
     cDevice* device = cDevice::GetDevice(i);
     cDvbDevice* d = dynamic_cast<cDvbDevice*>(device);
     if (d != nullptr)
        AddToList(d, i, false);
     AddHook(i);
     }
  std::string b(Setup.DeviceBondings);
  Devices.WriteString("Device", "Bondings", b);
  Devices.WriteBool("Device", "UseBonding", b.find_first_of("123456789") != std::string::npos);
}

bool cDeviceManager::Start(void) {
  ToStdErr("begin");
  watch->StartupDvb();
  return true;
}

bool cDeviceManager::Stop(void) {
  ToStdErr("");
  delete watch;
  watch = nullptr;
  return true;
}

bool cDeviceManager::KnownDevice(int adapter, int demux) {
  return DeviceNumber(adapter, demux) != -1;
}

int cDeviceManager::DeviceNumber(int adapter, int demux) {
  std::string Adapter ("adapter"  + std::to_string(adapter));
  std::string Demux("demux" + std::to_string(demux));

  for(int i = 0; i < cDevice::NumDevices(); i++) {
     std::string Device("device"   + std::to_string(i));

     if (Devices.ReadString(Device, "Adapter", "?") != Adapter)
        continue;

     if ((demux < 0) or (Devices.ReadString(Device, "Demux", "?") == Demux))
        return i;
     }
  return -1;
}

void cDeviceManager::NewAdapter(std::string Adapter) {
  std::string path("/dev/dvb/" + Adapter);
  int adapter = std::stoi(Adapter.substr(7));

  if (not KnownDevice(adapter)) {
     ToStdErr(Adapter + " not known.");
     std::this_thread::sleep_for(std::chrono::milliseconds(settle));
     }

  for(auto Demux:cFileList(path, "demux").List()) {
     if (numDevices >= maxDevices) {
        ToStdErr("maxDevices reached.");
        break;
        }

     int demux = std::stoi(Demux.substr(5));

     if (KnownDevice(adapter, demux)) {
        ToStdErr(Adapter + " already known.");
        continue;
        }

     std::string FirstFrontend;
     FirstFrontend = path + "/frontend" + std::to_string(demux);
     if (not FileExists(FirstFrontend)) {
        ToStdErr(Adapter + " -> no frontend.");
        continue;
        }

     ToStdErr("Add "  +path + "/frontend" + std::to_string(demux));                        
     int index = cDevice::NumDevices();
     AddToList(new cDvbDevice(adapter, demux), index, true);
     AddHook(index);
     if (Devices.ReadBool("Device", "UseBonding", false)) {
        ToStdErr("Rebonding devices.");
        cDvbDevice::BondDevices(Devices.ReadString("Device", "Bondings", "").c_str());
        }
     }
}

void cDeviceManager::DelAdapter(std::string Adapter) {
  std::string path("/dev/dvb/" + Adapter);
  int adapter = std::stoi(Adapter.substr(7));
    
  if (not KnownDevice(adapter))
     return;

  for(int i = 0; i < cDevice::NumDevices(); i++) {
     /* okay - we need a minimum patch here to actually delete devices. */
     }
}

void cDeviceManager::GetDeviceInfo(int Index, cDeviceInfo& Dest) {
  cDevice* device = cDevice::GetDevice(Index);

  if (device == nullptr)
     Dest.Name = "device nullptr";
  else {
     Dest.Name = *device->DeviceName();
     if (Dest.Name.empty())
        Dest.Name = "(empty)";
     }

  Dest.Device        = "device" + std::to_string(Index);
  Dest.Adapter       = Devices.ReadString(Dest.Device, "Adapter", "none");
  Dest.FirstFrontend = Devices.ReadString(Dest.Device, "FirstFrontend", "none");
  Dest.Demux         = Devices.ReadString(Dest.Device, "Demux", "none");
  Dest.Frontends     = std::to_string(Devices.ReadInteger(Dest.Device, "Frontends", 0));
  Dest.MultiFrontend = Devices.ReadBool(Dest.Device, "MultiFrontend", false)?"true":"false";
  Dest.Loaded        = "no";
  Dest.Available     = Dest.Adapter != "none";
  Dest.Dvb           = Devices.ReadBool(Dest.Device, "IsDvb", false);

  if (Dest.Available) {
     Dest.Adapter = "/dev/dvb/" + Dest.Adapter;
     if (Dest.FirstFrontend != "none")
        Dest.FirstFrontend.insert(0, Dest.Adapter + "/");
     if (Dest.Demux != "none")
        Dest.Demux.insert(0, Dest.Adapter + "/");
     if (Devices.ReadBool(Dest.Device, "Late", false))
        Dest.Loaded = "by easyvdr plugin";
     else
        Dest.Loaded = "at VDR start";
     }
}

void cDeviceManager::MaxDevices(size_t num) {
  maxDevices = std::min(num, (size_t) MAXDEVICES);
}

void cDeviceManager::AddHook(int index) {
  const unsigned any = -1;
  std::string Device = "device" + std::to_string(index);
  cHook h;
  h.source = any;
  h.epg = Devices.ReadBool(Device, "ProvidesEpg", true);

  if (not Devices.ReadBool(Device, "ProvidesSourceA", true))
     h.source &= ~1;
  if (not Devices.ReadBool(Device, "ProvidesSourceC", true))
     h.source &= ~4;
  if (not Devices.ReadBool(Device, "ProvidesSourceT", true))
     h.source &= ~16;
  if (not Devices.ReadBool(Device, "ProvidesSourceT2", true))
     h.source &= ~32;
  if (not Devices.ReadBool(Device, "ProvidesSourceS", true))
     h.source &= ~64;
  if (not Devices.ReadBool(Device, "ProvidesSourceS2", true))
     h.source &= ~128;

  if (not h.epg or (h.source != any)) {
     h.device = cDevice::GetDevice(index);
     hooks.push_back(h);
     }
}

bool cDeviceManager::DeviceProvidesTransponder(const cDevice* Device, const cChannel* Channel) const {
  unsigned s;

  if      (Channel->IsAtsc())  s = 1;
  else if (Channel->IsCable()) s = 4;
  else if (Channel->IsTerr())  s = 16;
  else if (Channel->IsSat())   s = 64;
  else                         s = 0;

  if (std::string(Channel->Parameters()).find("S1") != std::string::npos) s <<= 1;

  for(auto h:hooks) {
     if (h.device != Device)
        continue;
     return (h.source & s) > 0;
     }
  return true;  
}

bool cDeviceManager::DeviceProvidesEIT(const cDevice* Device) const {
  for(auto h:hooks) {
     if (h.device != Device)
        continue;
     return h.epg;
     }
  return true;
}

void cDeviceManager::Enabled(bool On) {
  #ifdef __DYNAMIC_DEVICE_PROBE
     #warning "DeviceManager feature disabled - incompatible with dynamite vdr patch"
     (void) On;
     enabled = false;
  #else
     enabled = On;
  #endif
}

bool cDeviceManager::Enabled(void) {
  return enabled;
}

/*******************************************************************************
 * cDvbWatcher
 ******************************************************************************/
cDvbWatcher::cDvbWatcher(cDeviceManager* Parent) :
  iNotifyBase("/dev/dvb"), parent(Parent)
{
  AddWatch();
}

cDvbWatcher::~cDvbWatcher() {
  Cancel();
  RemoveWatch();
}

void cDvbWatcher::StartupDvb(void) {
  auto items = cFileList("/dev/dvb", "adapter").List();
  for(auto s:items) OnNewDir(s);
  Start();
}

void cDvbWatcher::OnNewDir(std::string Name) {
  parent->NewAdapter(Name);
}

void cDvbWatcher::OnDelDir(std::string Name) {
  parent->DelAdapter(Name);
}    


/*******************************************************************************
 * cDevWatcher
 ******************************************************************************/
cDevWatcher::cDevWatcher(cDeviceManager* Parent) :
  iNotifyBase("/dev"), parent(Parent), dvb(nullptr), started(false)
{
  Start();
  auto items = cFileList("/dev", "dvb").List();
  for(auto s:items)
     if (s == "dvb") {
        CreateDvbWatch();
        break;
        }
}

cDevWatcher::~cDevWatcher() {
  DeleteDvbWatch();
  Cancel();
  RemoveWatch();
}

void cDevWatcher::StartupDvb(void) {
  started = true;
  if (dvb) dvb->StartupDvb();
}

void cDevWatcher::CreateDvbWatch(void) {
  if (!dvb) {
     dvb = new cDvbWatcher(parent);
     if (started) dvb->StartupDvb();
     }
}

void cDevWatcher::DeleteDvbWatch(void) {
  delete dvb;
  dvb = nullptr;
}

void cDevWatcher::OnNewDir(std::string Name) {
  if (Name == "dvb") CreateDvbWatch();
}

void cDevWatcher::OnDelDir(std::string Name) {
  if (Name == "dvb") DeleteDvbWatch();
}

void cDevWatcher::OnDelWatchedFolder(std::string Name) {
  DeleteDvbWatch();
  Cancel();
}
