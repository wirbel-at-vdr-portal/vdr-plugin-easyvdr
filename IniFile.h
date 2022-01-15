/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 * series  - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <string>
#include <vector>
#include <tuple>



class TIniFile {
private:
  static constexpr size_t npos = std::string::npos;

  bool valid;
  bool modified;
  std::string filename;
  std::vector<std::tuple<std::string,std::string,std::string>> items;

  static void Trim(std::string& s, std::string to_trim = "\t ");
  static std::vector<std::string> split(const std::string& s, const char delim);
  bool Parse(std::stringstream& ss);
  std::vector<std::tuple<std::string,std::string,std::string>>::iterator
  Get(std::string Section, std::string Ident);
  bool StrToBool(std::string s);
  std::string BoolToStr(bool b);
  void PrintTuple(std::tuple<std::string,std::string,std::string> t);
  void StdErr(const char* f, int l, std::string msg);
public:
  TIniFile(void);
  TIniFile(std::string aFileName);
  ~TIniFile() {}

  // read ini from file system.
  bool ReadFile(std::string aFileName);

  // write changes to disk.
  // The newly written ini doesnt contain comments or empty lines
  // and is sorted case-sensitive.
  bool UpdateFile(void);

  // true, if 'filename' was read and is a valid ini file.
  bool Valid(void);

  // true, if contents changed and UpdateFile() should be called.
  bool Modified(void);

  // the name of the ini file on disk
  std::string FileName(void);

  // print contents on stdout
  void PrintTuples(void);

  // Deletes the key Ident from section Section.
  // returns true if a key was actually deleted, false otherwise.
  bool DeleteKey(std::string Section, std::string Ident);

  // Check if a section exists.
  bool SectionExists(std::string Section);

  // Checks whether the key Ident exists in section Section
  bool ValueExists(std::string Section, std::string Ident);

  //  returns the names of existing non-empty sections in Sections
  void ReadSections(std::vector<std::string>& Sections);

  // return the names of the keys in section Section in Idents
  void ReadSection(std::string Section, std::vector<std::string>& Idents);

  // erase a section with all keys
  bool EraseSection(std::string Section);

  // read a key as a string, if not found Default is returned. 
  std::string ReadString (std::string Section, std::string Ident, std::string Default);

  // read a key as a integer, if not found Default is returned.
  // May throw exception, if conversion to integer fails.
  int ReadInteger(std::string Section, std::string Ident, int Default);

  // read a key as a double, if not found Default is returned.
  // May throw exception, if conversion to double fails, ie. comma vs. dot
  double ReadFloat(std::string Section, std::string Ident, double Default);

  // read a key as a bool, if not found Default is returned.
  // valid for true  values are lowercase 'true' , or '1'.
  // valid for false values are lowercase 'false', or '0'.
  // Anything else is read as false.
  bool ReadBool(std::string Section, std::string Ident, bool Default);

  // write a key as a string
  void WriteString(std::string Section, std::string Ident, std::string Value);

  // write a key as a integer
  void WriteInteger(std::string Section, std::string Ident, int Value);

  // write a key as a double
  void WriteFloat(std::string Section, std::string Ident, double Value);

  // write a key as a bool: 'true' or 'false'
  void WriteBool(std::string Section, std::string Ident, bool Value);

  // clear all contents
  void Clear(void);
};
