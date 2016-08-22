#include <stdio.h>
#include <stdlib.h>
#include <ctime>

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>

#include <string>

#include "file_watcher.h"

FileWatcher::FileWatcher(const char* pfilepath, Callback* pcb)
:file_(pfilepath), pcb_(pcb)
{
	struct stat st1;
	stat(file_.c_str(), &st1);
	last_reload_time_ = st1.st_mtime;
};

bool FileWatcher::check()
{
	if(!pcb_ || !file_.size()) return false;

	struct stat st1;
	stat(file_.c_str(), &st1);

	if( st1.st_mtime > last_reload_time_)
	{
		(*pcb_)();
        last_reload_time_ = st1.st_mtime;
		return true;
	}

	return false;
}
