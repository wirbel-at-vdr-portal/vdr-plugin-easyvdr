/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include "FileList.h"
#include "IniFile.h"


/* forward declarations. */
bool GetArg(std::string& Dest, int& pos, int nargs, const char* args[], bool check = true);
int ErrorMessage(std::string msg);
int ListPlugins(std::string PluginDir);
int EnabledPlugins(std::string PluginDir, std::string IniDir);
int DisabledPlugins(std::string PluginDir, std::string IniDir);
int AllStatus(std::string PluginDir, std::string IniDir);
int Enable(std::string PluginDir, std::string IniDir, std::string Plugin);
int Disable(std::string PluginDir, std::string IniDir, std::string Plugin);
int Status(std::string PluginDir, std::string IniDir, std::string Plugin, bool Header);
int GetCommandline(std::string PluginDir, std::string IniDir, std::string Plugin);
int SetCommandline(std::string PluginDir, std::string IniDir, std::string Plugin, std::string CmdLine);
int DelCommandline(std::string PluginDir, std::string IniDir, std::string Plugin, std::string What);
int AddCommandline(std::string PluginDir, std::string IniDir, std::string Plugin, std::string What);
int ReplaceCommandline(std::string PluginDir, std::string IniDir, std::string Plugin, std::string From, std::string To);



int HelpText(std::string ProgName) {
  std::cout << ProgName <<
  " - a commandline tool for the easyvdr VDR Plugin.\n"
  "It configures VDR Plugins by writing to their ini files.\n\n"
  "The following options are available.\n"
  "   -h, --help\n"
  "          show this help text and exit.\n"
  "   --list-plugins\n"
  "          show all available plugins\n"
  "   --enabled-plugins\n"
  "         show all enabled plugins\n"
  "   --disabled-plugins\n"
  "         show all disabled plugins\n"
  "   --all-status\n"
  "         see --status, but for all known plugins.\n"
  "   --status\n"
  "         shows the status and commandline of a plugin.\n"
  "         Requires --plugin PLUGINNAME.\n"
  "   --plugin PLUGINNAME\n"
  "         selects the plugin PLUGINNAME for commands.\n"
  "   --plugindir DIRECTORY\n"
  "          use DIRECTORY as path for plugins libs,\n"
  "          instead of /usr/local/lib/vdr\n"
  "   --inidir DIRECTORY\n"
  "          use DIRECTORY as path for plugin configs,\n"
  "          instead of /etc/vdr/conf.d\n"
  "   --enable\n"
  "         enable autorun of this plugin.\n"
  "         Requires --plugin PLUGINNAME.\n"
  "   --disable\n"
  "         disable autorun of this plugin.\n"
  "         Requires --plugin PLUGINNAME.\n"
  "   --get-commandline\n"
  "         prints the commandline of a plugin to stdout and exit.\n"
  "         Requires --plugin PLUGINNAME.\n"
  "   --set-commandline STRING\n"  
  "         set the commandline of a plugin to STRING.\n"
  "         Requires --plugin PLUGINNAME.\n"
  "   --del-commandline STRING\n"  
  "         deletes STRING from the commandline of a plugin,\n"
  "         does nothing if that string is not found.\n"
  "         Requires --plugin PLUGINNAME.\n"
  "   --add-commandline STRING\n"  
  "         adds STRING to the commandline of a plugin.\n"
  "         Requires --plugin PLUGINNAME.\n"
  "   --replace-commandline FROM TO\n"
  "         requires two arguments: FROM and TO\n"           
  "         replaces the first hit of FROM by TO in plugins commandline\n"
  "         Requires --plugin PLUGINNAME.\n"

  << std::endl;
  return 0;
}


int main(int nargs, const char* args[]) {
  std::string ProgName(args[0]);
  std::string plugin;
  std::string plugindir("/usr/local/lib/vdr");
  std::string inidir("/etc/vdr/conf.d");
  std::string from,to;

  enum {
      unset = 0,
      list_plugins,
      enabled_plugins,
      disabled_plugins,
      all_status,
      enable,
      disable,
      status,
      get_commandline,
      set_commandline,
      del_commandline,
      add_commandline,
      replace_commandline
      } cmd = unset;

  ProgName.erase(0, ProgName.find_last_of("/\\")+1);
  
  for(int i=1; i<nargs; i++) {
     std::string Argument = args[i];
     std::string Param;

     if ((Argument == "-h") or (Argument == "--help"))
        return HelpText(ProgName);
     else if (Argument == "--plugin") {
        if (not GetArg(plugin, i, nargs, args))
           return ErrorMessage("missing plugin name.");
        }
     else if (Argument == "--plugindir") {
        if (not GetArg(plugindir, i, nargs, args))
           return ErrorMessage("missing plugindir path.");
        if (*plugindir.rbegin() == '/') plugindir.erase(plugindir.size()-1);
        }
     else if (Argument == "--inidir") {
        if (not GetArg(inidir, i, nargs, args))
           return ErrorMessage("missing inidir path.");
        if (*inidir.rbegin() == '/') inidir.erase(inidir.size()-1);
        }
     else if (Argument == "--list-plugins")
        cmd = list_plugins;
     else if (Argument == "--all-status")
        cmd = all_status;
     else if (Argument == "--enabled-plugins")
        cmd = enabled_plugins;
     else if (Argument == "--disabled-plugins")
        cmd = disabled_plugins;
     else if (Argument == "--enable")
        cmd = enable;
     else if (Argument == "--disable")
        cmd = disable;
     else if (Argument == "--status")
        cmd = status;
     else if (Argument == "--get-commandline")
        cmd = get_commandline;
     else if (Argument == "--set-commandline") {
        cmd = set_commandline;
        if (not GetArg(to, i, nargs, args, false))
           return ErrorMessage("missing commandline argument.");
        }
     else if (Argument == "--del-commandline") {
        cmd = del_commandline;
        if (not GetArg(to, i, nargs, args, false))
           return ErrorMessage("missing commandline argument.");
        }
     else if (Argument == "--add-commandline") {
        cmd = add_commandline;
        if (not GetArg(to, i, nargs, args, false))
           return ErrorMessage("missing commandline argument.");
        }
     else if (Argument == "--replace-commandline") {
        cmd = replace_commandline;

        if (not GetArg(from, i, nargs, args, false))
           return ErrorMessage("missing argument.");

        if (not GetArg(to, i, nargs, args, false))
           return ErrorMessage("missing argument.");
        }
     else return ErrorMessage("cannot understand '" + Argument + "'");
     }

  switch(cmd) {
     case list_plugins        : return ListPlugins(plugindir);
     case enabled_plugins     : return EnabledPlugins(plugindir, inidir); 
     case disabled_plugins    : return DisabledPlugins(plugindir, inidir);
     case all_status          : return AllStatus(plugindir, inidir);      
     case enable              : return Enable(plugindir, inidir, plugin);          
     case disable             : return Disable(plugindir, inidir, plugin);         
     case status              : return Status(plugindir, inidir, plugin, true);          
     case get_commandline     : return GetCommandline(plugindir, inidir, plugin); 
     case set_commandline     : return SetCommandline(plugindir, inidir, plugin, to); 
     case del_commandline     : return DelCommandline(plugindir, inidir, plugin, to);
     case add_commandline     : return AddCommandline(plugindir, inidir, plugin, to);
     case replace_commandline : return ReplaceCommandline(plugindir, inidir, plugin, from, to);
     case unset               : return 0;
     default                  : return 0;
     };

  return 0; // not reached.
}






/*******************************************************************************
 * Implementation of the functions follows.
 ******************************************************************************/

int ErrorMessage(std::string msg) {
  std::cerr << "ERROR: " << msg << std::endl;
  return -1;
}

void ToStdOut(std::string s) {
  std::cerr << s << std::endl;
}

std::string FileExt(void) {
  return ".so." + std::string(APIVERSION);
}

bool FileExists(std::string FileName) {
  return (access(FileName.c_str(), R_OK) == 0); 
}

std::vector<std::string> AvailablePlugins(std::string PluginDir) {
  static bool initialized = false;
  static std::vector<std::string> v;
  if (not initialized) {
     cFileList files(PluginDir, FileExt());     
     for(auto s:files.List()) {
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
     
        if (not s.empty() and (s != "easyvdr"))
           v.push_back(s);
        }
     initialized = true;
     }
  return v;
}

std::vector<std::string> ConfiguredPlugins(std::string IniDir) {
  static bool initialized = false;
  static std::vector<std::string> v;
  if (not initialized) {
     cFileList files(IniDir,  "_settings.ini");
     v = files.List();
     for(auto& s:v)
        s.erase(s.size()-strlen("_settings.ini"));

     initialized = true;
     }
  return v; 
}

int ListPlugins(std::string PluginDir) {
  for(auto s:AvailablePlugins(PluginDir))
     ToStdOut(s);
  return 0;
}

int EnabledPlugins(std::string PluginDir, std::string IniDir) {
  for(auto s:AvailablePlugins(PluginDir)) {
     std::string ConfigFile = IniDir + "/" + s + "_settings.ini";
     if (not FileExists(ConfigFile))
        continue;

     TIniFile ini(ConfigFile);
     if (not ini.Valid())
        continue;

     if (not ini.ReadBool("EasyPluginManager", "AutoRun", false))
        continue;

     ToStdOut(s);
     }
  return 0;
}

int DisabledPlugins(std::string PluginDir, std::string IniDir) {
  for(auto s:AvailablePlugins(PluginDir)) {
     std::string ConfigFile = IniDir + "/" + s + "_settings.ini";
     if (not FileExists(ConfigFile)) {
        ToStdOut(s);
        continue;
        }

     TIniFile ini(ConfigFile);
     if (not ini.Valid()) {
        ToStdOut(s);
        continue;
        }

     if (ini.ReadBool("EasyPluginManager", "AutoRun", false))
        continue;

     ToStdOut(s);
     }
  return 0;
}

std::string FillString(std::string s, size_t len, char c = ' ') {
  if (s.size() < len)
     s += std::string(len-s.size(),c);
  return s;
}

void StatusHeader(void) {
  ToStdOut(FillString(" Plugin", 20) + "|" +
           FillString(" install", 9) + "|" +
           FillString(" ini", 9) + "|" +
           FillString(" AutoRun", 9) + "|" +
           FillString(" Stop", 6) + "|" +
           " Arguments");
  ToStdOut(FillString("",80,'-'));
}

int AllStatus(std::string PluginDir, std::string IniDir) {
  StatusHeader();
  auto avail = AvailablePlugins(PluginDir);
  auto confs = ConfiguredPlugins(IniDir);
  avail.insert(avail.end(), confs.begin(), confs.end());

  std::sort(avail.begin(), avail.end());
  avail.erase(std::unique(avail.begin(), avail.end()), avail.end());

  for(auto Plugin:avail)
     Status(PluginDir, IniDir, Plugin, false);
  return 0;
}

int Status(std::string PluginDir, std::string IniDir, std::string Plugin, bool Header) {
  if (Plugin.empty())
     return ErrorMessage("missing plugin name");

  auto av = AvailablePlugins(PluginDir);
  bool installed = std::find(av.begin(), av.end(), Plugin) != av.end();
  std::string ConfigFile = IniDir + "/" + Plugin + "_settings.ini";
  std::string s(FillString(" " + Plugin, 20) + "|");

  if (Header)
     StatusHeader();

  if (installed)
     s += FillString(" yes", 9) + "|";
  else
     s += FillString(" no", 9) + "|";

  if (not FileExists(ConfigFile)) {
     s += FillString(" missing", 9) + "|";
     s += FillString(" no", 9) + "|";
     s += FillString(" yes", 6) + "|";
     ToStdOut(s);
     return 0;
     }

  TIniFile ini(ConfigFile);
  if (not ini.Valid()) {
     s += FillString(" invalid", 9) + "|";
     s += FillString(" no", 9) + "|";
     s += FillString(" yes", 6) + "|";
     ToStdOut(s);
     return 0;
     }

  s += FillString(" valid", 9) + "|";

  if (ini.ReadBool("EasyPluginManager", "AutoRun", false))
     s += FillString(" yes", 9) + "|";
  else
     s += FillString(" no", 9) + "|";

  if (ini.ReadBool("EasyPluginManager", "Stop", true))
     s += FillString(" yes", 6) + "|";
  else
     s += FillString(" no", 6) + "|";
  
  s += " " + ini.ReadString("EasyPluginManager", "Args", "");
  ToStdOut(s);
  return 0;
}

int Enable(std::string PluginDir, std::string IniDir, std::string Plugin) {
  if (Plugin.empty())
     return ErrorMessage("missing plugin name");

  std::string ConfigFile = IniDir + "/" + Plugin + "_settings.ini";
  TIniFile ini(ConfigFile);
  ini.WriteBool("EasyPluginManager", "AutoRun", true);
  return ini.UpdateFile() ? 0 : -1;
}

int Disable(std::string PluginDir, std::string IniDir, std::string Plugin) {
  if (Plugin.empty())
     return ErrorMessage("missing plugin name");

  std::string ConfigFile = IniDir + "/" + Plugin + "_settings.ini";
  TIniFile ini(ConfigFile);
  ini.WriteBool("EasyPluginManager", "AutoRun", false);
  return ini.UpdateFile() ? 0 : -1;
}

int GetCommandline(std::string PluginDir, std::string IniDir, std::string Plugin) {
  if (Plugin.empty())
     return ErrorMessage("missing plugin name");

  std::string ConfigFile = IniDir + "/" + Plugin + "_settings.ini";
  TIniFile ini(ConfigFile);
  ToStdOut(ini.ReadString("EasyPluginManager", "Args", ""));
  return 0;
}

int SetCommandline(std::string PluginDir, std::string IniDir, std::string Plugin, std::string CmdLine) {
  if (Plugin.empty())
     return ErrorMessage("missing plugin name");

  std::string ConfigFile = IniDir + "/" + Plugin + "_settings.ini";
  TIniFile ini(ConfigFile);
  ini.WriteString("EasyPluginManager", "Args", CmdLine);
  return ini.UpdateFile() ? 0 : -1;
}

int DelCommandline(std::string PluginDir, std::string IniDir, std::string Plugin, std::string What) {
  if (Plugin.empty())
     return ErrorMessage("missing plugin name");

  std::string ConfigFile = IniDir + "/" + Plugin + "_settings.ini";
  TIniFile ini(ConfigFile);
  std::string CmdLine(ini.ReadString("EasyPluginManager", "Args", ""));
  if (CmdLine.empty())
     return 0;

  size_t pos = CmdLine.find(What);
  if (pos == std::string::npos)
     return 0;

  CmdLine.erase(pos, What.size());

  ini.WriteString("EasyPluginManager", "Args", CmdLine);
  return ini.UpdateFile() ? 0 : -1;
}


int AddCommandline(std::string PluginDir, std::string IniDir, std::string Plugin, std::string What) {
  if (Plugin.empty())
     return ErrorMessage("missing plugin name");

  std::string ConfigFile = IniDir + "/" + Plugin + "_settings.ini";
  TIniFile ini(ConfigFile);
  std::string CmdLine(ini.ReadString("EasyPluginManager", "Args", ""));

  CmdLine += What;

  ini.WriteString("EasyPluginManager", "Args", CmdLine);
  return ini.UpdateFile() ? 0 : -1;
}


int ReplaceCommandline(std::string PluginDir, std::string IniDir, std::string Plugin, std::string From, std::string To) {
  if (Plugin.empty())
     return ErrorMessage("missing plugin name");

  std::string ConfigFile = IniDir + "/" + Plugin + "_settings.ini";
  TIniFile ini(ConfigFile);
  std::string CmdLine(ini.ReadString("EasyPluginManager", "Args", ""));
  if (CmdLine.empty())
     return 0;

  size_t pos = CmdLine.find(From);
  if (pos == std::string::npos)
     return 0;

  CmdLine.replace(pos, From.size(), To);

  ini.WriteString("EasyPluginManager", "Args", CmdLine);
  return ini.UpdateFile() ? 0 : -1;
}


bool GetArg(std::string& Dest, int& pos, int nargs, const char* args[], bool check) {
  if (++pos<nargs) {
     Dest = args[pos];
     if (check)
        return Dest.find('-') != 0;
     return true;
     }
  return false;
}
