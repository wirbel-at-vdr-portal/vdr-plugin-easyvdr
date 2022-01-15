/******************************************************************************
 * easyvdr - A plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *****************************************************************************/
#pragma once
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <limits.h>
#include "ThreadBase.h"

class iNotifyBase : public ThreadBase {
private:
  std::string dir;
  uint32_t mask;
  int fd;
  int watch;
  bool debug;
  void StdErr(std::string s) {
     if (debug) std::cerr << s << std::endl;
     }
  bool Flags(const struct inotify_event* ev, uint32_t mask) {
     return (ev->mask & mask) == mask;
     }
protected:
  void Action(void) {
     if (not AddWatch())
        return;

     while(Running()) {
        char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
        const struct inotify_event* ev;
        ssize_t len = read(fd, buf, sizeof(buf));

        if (len < 0) {
           if (errno == EAGAIN)
              continue;
           else
              break;
           }

        for(char* p = buf; p < (buf + len); p += sizeof(struct inotify_event) + ev->len) {
           ev = (const struct inotify_event*) p;

           if (ev->len < 1) continue;

           if (debug) StdErr("name   = " + std::string(ev->name));

           if (Flags(ev, IN_ISDIR | IN_CREATE))            OnNewDir(ev->name);
           else if (Flags(ev, IN_ISDIR | IN_DELETE))       OnDelDir(ev->name);
           else if (Flags(ev, IN_CREATE))                  OnNewFile(ev->name);
           else if (Flags(ev, IN_DELETE))                  OnDelFile(ev->name);
           else if (Flags(ev, IN_ISDIR | IN_DELETE_SELF))  OnDelWatchedFolder(ev->name);
           }
        }
     RemoveWatch();
     close(fd);
     }
public:
  iNotifyBase(std::string Dir, uint32_t Mask = IN_CREATE | IN_DELETE | IN_DELETE_SELF) :
    dir(Dir), mask(Mask), fd(-1), watch(-1) {}
  ~iNotifyBase() { RemoveWatch(); }
  virtual void OnNewDir(std::string Name)           {(void) Name;}
  virtual void OnDelDir(std::string Name)           {(void) Name;}
  virtual void OnNewFile(std::string Name)          {(void) Name;}
  virtual void OnDelFile(std::string Name)          {(void) Name;}
  virtual void OnDelWatchedFolder(std::string Name) {(void) Name;}
  void Debug(bool On) { debug = On; }
  void RemoveWatch(void) {
     if (watch >= 0 and fd >= 0) {
        inotify_rm_watch(fd, watch);
        watch = -1;
        }
     }
  bool AddWatch(void) {
     if (fd < 0) {
        if ((fd = inotify_init()) < 0) {
           StdErr("inotify_init() failed");
           return false;
           }
        }
     if (watch < 0) {
        if ((watch = inotify_add_watch(fd, dir.c_str(), mask)) < 0) {
           close(fd); fd = -1;
           StdErr("inotify_add_watch() failed for " + dir + ", " + strerror(errno));
           return false;
           }
        }
     return true;
     }
};
