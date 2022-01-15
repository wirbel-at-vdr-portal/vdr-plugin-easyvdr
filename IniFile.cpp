/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 * series  - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <tuple>
#include <algorithm>
#include "IniFile.h"

#define ToStdErr(msg) StdErr(__PRETTY_FUNCTION__,__LINE__,msg)


/*******************************************************************************
 * TIniFile
 ******************************************************************************/
TIniFile::TIniFile(void) : valid(false), modified(false) {}

TIniFile::TIniFile(std::string aFileName) : valid(false), modified(false), filename(aFileName) {
  ReadFile(aFileName);
}

bool TIniFile::ReadFile(std::string aFileName) {
  if (valid)
     Clear();

  std::ifstream is(aFileName.c_str());
  if (is) {
     std::stringstream ss;
     ss << is.rdbuf();
     is.close();

     valid = Parse(ss);
     }
  return valid;
}

bool TIniFile::Parse(std::stringstream& ss) {
  std::vector<std::string> lines;
  std::string s,section;
  size_t line = 0;

  items.clear();
  while(std::getline(ss, s)) {
     if (s.back() == '\r') s.pop_back(); // DOS \r\n -> UNIX \n
     line++;
     if (s.empty())
        continue;

     Trim(s);
     if (s.empty() or (s.find('#') == 0))
        continue;

     if (s.find('[') == 0) {
        if (s.find(']') == npos) {
           ToStdErr("error in " + filename + " line " + std::to_string(line) +
                    ", missing closing bracket.");
           return false;
           }
        Trim(s,"[]");
        section = s;
        continue;
        }

     if (s.find('=') != npos) {
        if (section.empty()) {
           ToStdErr("error in " + filename + " line " + std::to_string(line) +
                    ", pair without section.");
           return false;
           }
        auto pair = split(s,'=');
        if (pair.size() > 2) {
           for(size_t i = 2; i < pair.size(); i++)
              pair[1] += "=" + pair[i];
           }
        else if (pair.size() < 2)
           pair.push_back("");
        Trim(pair[0]);
        Trim(pair[1]);
        items.push_back(std::make_tuple(section,pair[0],pair[1]));
        continue;
        }
     ToStdErr("error in " + filename + " line " + std::to_string(line) +
              ", '" + s + "'" + " is neither comment, pair or section.");
     return false;
     }
  return true;
}

bool TIniFile::UpdateFile(void) {
  std::sort(items.begin(), items.end());

  std::stringstream ss;
  std::string section;
  for(auto i:items) {
     if (section != std::get<0>(i)) {
        section = std::get<0>(i);
        ss << "[" << section << "]\n";
        }
     ss << std::get<1>(i) << "=" << std::get<2>(i) << "\n";
     }

  std::ofstream fs(filename.c_str());
  if (fs.fail())
     return false;
  fs << ss.rdbuf();
  bool result = fs.good();
  fs.close();
  modified = false;
  return result;
}

bool TIniFile::Valid(void) {
  return valid;
}

bool TIniFile::Modified(void) {
  return modified;
}

std::string TIniFile::FileName(void) {
  return filename;
}

void TIniFile::PrintTuple(std::tuple<std::string,std::string,std::string> t) {
  std::cout << std::get<0>(t) << "::"
            << std::get<1>(t) << '='
            << std::get<2>(t) << std::endl;
}

void TIniFile::PrintTuples(void) {
  for(auto t:items) PrintTuple(t);
}

std::vector<std::tuple<std::string,std::string,std::string>>::iterator
TIniFile::Get(std::string Section, std::string Ident) {
  auto it = items.begin();
  for(;it!=items.end();it++)
     if ((std::get<0>(*it) == Section) and (std::get<1>(*it) == Ident))
        return it;

  return it;
}

bool TIniFile::DeleteKey(std::string Section, std::string Ident) {
  auto it = Get(Section, Ident);
  if (it == items.end())
     return false;

  items.erase(it);
  modified = true;
  return true;
}

void TIniFile::ReadSections(std::vector<std::string>& Sections) {
  std::string section;
  Sections.clear();
  for(auto i:items)
     if (std::get<0>(i) != section) {
        section = std::get<0>(i);
        Sections.push_back(section);
        }
}

bool TIniFile::SectionExists(std::string Section) {
  for(auto i:items)
     if (std::get<0>(i) == Section)
        return true;
  return false;
}

bool TIniFile::ValueExists(std::string Section, std::string Ident) {
  return Get(Section, Ident) != items.end();
}

void TIniFile::ReadSection(std::string Section, std::vector<std::string>& Idents) {
  Idents.clear();
  for(auto i:items)
     if (std::get<0>(i) == Section)
        Idents.push_back(std::get<1>(i));
}

bool TIniFile::EraseSection(std::string Section) {
  bool found = false;
  auto it = items.begin();
  for(;it!=items.end();it++)
     if (std::get<0>(*it) == Section) {
        items.erase(it);
        it = items.begin();
        found = true;
        }
  modified = found;
  return found;
}

std::string TIniFile::BoolToStr(bool b) {
  return b?"true":"false";
}

bool TIniFile::StrToBool(std::string s) {
  transform(s.begin(), s.end(), s.begin(), ::tolower);
  return (s == "true") or (s == "1");
}

std::string TIniFile::ReadString(std::string Section, std::string Ident, std::string Default) {
  auto it = Get(Section, Ident);
  if (it == items.end())
     return Default;
  return std::get<2>(*it);  
}

int TIniFile::ReadInteger(std::string Section, std::string Ident, int Default) {
  return std::stoi(ReadString(Section, Ident, std::to_string(Default)));
}

double TIniFile::ReadFloat(std::string Section, std::string Ident, double Default) {
  return std::stod(ReadString(Section, Ident, std::to_string(Default)));
}

bool TIniFile::ReadBool(std::string Section, std::string Ident, bool Default) { 
  return StrToBool(ReadString(Section, Ident, BoolToStr(Default)));
}

void TIniFile::WriteString(std::string Section, std::string Ident, std::string Value) {
  auto it = Get(Section, Ident);
  if (it == items.end()) {
     items.push_back(std::make_tuple(Section,Ident,Value));
     modified = true;
     return;
     }
  std::get<2>(*it) = Value;
  modified = true;
}

void TIniFile::WriteInteger(std::string Section, std::string Ident, int Value) {
  WriteString(Section, Ident, std::to_string(Value));
}

void TIniFile::WriteFloat(std::string Section, std::string Ident, double Value) {
  WriteString(Section, Ident, std::to_string(Value));
}

void TIniFile::WriteBool(std::string Section, std::string Ident, bool Value) {
  WriteString(Section, Ident, BoolToStr(Value));
}

std::vector<std::string> TIniFile::split(const std::string& s, const char delim) {
  std::stringstream ss(s);
  std::vector<std::string> result;
  std::string item;
  while(std::getline(ss, item, delim))
     result.push_back(item);
  return result;
}

void TIniFile::Trim(std::string& s, std::string to_trim) {
  if (s.empty()) return;
  size_t first = s.find_first_not_of(to_trim);
  size_t last  = s.find_last_not_of(to_trim);
  if (last != std::string::npos)
     s = s.substr(first,last-first+1);
  else
     s = "";
}

void TIniFile::StdErr(const char* f, int l, std::string msg) {
  std::cerr << f << ':' << l << " : " << msg << std::endl;
}

void TIniFile::Clear(void) {
  items.clear();
  modified = false;
  valid = false;
}
