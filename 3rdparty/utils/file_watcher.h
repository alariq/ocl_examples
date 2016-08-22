#ifndef __FILE_WATCHER_H__
#define __FILE_WATCHER_H__


class FileWatcher {
public:
	struct Callback {
		virtual void operator()() = 0;
	};

	FileWatcher(const char* pfilepath, Callback* pcb);
	bool check();
	
private:
	std::string file_;
	Callback* pcb_;
	time_t last_reload_time_;
};



#endif // __FILE_WATCHER_H__